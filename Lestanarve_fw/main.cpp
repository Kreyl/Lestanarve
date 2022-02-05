#include "board.h"
#include "ch.h"
#include "hal.h"
#include "uart2.h"
#include "led.h"
#include "Sequences.h"
#include "radio_lvl1.h"
#include "kl_lib.h"
#include "MsgQ.h"
#include "acg_lsm6ds3.h"
#include "SimpleSensors.h"
#include "ws2812b.h"
#include "LedEffects.h"
#include "kl_fs_utils.h"
#include "usb_msd.h"
#include "Settings.h"
#include "Motion.h"
#include "adcL476.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(256000, CMD_UART_PARAMS);
CmdUart_t Uart{CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// ==== LEDs ====
LedSmooth_t Lumos { LED_PIN };
static const NeopixelParams_t NpxParams {NPX_SPI, NPX_DATA_PIN,
    NPX_DMA, NPX_DMA_MODE(NPX_DMA_REQ),
    NPX_LED_CNT, npxRGB
};
Neopixels_t Leds{&NpxParams};

void LedPwrOn()  { PinSetHi(NPX_PWR_PIN); }
void LedPwrOff() { PinSetLo(NPX_PWR_PIN); }

// ==== USB & FileSys ====
FATFS FlashFS;
bool UsbIsConnected = false;

// ==== ADC ====
#define BATTERY_DEAD_mv 3300
void OnAdcDoneI();

const AdcSetup_t AdcSetup = {
        .SampleTime = ast12d5Cycles,
        .Oversampling = AdcSetup_t::oversmp128,
        .DoneCallback = OnAdcDoneI,
        .Channels = {
                {BAT_ADC_PIN},
                {nullptr, 0, ADC_VREFINT_CHNL}
        }
};

static void EnterSleepNow();
static void EnterSleep();

// ==== Timers ====
static TmrKL_t TmrAcg {TIME_MS2I(11), evtIdAcc, tktPeriodic};
#endif

int main(void) {
#if 1 // ==== Init clock system ====
    Clk.SetVoltageRange(mvrHiPerf);
    Clk.SetupFlashLatency(40, mvrHiPerf);
    Clk.EnablePrefetch();
    // HSE or MSI
    if(Clk.EnableHSE() == retvOk) {
        Clk.SetupPllSrc(pllsrcHse);
        Clk.SetupM(3);
    }
    else { // PLL fed by MSI
        Clk.SetupPllSrc(pllsrcMsi);
        Clk.SetupM(1);
    }
    // SysClock 40MHz
    Clk.SetupPll(20, 2, 4);
    Clk.SetupBusDividers(ahbDiv1, apbDiv1, apbDiv1);
    if(Clk.EnablePLL() == retvOk) {
        Clk.EnablePllROut();
        Clk.SwitchToPLL();
    }
    // 48MHz clock for USB & 24MHz clock for ADC
    Clk.SetupPllSai1(24, 4, 2, 7); // 4MHz * 24 = 96; R = 96 / 4 = 24, Q = 96 / 2 = 48
    if(Clk.EnablePllSai1() == retvOk) {
        // Setup Sai1R as ADC source
        Clk.EnableSai1ROut();
        uint32_t tmp = RCC->CCIPR;
        tmp &= ~RCC_CCIPR_ADCSEL;
        tmp |= 0b01UL << 28; // SAI1R is ADC clock
        // Setup Sai1Q as 48MHz source
        Clk.EnableSai1QOut();
        tmp &= ~RCC_CCIPR_CLK48SEL;
        tmp |= 0b01UL << 26;
        RCC->CCIPR = tmp;
    }
    Clk.UpdateFreqValues();
#endif
    // === Init OS ===
    halInit();
    chSysInit();
    EvtQMain.Init();

    // ==== Init hardware ====
    Uart.Init();
    Printf("\r%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));
    Clk.PrintFreqs();
    if(Clk.IsHseOn()) Printf("Quartz ok\r\n");

    Lumos.Init();

    // ==== Leds ====
    Leds.Init();
    // LED pwr pin
    PinSetupOut(NPX_PWR_PIN, omPushPull);
    LedPwrOn();
//    Leds.SetAll(clGreen);
//    Leds.SetCurrentColors();

    // Init filesystem
    FRESULT err;
    err = f_mount(&FlashFS, "", 0);
    if(err == FR_OK) {
        Settings.Load();
        if(Settings.LoadSuccessful) Lumos.StartOrRestart(lsqLStart);
        else Lumos.StartOrRestart(lsqLError);
    }
    else Printf("FS error\r");

    Eff::Init();
    Motion::Init();
    Acg.Init();
    TmrAcg.StartOrRestart();

    UsbMsd.Init();
    SimpleSensors::Init();

    // Battery measurement
    PinSetupOut(BAT_MEAS_EN, omPushPull);
    PinSetHi(BAT_MEAS_EN); // Enable it forever, as 200k produces ignorable current

    // Inner ADC
    Adc.Init(AdcSetup);
    Adc.StartPeriodicMeasurement(1);
//    PinSetupAnalog(ADC_BAT_PIN);


    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
            case evtIdADC: {
                Iwdg::Reload();
                uint32_t Battery_mV = 2 * Adc.Adc2mV(Msg.Values16[0], Msg.Values16[1]); // *2 because of resistor divider
                Printf("VBat: %u mV\r", Battery_mV);
                if(Battery_mV < BATTERY_DEAD_mv) {
                    Printf("Discharged: %u\r", Battery_mV);
                    EnterSleep();
                }
            } break;

            case evtIdAcc:
                Motion::Update();
                break;

            case evtIdShellCmdRcvd:
                while(((CmdUart_t*)Msg.Ptr)->TryParseRxBuff() == retvOk) OnCmd((Shell_t*)((CmdUart_t*)Msg.Ptr));
                break;

#if 1       // ======= USB =======
            case evtIdUsbConnect:
                Printf("USB connect\r");
                UsbMsd.Connect();
                break;
            case evtIdUsbDisconnect:
                UsbMsd.Disconnect();
                Printf("USB disconnect\r");
                break;
            case evtIdUsbReady:
                Printf("USB ready\r");
                break;
#endif
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

void ProcessUsbDetect(PinSnsState_t *PState, uint32_t Len) {
    if((*PState == pssRising or *PState == pssHi) and !UsbIsConnected) {
        UsbIsConnected = true;
        EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbConnect));
    }
    else if((*PState == pssFalling or *PState == pssLo) and UsbIsConnected) {
        UsbIsConnected = false;
        EvtQMain.SendNowOrExit(EvtMsg_t(evtIdUsbDisconnect));
    }
}

void ProcessCharging(PinSnsState_t *PState, uint32_t Len) {
    if(*PState == pssLo) {
        Lumos.StartOrContinue(lsqLCharging);
        LedPwrOff();
    }
    else if(*PState == pssRising) { // Charge stopped
        Lumos.StartOrContinue(lsqLStart);
    }
    else if(*PState == pssHi) LedPwrOn();
}

void OnAdcDoneI() {
    AdcBuf_t &FBuf = Adc.GetBuf();
    EvtMsg_t Msg(evtIdADC);
    Msg.Values16[0] = FBuf[0];
    Msg.Values16[1] = FBuf[1];
    EvtQMain.SendNowOrExitI(Msg);
}

void EnterSleepNow() {
    // Enable inner pull-ups
    PWR->PUCRA |= PWR_PDCRA_PA0; // Charging
    // Enable inner pull-downs
    PWR->PDCRA |= PWR_PDCRA_PA2; // USB
    // Apply PullUps and PullDowns
    PWR->CR3 |= PWR_CR3_APC;
    // Enable wake-up srcs
    Sleep::EnableWakeup1Pin(rfFalling); // Charging
    Sleep::EnableWakeup4Pin(rfRising); // USB
    Sleep::ClearWUFFlags();
    Sleep::EnterStandby();
}

void EnterSleep() {
    Printf("Entering sleep\r");
    chThdSleepMilliseconds(45);
    chSysLock();
    EnterSleepNow();
    chSysUnlock();
}

#if 1 // ================= Command processing ====================
void OnCmd(Shell_t *PShell) {
	Cmd_t *PCmd = &PShell->Cmd;
    if(PCmd->NameIs("Ping")) {
        PShell->Ok();
    }
    else if(PCmd->NameIs("Version")) PShell->Print("%S %S\r", APP_NAME, XSTRINGIFY(BUILD_TIME));

    else if(PCmd->NameIs("Set")) {
        int R, A;
        Color_t Clr;
        if(PCmd->GetNext<int>(&R) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<int>(&A) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetClrRGB(&Clr) != retvOk) { PShell->BadParam(); return; }
        Eff::Set(R, A, Clr);
        Leds.SetCurrentColors();
        PShell->Ok();
    }

    else if(PCmd->NameIs("SetIndx")) {
        int Indx;
        Color_t Clr;
        if(PCmd->GetNext<int>(&Indx) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetClrRGB(&Clr) != retvOk) { PShell->BadParam(); return; }
        Leds.ClrBuf[Indx] = Clr;
        Leds.SetCurrentColors();
        PShell->Ok();
    }

    /*
    else if(PCmd->NameIs("Stoch")) {
        uint32_t DelayIdleMin, DelayIdleMax;
        uint32_t DelayOnMin, DelayOnMax;
        uint32_t SmoothMin, SmoothMax;
        uint16_t ClrHMin, ClrHMax;
        uint8_t ClrVIdle;
        if(PCmd->GetNext<uint32_t>(&DelayIdleMin) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint32_t>(&DelayIdleMax) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint32_t>(&DelayOnMin) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint32_t>(&DelayOnMax) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint32_t>(&SmoothMin) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint32_t>(&SmoothMax) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint16_t>(&ClrHMin) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint16_t>(&ClrHMax) != retvOk) { PShell->BadParam(); return; }
        if(PCmd->GetNext<uint8_t>(&ClrVIdle) != retvOk) { PShell->BadParam(); return; }
        chSysLock();
        StochSettings.DelayIdleMin = DelayIdleMin;
        StochSettings.DelayIdleMax = DelayIdleMax;
        StochSettings.DelayOnMin = DelayOnMin;
        StochSettings.DelayOnMax = DelayOnMax;
        StochSettings.SmoothMin = SmoothMin;
        StochSettings.SmoothMax = SmoothMax;
        StochSettings.ClrHMin = ClrHMin;
        StochSettings.ClrHMax = ClrHMax;
        StochSettings.ClrVIdle = ClrVIdle;
        chSysUnlock();
        PShell->Ok();
    }
*/

    else if(PCmd->NameIs("CLS")) {
        Leds.SetAll(clBlack);
        Leds.SetCurrentColors();
        PShell->Ok();
    }

    else PShell->CmdUnknown();
}
#endif

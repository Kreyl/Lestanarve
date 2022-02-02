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
#include <math.h>
#include "kl_fs_utils.h"
#include "usb_msd.h"

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(256000, CMD_UART_PARAMS);
CmdUart_t Uart{CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// ==== LEDs ====
#define SMOOTH_MIN_IDLE     270
#define SMOOTH_MAX_IDLE     405
#define SMOOTH_MIN_FAST     45
#define SMOOTH_MAX_FAST     90

LedSmooth_t Lumos { LED_PIN };
static const NeopixelParams_t NpxParams {NPX_SPI, NPX_DATA_PIN,
    NPX_DMA, NPX_DMA_MODE(NPX_DMA_REQ),
    NPX_LED_CNT, npxRGB
};
Neopixels_t Leds{&NpxParams};
void ProcessAcc();

// ==== USB & FileSys ====
FATFS FlashFS;
uint8_t LoadSettings();
bool UsbIsConnected = false;

// ==== Timers ====
static TmrKL_t TmrAcg {TIME_MS2I(11), evtIdAcc, tktPeriodic};
#endif

int main(void) {
#if 1 // ==== Init clock system ====
    Clk.SetVoltageRange(mvrHiPerf);
    Clk.SetupFlashLatency(80, mvrHiPerf);
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
    // SysClock 80MHz
    Clk.SetupPll(40, 2, 4);
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
    PinSetHi(NPX_PWR_PIN);
//    Leds.SetAll(clGreen);
//    Leds.SetCurrentColors();

    // Init filesystem
    FRESULT err;
    err = f_mount(&FlashFS, "", 0);
    if(err == FR_OK) {
        if(LoadSettings() == retvOk) Lumos.StartOrRestart(lsqLStart);
        else Lumos.StartOrRestart(lsqLError);
    }
    else Printf("FS error\r");

    Acg.Init();

    // Setup stoch settings
    StochSettings.DelayIdleMin = 9;
    StochSettings.DelayIdleMax = 18;
    StochSettings.DelayOnMin = 9;
    StochSettings.DelayOnMax = 18;
    StochSettings.SmoothMin = SMOOTH_MIN_IDLE;
    StochSettings.SmoothMax = SMOOTH_MAX_IDLE;
    StochSettings.ClrHMin = 120;
    StochSettings.ClrHMax = 270;
    StochSettings.ClrVIdle = 0;


    Eff::Init();
    Eff::Start();

    TmrAcg.StartOrRestart();

    UsbMsd.Init();
    SimpleSensors::Init();
//    Adc.Init();

    // ==== Time and timers ====
//    TmrEverySecond.StartOrRestart();

    // ==== Radio ====
//    if(Radio.Init() == retvOk) Led.StartOrRestart(lsqStart);
//    else Led.StartOrRestart(lsqFailure);
//    chThdSleepMilliseconds(1008);

    // Main cycle
    ITask();
}

__noreturn
void ITask() {
    while(true) {
        EvtMsg_t Msg = EvtQMain.Fetch(TIME_INFINITE);
        switch(Msg.ID) {
//            case evtIdEverySecond:
//                TimeS++;
//                ReadAndSetupMode();
//                break;

            case evtIdAcc:
                ProcessAcc();
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
//                if(Settings.Load() != retvOk) LedInd.StartOrRestart(lsqError); // XXX
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

}

uint8_t LoadSettings() {
    return retvOk;
}

#if 1 // ========================= Acg to Eff settings =========================
float Diff(float x0) {
    static float y1 = 0;
    static float x1 = 4300000;
    float y0 = x0 - x1 + 0.9 * y1;
    y1 = y0;
    x1 = x0;
    return y0;
}

template <uint32_t WindowSz>
class MAvg_t {
private:
    float IBuf[WindowSz], ISum;
public:
    float CalcNew(float NewVal) {
        ISum = ISum + NewVal - IBuf[WindowSz-1];
        // Shift buf
        for(int i=(WindowSz-1); i>0; i--) IBuf[i] = IBuf[i-1];
        IBuf[0] = NewVal;
        return ISum / WindowSz;
    }
};

template <uint32_t WindowSz>
class MMax_t {
private:
    uint32_t Cnt1 = 0, Cnt2 = WindowSz / 2;
    float Max1 = -INFINITY, Max2 = -INFINITY;
public:
    float CalcNew(float NewVal) {
        Cnt1++;
        Cnt2++;
        // Threat Max2
        if(Cnt2 >= WindowSz) {
            Max2 = NewVal;
            Cnt2 = 0;
        }
        else {
            if(NewVal > Max2) Max2 = NewVal;
        }
        // Threat Max1
        if(Cnt1 >= WindowSz) {
            Cnt1 = 0;
            Max1 = (NewVal > Max2)? NewVal : Max2;
        }
        else { if(NewVal > Max1) Max1 = NewVal; }
        return Max1;
    }
};

MAvg_t<8> MAvgAcc;
MMax_t<64> MMaxDelta;
MAvg_t<256> MAvgDelta;

#define DELTA_TOP   2000000

void ProcessAcc() {
    AccSpd_t AccSpd;
    chSysLock();
    AccSpd = Acg.AccSpd;
    chSysUnlock();
    float a = AccSpd.a[0] * AccSpd.a[0] + AccSpd.a[1] * AccSpd.a[1] + AccSpd.a[2] * AccSpd.a[2];
    a = Diff(a);
    float Avg = MAvgAcc.CalcNew(a);
    float Delta = abs(Avg);
    if(Delta > DELTA_TOP) Delta = DELTA_TOP;
//    Delta = MAvgDelta.CalcNew(Delta);
    float DeltaMax = MMaxDelta.CalcNew(Delta);
    float xval = MAvgDelta.CalcNew(DeltaMax);
    // Calculate new settings
    float fSmoothMin = abs(Proportion<float>(0, DELTA_TOP, SMOOTH_MIN_IDLE, SMOOTH_MIN_FAST, xval));
    float fSmoothMax = abs(Proportion<float>(0, DELTA_TOP, SMOOTH_MAX_IDLE, SMOOTH_MAX_FAST, xval));
    int32_t SmoothMin = (int32_t) fSmoothMin;
    int32_t SmoothMax = (int32_t) fSmoothMax;

    chSysLock();
    StochSettings.SmoothMin = SmoothMin;
    StochSettings.SmoothMax = SmoothMax;
    chSysUnlock();
//    Printf("%d\t%d\r", SmoothMin, SmoothMax);
}
#endif

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

//    else if(PCmd->NameIs("Radius")) {
//        uint32_t Delay;
//        Color_t Clr;
//        if(PCmd->GetNext<uint32_t>(&Delay) != retvOk) { PShell->BadParam(); return; }
//        if(PCmd->GetClrRGB(&Clr) != retvOk) { PShell->BadParam(); return; }
//        Eff::StartRadius(Delay, Clr);
//        PShell->Ok();
//    }

    else if(PCmd->NameIs("CLS")) {
        Leds.SetAll(clBlack);
        Leds.SetCurrentColors();
        PShell->Ok();
    }

    else PShell->CmdUnknown();
}
#endif

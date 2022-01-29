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

#if 1 // ======================== Variables and defines ========================
// Forever
EvtMsgQ_t<EvtMsg_t, MAIN_EVT_Q_LEN> EvtQMain;
static const UartParams_t CmdUartParams(256000, CMD_UART_PARAMS);
CmdUart_t Uart{CmdUartParams};
static void ITask();
static void OnCmd(Shell_t *PShell);

// ==== Periphery ====
LedSmooth_t Lumos { LED_PIN };
static const NeopixelParams_t NpxParams {NPX_SPI, NPX_DATA_PIN,
    NPX_DMA, NPX_DMA_MODE(NPX_DMA_REQ),
    NPX_LED_CNT, npxRGB
};
Neopixels_t Leds{&NpxParams};

// ==== Timers ====
//static TmrKL_t TmrEverySecond {MS2ST(1000), evtIdEverySecond, tktPeriodic};
//static int32_t TimeS;
#endif

int main(void) {
#if 1 // ==== Init clock system ====
    Clk.SetVoltageRange(mvrHiPerf);
    Clk.SetupFlashLatency(20, mvrHiPerf);
    Clk.EnablePrefetch();
    if(Clk.EnableHSE() == retvOk) {
        Clk.SetupPllSrc(pllsrcHse);
        Clk.SetupM(3);
    }
    else { // PLL fed by MSI
        Clk.SetupPllSrc(pllsrcMsi);
        Clk.SetupM(3);
    }
    Clk.SetupPll(20, 4, 4);
    Clk.SetupBusDividers(ahbDiv1, apbDiv1, apbDiv1);
    if(Clk.EnablePLL() == retvOk) {
        Clk.EnablePllROut();
        Clk.SwitchToPLL();
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
    Lumos.StartOrRestart(lsqLStart);

    // ==== Leds ====
    Leds.Init();
    // LED pwr pin
    PinSetupOut(NPX_PWR_PIN, omPushPull);
    PinSetHi(NPX_PWR_PIN);
    Leds.SetAll(clGreen);
    Leds.SetCurrentColors();

//    Acg.Init();

    Eff::Init();
    Eff::Start();

//    SimpleSensors::Init();
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

            case evtIdShellCmdRcvd:
                while(((CmdUart_t*)Msg.Ptr)->TryParseRxBuff() == retvOk) OnCmd((Shell_t*)((CmdUart_t*)Msg.Ptr));
                break;
            default: Printf("Unhandled Msg %u\r", Msg.ID); break;
        } // Switch
    } // while true
} // ITask()

void ProcessCharging(PinSnsState_t *PState, uint32_t Len) {

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

/*
 * LedEffects.cpp
 *
 *  Created on: 29 янв. 2022 г.
 *      Author: layst
 */

#include "LedEffects.h"
#include "ws2812b.h"
#include "board.h"
#include "Settings.h"

extern Neopixels_t Leds;

#define LED_CNT     NPX_LED_CNT

#define RMIN        0L
#define RMAX        96L
#define AMIN        -2L
#define AMAX        183L

static int ITable[LED_CNT][2] = {
    {83, 179}, //0
    {72, 182}, //1
    {53, 166}, //2
    {34, 163}, //3
    {15, 159}, //4
    {0, 0}, //5
    {75, 166}, //6
    {58, 144}, //7
    {33, 133}, //8
    {20, 126}, //9
    {43, 122}, //10
    {45, 108}, //11
    {67, 94}, //12
    {95, 86}, //13
    {45, 86}, //14
    {18, 89}, //15
    {69, 79}, //16
    {45, 66}, //17
    {22, 53}, //18
    {45, 52}, //19
    {32, 44}, //20
    {62, 27}, //21
    {77, 10}, //22
    {85, -2}, //23
    {72, -2}, //24
    {52, 11}, //25
    {30, 17}, //26
    {14, 24}, //27
};

class Lipte_t {
private:
    enum {staIdle, staFadeIn, staOn, staFadeOut} IState = staFadeIn;
    uint32_t DurationOff, DurationOn;
    uint32_t Smooth;
    int32_t ITmr;
    // Led settings
    ColorHSV_t IClrPrev, IClrTarget{0, 100, 100};
    uint32_t IClrDelay;
public:
    ColorHSV_t IClrCurr;
    void Generate() {
        Settings.Generate(&DurationOff, &DurationOn, &Smooth, &IClrTarget.H);
        IClrCurr = IClrTarget;
        IClrCurr.V = Settings.ClrVIdle;
        IClrPrev = IClrCurr;
        ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
    }

    bool OnTickHasChanged() {
//        Printf("%d\r", ITmr);
        ITmr--;
        if(ITmr <= 0) {
            switch(IState) {
                case staIdle:
                    IState = staFadeIn;
                    Generate();
                    break;

                case staFadeIn:
                    // Check if FadeIn done
                    if(IClrCurr == IClrTarget) {
                        IState = staOn;
                        ITmr = DurationOn;
                    }
                    // Not done
                    else {
                        IClrCurr.V++;
                        ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
                    }
                    break;

                case staOn:
                    IState = staFadeOut;
                    ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
                    break;

                case staFadeOut:
                    // Check if FadeOut done
                    if(IClrCurr.V == Settings.ClrVIdle) {
                        IState = staIdle;
                        ITmr = DurationOff;
                    }
                    // Not done
                    else {
                        IClrCurr.V--;
                        ITmr = ClrCalcDelay(IClrCurr.V, Smooth);
                    }
                    break;
            } // switch
        } // if tmr <= 0
        bool HasChanged = !(IClrCurr == IClrPrev);
        IClrPrev = IClrCurr;
        return HasChanged;
    }
};

Lipte_t Lipti[LED_CNT];

static THD_WORKING_AREA(waEffThread, 256);
__noreturn
static void EffThread(void *arg) {
    chRegSetThreadName("Eff");
    while(true) {
        chThdSleepMilliseconds(1);
        bool HasChanged = false;
        for(int i=0; i<LED_CNT; i++) {
            if(Lipti[i].OnTickHasChanged()) {
                Leds.ClrBuf[i] = Lipti[i].IClrCurr.ToRGB();
                HasChanged = true;
            }
        }
        if(HasChanged) Leds.SetCurrentColors();
    } // while true
}


namespace Eff {

void Init() {
    for(auto &Lipte : Lipti) Lipte.Generate();
    chThdCreateStatic(waEffThread, sizeof(waEffThread), HIGHPRIO, (tfunc_t)EffThread, NULL);
}

void Set(int R, int A, Color_t AClr) {
    for(int i=0; i<LED_CNT; i++) {
        if((R == ITable[i][0]) and (A == ITable[i][1])) Leds.ClrBuf[i] = AClr;
    }
}

}

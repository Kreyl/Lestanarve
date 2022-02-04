/*
 * Motion.cpp
 *
 *  Created on: 4 февр. 2022 г.
 *      Author: layst
 */

#include "Settings.h"
#include <math.h>
#include "acg_lsm6ds3.h"

// ==== Differentiator ====
float Diff(float x0) {
    static float y1 = 0;
    static float x1 = 4300000;
    float y0 = x0 - x1 + 0.9 * y1;
    y1 = y0;
    x1 = x0;
    return y0;
}

// ==== Moving Average ====
class MAvg_t {
private:
    uint32_t WindowSz;
    float IBuf[MAVG_WINDOW_MAX], ISum;
public:
    void Init(uint32_t AWindowSz) {
        WindowSz = AWindowSz;
        ISum = 0;
    }

    float CalcNew(float NewVal) {
        ISum = ISum + NewVal - IBuf[WindowSz-1];
        // Shift buf
        for(int i=(WindowSz-1); i>0; i--) IBuf[i] = IBuf[i-1];
        IBuf[0] = NewVal;
        return ISum / WindowSz;
    }
};

MAvg_t MAvgAcc;
MAvg_t MAvgDelta;

// ==== Moving Maximum ====
class MMax_t {
private:
    uint32_t Cnt1 = 0, Cnt2 = 0;
    float Max1 = -INFINITY, Max2 = -INFINITY;
public:
    void Init() {
        Cnt1 = 0;
        Cnt2 = Settings.MMaxWindow / 2;
    }

    float CalcNew(float NewVal) {
        Cnt1++;
        Cnt2++;
        // Process Max2
        if(Cnt2 >= Settings.MMaxWindow) {
            Max2 = NewVal;
            Cnt2 = 0;
        }
        else {
            if(NewVal > Max2) Max2 = NewVal;
        }
        // Process Max1
        if(Cnt1 >= Settings.MMaxWindow) {
            Cnt1 = 0;
            Max1 = (NewVal > Max2)? NewVal : Max2;
        }
        else {
            if(NewVal > Max1) Max1 = NewVal;
        }
        return Max1;
    }
} MMaxDelta;


namespace Motion {

void Init() {
    MAvgAcc.Init(8);
    MAvgDelta.Init(Settings.MAvgWindow);
    MMaxDelta.Init();
}

void Update() {
    AccSpd_t AccSpd;
    chSysLock();
    AccSpd = Acg.AccSpd;
    chSysUnlock();
    // Calc scalar value of acceleration vector
    float a = AccSpd.a[0] * AccSpd.a[0] + AccSpd.a[1] * AccSpd.a[1] + AccSpd.a[2] * AccSpd.a[2];
    // Remove constant component
    a = Diff(a);
    // Smooth it with moving average of 8
    float Avg = MAvgAcc.CalcNew(a);
    // Rectify it to get delta
    float Delta = abs(Avg);
    // Limit delta top
    if(Delta > Settings.TopAcceleration) Delta = Settings.TopAcceleration;
    // Get moving max of delta
    float DeltaMax = MMaxDelta.CalcNew(Delta);
    // Smooth fronts of max with MAVG
    float xval = MAvgDelta.CalcNew(DeltaMax);
    // Calculate new flaring settings
    float fSmoothMin = abs(Proportion<float>(0, Settings.TopAcceleration, Settings.Smooth.MinSlow, Settings.Smooth.MinFast, xval));
    float fSmoothMax = abs(Proportion<float>(0, Settings.TopAcceleration, Settings.Smooth.MaxSlow, Settings.Smooth.MaxFast, xval));
    int32_t SmoothMin = (int32_t) fSmoothMin;
    int32_t SmoothMax = (int32_t) fSmoothMax;
    // Set new settings
    chSysLock();
    Settings.Smooth.MinCurr = SmoothMin;
    Settings.Smooth.MaxCurr = SmoothMax;
    chSysUnlock();
//    Printf("%d\t%d\r", SmoothMin, SmoothMax);
}

}


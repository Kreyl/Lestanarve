/*
 * LedEffects.h
 *
 *  Created on: 29 янв. 2022 г.
 *      Author: layst
 */

#pragma once

#include "color.h"

class StochSettings_t {
public:
    uint32_t DelayIdleMin, DelayIdleMax;
    uint32_t DelayOnMin, DelayOnMax;
    uint32_t SmoothMin, SmoothMax;
    uint16_t ClrHMin, ClrHMax;
    uint8_t ClrVIdle;
    void Generate(uint32_t *DelayIdle, uint32_t *DelayOn, uint32_t *Smooth, uint16_t *ClrH) {
        *DelayIdle = Random::Generate(DelayIdleMin, DelayIdleMax);
        *DelayOn   = Random::Generate(DelayOnMin, DelayOnMax);
        *Smooth    = Random::Generate(SmoothMin, SmoothMax);
        *ClrH      = Random::Generate(ClrHMin, ClrHMax);
    }
};

extern StochSettings_t StochSettings;

namespace Eff {
void Init();

void Start();

void Set(int R, int A, Color_t AClr);

//void StartRadius(uint32_t Delay_ms, Color_t AClr);

}

/*
 * LedEffects.h
 *
 *  Created on: 29 янв. 2022 г.
 *      Author: layst
 */

#pragma once

#include "color.h"

namespace Eff {
void Init();

void Start();

void Set(int R, int A, Color_t AClr);

//void StartRadius(uint32_t Delay_ms, Color_t AClr);

}

/*
 * Settings.cpp
 *
 *  Created on: 3 февр. 2022 г.
 *      Author: layst
 */

#include "Settings.h"
#include "kl_lib.h"
#include "uart2.h"

Settings_t Settings;

void Settings_t::Generate(uint32_t *DurationOff, uint32_t *DurationOn, uint32_t *Smooth, uint16_t *ClrH) {
    *DurationOff = Random::Generate(Duration.OffMin, Duration.OffMax);
    *DurationOn  = Random::Generate(Duration.OnMin, Duration.OnMax);
    *Smooth      = Random::Generate(SmoothMin, SmoothMax);
    *ClrH        = Random::Generate(ClrHMin, ClrHMax);
}

void Settings_t::Load() {



}

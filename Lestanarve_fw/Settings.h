/*
 * Settings.h
 *
 *  Created on: 3 февр. 2022 г.
 *      Author: layst
 */

#pragma once

#include <inttypes.h>

class Settings_t {
public:
    struct {
        uint32_t OffMin = 9, OffMax = 18;
        uint32_t OnMin = 9, OnMax = 18;
    } Duration;
    struct {
        uint32_t SlowMin = 270, SlowMax = 405;
        uint32_t FastMin = 45, FastMax = 90;
    } Smooth;
    uint16_t ClrHMin = 120, ClrHMax = 270;
    uint8_t ClrVIdle = 0;
    void Generate(uint32_t *DurationOff, uint32_t *DurationOn, uint32_t *Smooth, uint16_t *ClrH);
    void Load();
};

extern Settings_t Settings;

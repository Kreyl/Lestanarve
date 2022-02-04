/*
 * Settings.h
 *
 *  Created on: 3 февр. 2022 г.
 *      Author: layst
 */

#pragma once

#include <inttypes.h>

#define SETTINGS_FILENAME   "Settings.ini"
#define MAVG_WINDOW_MAX     256
#define MMAX_WINDOW_MAX     512

class Settings_t {
public:
    // Flaring
    struct {
        uint32_t MinOff = 9, MaxOff = 18;
        uint32_t MinOn = 9, MaxOn = 18;
    } Duration;
    struct {
        uint32_t MinSlow = 270, MaxSlow = 405;
        uint32_t MinFast = 45,  MaxFast = 90;
        uint32_t MinCurr = 270, MaxCurr = 405;
    } Smooth;
    uint16_t ClrHMin = 120, ClrHMax = 270;
    uint8_t ClrVIdle = 0;
    // Motion
    uint32_t TopAcceleration = 2000000;
    uint32_t MMaxWindow;
    uint32_t MAvgWindow;

    void Generate(uint32_t *DurationOff, uint32_t *DurationOn, uint32_t *Smooth, uint16_t *ClrH);
    void Load();
    bool LoadSuccessful = false;
};

extern Settings_t Settings;

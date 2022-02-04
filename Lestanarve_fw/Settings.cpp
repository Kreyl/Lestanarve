/*
 * Settings.cpp
 *
 *  Created on: 3 февр. 2022 г.
 *      Author: layst
 */

#include "Settings.h"
#include "kl_lib.h"
#include "uart2.h"
#include "kl_fs_utils.h"

Settings_t Settings;

void Settings_t::Generate(uint32_t *PDurationOff, uint32_t *PDurationOn, uint32_t *PSmooth, uint16_t *PClrH) {
    *PDurationOff = Random::Generate(Duration.MinOff, Duration.MaxOff);
    *PDurationOn  = Random::Generate(Duration.MinOn, Duration.MaxOn);
    *PSmooth      = Random::Generate(Smooth.MinCurr, Smooth.MaxCurr);
    *PClrH        = Random::Generate(ClrHMin, ClrHMax);
}

template <typename T>
void LoadParam(const char *ASection, const char *AKey, T *POutput) {
    if(ini::Read<T>(SETTINGS_FILENAME, ASection, AKey, POutput) != retvOk) {
        Settings.LoadSuccessful = false;
        Printf("%S read fail\r", AKey);
    }
}

template <typename T>
void CheckMinMax(T *PMin, T *PMax, T DefaultMin, T DefaultMax, const char* ParamName) {
    if(*PMin > *PMax) {
        Settings.LoadSuccessful = false;
        *PMin = DefaultMin;
        *PMax = DefaultMax;
        Printf("%S Bad Values\r", ParamName);
    }
}

template <typename T>
void CheckValueMax(T *PParam, T MaxValue, T DefaultValue, const char* ParamName) {
    if(*PParam > MaxValue) {
        Settings.LoadSuccessful = false;
        *PParam = DefaultValue;
        Printf("%S Bad Value\r", ParamName);
    }
}

template <typename T>
void CheckValueMinMax(T *PParam, T MinValue, T MaxValue, T DefaultValue, const char* ParamName) {
    if(*PParam > MaxValue or *PParam < MinValue) {
        Settings.LoadSuccessful = false;
        *PParam = DefaultValue;
        Printf("%S Bad Values\r", ParamName);
    }
}

void Settings_t::Load() {
    LoadSuccessful = true;
    // Flaring
    LoadParam<uint32_t>("Flaring", "DurationOffMin", &Duration.MinOff);
    LoadParam<uint32_t>("Flaring", "DurationOffMax", &Duration.MaxOff);
    LoadParam<uint32_t>("Flaring", "DurationOnMin", &Duration.MinOn);
    LoadParam<uint32_t>("Flaring", "DurationOnMax", &Duration.MaxOn);
    LoadParam<uint32_t>("Flaring", "SmoothMinSlow", &Smooth.MinSlow);
    LoadParam<uint32_t>("Flaring", "SmoothMaxSlow", &Smooth.MaxSlow);
    LoadParam<uint32_t>("Flaring", "SmoothMinFast", &Smooth.MinFast);
    LoadParam<uint32_t>("Flaring", "SmoothMaxFast", &Smooth.MaxFast);
    LoadParam<uint16_t>("Flaring", "ClrHMin", &ClrHMin);
    LoadParam<uint16_t>("Flaring", "ClrHMax", &ClrHMax);
    LoadParam<uint8_t>("Flaring", "ClrVIdle", &ClrVIdle);
    // Motion
    LoadParam<uint32_t>("Motion", "TopAcceleration", &TopAcceleration);
    LoadParam<uint32_t>("Motion", "MMaxWindow", &MMaxWindow);
    LoadParam<uint32_t>("Motion", "MAvgWindow", &MAvgWindow);

    // Check
    CheckMinMax<uint32_t>(&Duration.MinOff, &Duration.MaxOff, 9, 18, "Duration Off");
    CheckMinMax<uint32_t>(&Duration.MinOn, &Duration.MaxOn, 9, 18, "Duration On");
    CheckMinMax<uint32_t>(&Smooth.MinSlow, &Smooth.MaxSlow, 270, 405, "Smooth Slow");
    CheckMinMax<uint32_t>(&Smooth.MinFast, &Smooth.MaxFast, 270, 405, "Smooth Fast");
    CheckMinMax<uint16_t>(&ClrHMin, &ClrHMax, 120, 270, "Color");
    CheckValueMax<uint16_t>(&ClrHMin, 360, 120, "ClrHMin");
    CheckValueMax<uint16_t>(&ClrHMax, 360, 270, "ClrHMax");
    CheckValueMax<uint8_t>(&ClrVIdle, 100, 0, "ClrVIdle");
    CheckValueMax<uint32_t>(&TopAcceleration, 0x7FFFFFFE, 2000000, "TopAcceleration");
    CheckValueMinMax<uint32_t>(&MMaxWindow, 4, MMAX_WINDOW_MAX, 64, "MMaxWindow");
    CheckValueMinMax<uint32_t>(&MAvgWindow, 2, MAVG_WINDOW_MAX, 256, "MAvgWindow");

    // Report
    if(LoadSuccessful) Printf("Settings load ok\r");
    else Printf("Settings load fail\r");
}

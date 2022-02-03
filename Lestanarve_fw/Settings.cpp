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

    // Report
    if(LoadSuccessful) Printf("Settings load ok\r");
    else Printf("Settings load fail\r");
}

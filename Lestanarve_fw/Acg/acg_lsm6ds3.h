/*
 * acg_lsm6ds3.h
 *
 *  Created on: 2 ���. 2017 �.
 *      Author: Kreyl
 */

#pragma once

#include "kl_lib.h"
#include "shell.h"

union AccSpd_t {
    uint32_t DWord32[4];
    struct {
        uint8_t __dummy;    // DMA will write here when transmitting addr
        int16_t g[3];
        int16_t a[3];
    } __attribute__ ((__packed__));
    void Print() { Printf("%d %d %d; %d %d %d\r", a[0],a[1],a[2], g[0],g[1],g[2]); }
    AccSpd_t& operator = (const AccSpd_t &Right) {
        DWord32[0] = Right.DWord32[0];
        DWord32[1] = Right.DWord32[1];
        DWord32[2] = Right.DWord32[2];
        DWord32[3] = Right.DWord32[3];
        return *this;
    }
};

void AcgIrqHandler();

class Acg_t {
private:
    const PinIrq_t IIrq;
    void IWriteReg(uint8_t AAddr, uint8_t AValue);
    void IReadReg(uint8_t AAddr, uint8_t *PValue);
public:
    AccSpd_t IAccSpd, AccSpd;
    void Init();
    void Shutdown();
    void Task();
    uint8_t GetSta();
    const stm32_dma_stream_t *PDmaTx = nullptr;
    const stm32_dma_stream_t *PDmaRx = nullptr;
    Acg_t() : IIrq(ACG_IRQ_PIN, pudPullDown, AcgIrqHandler), IAccSpd() {}
};

extern Acg_t Acg;

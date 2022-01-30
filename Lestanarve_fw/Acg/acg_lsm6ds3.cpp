/*
 * acg_lsm6ds3.cpp
 *
 *  Created on: 2 ���. 2017 �.
 *      Author: Kreyl
 */

#include "acg_lsm6ds3.h"
#include "MsgQ.h"
#include "shell.h"
#include "acg_lsm6ds3_defins.h"
#include "board.h"

#include "radio_lvl1.h"

#define LOG_TO_FILE             FALSE

#define READ_FROM_FILE          FALSE// TRUE

// SPI clock is up to 10MHz (ds p.26)
#define ACG_MAX_BAUDRATE_HZ     10000000

Acg_t Acg;
static Spi_t ISpi {ACG_SPI};
//static thread_reference_t ThdRef = nullptr;

static inline void ICsHi() { PinSetHi(ACG_CS_PIN); }
static inline void ICsLo() { PinSetLo(ACG_CS_PIN); }

// DMA reception complete
extern "C"
void AcgDmaRxCompIrq(void *p, uint32_t flags) {
    ICsHi();
    // Disable DMA
    dmaStreamDisable(Acg.PDmaTx);
    dmaStreamDisable(Acg.PDmaRx);
    Acg.AccSpd = Acg.IAccSpd;
//    chSysLockFromISR();
//    chThdResumeI(&ThdRef, MSG_OK);
//    chSysUnlockFromISR();
}

// Thread
//static THD_WORKING_AREA(waAcgThread, 512);
//__noreturn
//static void AcgThread(void *arg) {
//    chRegSetThreadName("Acg");
//    Acg.Task();
//}

//__noreturn
//void Acg_t::Task() {
//#if READ_FROM_FILE
//    bool Done = false;
//#endif
//    while(true) {
//        chSysLock();
//        chThdSuspendS(&ThdRef); // Wait IRQ
//        chSysUnlock();

//        rPkt_t IPkt;
//        IPkt.Time = chVTGetSystemTimeX() / 10;
//        IPkt.Btn = PinIsHi(GPIOA, 0);
//
//        IPkt.acc[0] = AccSpd.a[0];
//        IPkt.acc[1] = AccSpd.a[1];
//        IPkt.acc[2] = AccSpd.a[2];
//
//        IPkt.gyro[0] = AccSpd.g[0];
//        IPkt.gyro[1] = AccSpd.g[1];
//        IPkt.gyro[2] = AccSpd.g[2];

//        chSysLock();
//        Radio.TxBuf.PutI(IPkt);
//        chSysUnlock();

//        AccSpd.Print();

        // Detect motion
//        SaberMotn.Update(AccSpd.a, AccSpd.g);
//    }
//}

void Acg_t::Init() {
#if 1 // ==== GPIO ====
    PinSetupOut(ACG_CS_PIN, omPushPull);
    PinSetupAlterFunc(ACG_SCK_PIN);
    PinSetupAlterFunc(ACG_MISO_PIN);
    PinSetupAlterFunc(ACG_MOSI_PIN);
    // Power
    PinSetupOut(ACG_PWR_PIN, omPushPull);
    PinSetHi(ACG_PWR_PIN);
    ICsHi();
    IIrq.Init(ttRising);
    chThdSleepMilliseconds(18);
#endif
#if 1 // ==== SPI ====    MSB first, master, ClkIdleHigh, FirstEdge
    ISpi.Setup(boMSB, cpolIdleHigh, cphaSecondEdge, ACG_MAX_BAUDRATE_HZ);
    ISpi.EnableRxDma();
    ISpi.EnableTxDma();
    ISpi.Enable();
#endif
#if 1 // ==== Registers ====
    // Reset
    IWriteReg(0x12, 0x81);
    chThdSleepMilliseconds(11);
    uint8_t b;
    IReadReg(0x0F, &b);
    if(b != 0x69) {
        Printf("Wrong Acg WhoAmI: %X\r", b);
        return;
    }

    // FIFO
    IWriteReg(0x06, 6); // FIFO CTRL1: FIFO thr=6*2=12 (2 bytes per axis * 6 axis)
    IWriteReg(0x07, 0x00); // FIFO CTRL2: pedo dis, no temp, FIFO thr MSB = 0
    IWriteReg(0x08, (0b001 << 3) | (0b001)); // FIFO CTRL3: gyro and acc no decimation, both in fifo
    IWriteReg(0x09, 0x00); // FIFO CTRL4: no stop on thr, not only MSB, no fourth and third dataset
    IWriteReg(0x0A, (0b0110 << 3) | 0b110); // FIFO CTRL5: FIFO ODR = 416, FIFO mode = ovewrite old v

    IWriteReg(0x0B, 0x00); // DRDY_PULSE_CFG_G: DataReady latched mode, Wrist tilt INT2 dis
    IWriteReg(0x0D, 0x08); // INT1_CTRL: irq on FIFO threshold

    // CTRL
//    b = LSM6DS3_ACC_GYRO_BW_XL_400Hz | LSM6DS3_ACC_GYRO_FS_XL_8g | LSM6DS3_ACC_GYRO_ODR_XL_104Hz;
    b = LSM6DS3_ACC_GYRO_BW_XL_400Hz | LSM6DS3_ACC_GYRO_FS_XL_16g | LSM6DS3_ACC_GYRO_ODR_XL_104Hz;
    IWriteReg(0x10, b);
    b = LSM6DS3_ACC_GYRO_FS_G_2000dps | LSM6DS3_ACC_GYRO_ODR_G_104Hz;
    IWriteReg(0x11, b);
    IWriteReg(0x12, 0x44); // CTRL3_c: no reboot, block update, irq act hi & push-pull, spi 4w, reg addr inc, LSB first, no rst
    IWriteReg(0x13, 0x84); // CTRL4_c: DEN, no g sleep, i2c dis, no g LPF
    IWriteReg(0x14, 0x00); // CTRL5_c: no rounding, no self-test
    IWriteReg(0x15, 0x00); // CTRL6_c: no DEN, acc hi-perf en
    IWriteReg(0x16, 0x00); // CTRL7_G: g hi-perf en, g HPF dis, rounding dis
    IWriteReg(0x17, 0x00); // CTRL8_XL: no LPF2, no HP
    IWriteReg(0x1A, 0x80); // MASTER_CONFIG: DRDY on INT1, other dis
#endif
#if 1 // ==== DMA ====
    // Tx
    PDmaTx = dmaStreamAlloc(ACG_DMA_TX, IRQ_PRIO_MEDIUM, nullptr, nullptr);
    dmaStreamSetPeripheral(PDmaTx, &ACG_SPI->DR);
    // Rx
    PDmaRx = dmaStreamAlloc(ACG_DMA_RX, IRQ_PRIO_MEDIUM, AcgDmaRxCompIrq, nullptr);
    dmaStreamSetPeripheral(PDmaRx, &ACG_SPI->DR);
#endif
    // Thread
//    chThdCreateStatic(waAcgThread, sizeof(waAcgThread), NORMALPRIO, (tfunc_t)AcgThread, NULL);
    IIrq.EnableIrq(IRQ_PRIO_MEDIUM);
    Printf("IMU Init Done\r", b);
#if LOG_TO_FILE
    TryOpenFileRewrite(IFILENAME, &ImuFile);
#endif
#if READ_FROM_FILE
    Printf("Opening %S\r", IFILENAME);
    csv::OpenFile(IFILENAME);
#endif
}

void Acg_t::Shutdown() {
}

uint8_t Acg_t::GetSta() {
    uint8_t b;
    IReadReg(0x1E, &b);
    return b;
}

#if 1 // =========================== Low level =================================
void Acg_t::IWriteReg(uint8_t AAddr, uint8_t AValue) {
    ICsLo();
    ISpi.ReadWriteByte(AAddr);
    ISpi.ReadWriteByte(AValue);
    ICsHi();
}

void Acg_t::IReadReg(uint8_t AAddr, uint8_t *PValue) {
    ICsLo();
    ISpi.ReadWriteByte(AAddr | 0x80);   // Add "Read" bit
    *PValue = ISpi.ReadWriteByte(0);
    ICsHi();
}

const uint8_t SAddr = 0x3E | 0x80; // Add "Read" bit

void AcgIrqHandler() {
    ICsLo();
    // RX
    dmaStreamSetMemory0(Acg.PDmaRx, &Acg.IAccSpd);
    dmaStreamSetTransactionSize(Acg.PDmaRx, sizeof(AccSpd_t));
    dmaStreamSetMode(Acg.PDmaRx, ACG_DMA_RX_MODE);
    dmaStreamEnable(Acg.PDmaRx);
    // TX
    dmaStreamSetMemory0(Acg.PDmaTx, &SAddr);
    dmaStreamSetTransactionSize(Acg.PDmaTx, sizeof(AccSpd_t));
    dmaStreamSetMode(Acg.PDmaTx, ACG_DMA_TX_MODE);
    dmaStreamEnable(Acg.PDmaTx);
}
#endif

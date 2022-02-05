#pragma once

// ==== General ====
#define APP_NAME            "Lestanarve"

#ifndef TRUE
#define TRUE 1
#endif

// Version of PCB
#define PCB_VER                 1

// MCU type as defined in the ST header.
#define STM32L433xx

// Freq of external crystal if any. Leave it here even if not used.
#define CRYSTAL_FREQ_HZ         12000000

// OS timer settings
#define STM32_ST_IRQ_PRIORITY   2
#define STM32_ST_USE_TIMER      2
#define SYS_TIM_CLK             (Clk.APB1FreqHz)    // Timer 5 is clocked by APB1

//  Periphery
#define I2C1_ENABLED            FALSE
#define I2C2_ENABLED            FALSE
#define I2C3_ENABLED            FALSE
#define SIMPLESENSORS_ENABLED   TRUE
#define BUTTONS_ENABLED         FALSE

#define ADC_REQUIRED            TRUE
#define STM32_DMA_REQUIRED      TRUE    // Leave this macro name for OS

#if 1 // ========================== GPIO =======================================
// EXTI
#define INDIVIDUAL_EXTI_IRQ_REQUIRED    FALSE

// Buttons
#define BTN1_PIN        GPIOA, 0

// Battery
#define IS_CHARGING     GPIOA, 0, pudPullUp
#define BAT_MEAS_EN     GPIOB, 0
#define BAT_ADC_PIN     GPIOB, 1, 16 // PB1, ADC channel 16

// Light
#define LED_PIN         { GPIOB, 8, TIM16, 1, invNotInverted, omPushPull, 255 }
// Npx LEDs
#define NPX_LED_CNT     28
#define NPX_SPI         SPI3
#define NPX_DATA_PIN    GPIOB, 5, AF6
#define NPX_PWR_PIN     GPIOB, 6

// USB
#define USB_DM          GPIOA, 11
#define USB_DP          GPIOA, 12
#define USB_AF          AF10
#define USB_DETECT_PIN  GPIOA, 2

// Radio: SPI, PGpio, Sck, Miso, Mosi, Cs, Gdo0
#define CC_Setup0       SPI1, GPIOA, 5,6,7, GPIOC,15, GPIOA,4

// UART
#define UART_GPIO       GPIOA
#define UART_TX_PIN     9
#define UART_RX_PIN     10

// Acc
#define ACG_IRQ_PIN     GPIOC, 13
#define ACG_CS_PIN      GPIOB, 12
#define ACG_SCK_PIN     GPIOB, 13, omPushPull, pudNone, AF5
#define ACG_MISO_PIN    GPIOB, 14, omPushPull, pudNone, AF5
#define ACG_MOSI_PIN    GPIOB, 15, omPushPull, pudNone, AF5
#define ACG_PWR_PIN     GPIOB, 11
#define ACG_SPI         SPI2

// I2C
#define I2C1_GPIO       GPIOB
#define I2C1_SCL        8
#define I2C1_SDA        9
// I2C Alternate Function
#define I2C_AF          AF4

#endif // GPIO

#if 1 // =========================== I2C =======================================
// i2cclkPCLK1, i2cclkSYSCLK, i2cclkHSI
#define I2C_CLK_SRC     i2cclkHSI
#define I2C_BAUDRATE_HZ 400000
#endif

#if ADC_REQUIRED // ======================= Inner ADC ==========================
// Clock divider: clock is generated from the APB2
#define ADC_CLK_DIVIDER		adcDiv2

// ADC channels
#define ADC_BATTERY_CHNL 	14
// ADC_VREFINT_CHNL
#define ADC_CHANNELS        { ADC_BATTERY_CHNL, ADC_VREFINT_CHNL }
#define ADC_CHANNEL_CNT     2   // Do not use countof(AdcChannels) as preprocessor does not know what is countof => cannot check
#define ADC_SAMPLE_TIME     ast24d5Cycles
#define ADC_OVERSAMPLING_RATIO  64   // 1 (no oversampling), 2, 4, 8, 16, 32, 64, 128, 256
#endif

#if 1 // =========================== DMA =======================================
// ==== Uart ====
// Remap is made automatically if required
#define UART_DMA_TX     STM32_DMA_STREAM_ID(2, 6)
#define UART_DMA_RX     STM32_DMA_STREAM_ID(2, 7)
#define UART_DMA_CHNL   2
#define UART_DMA_TX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_LOW | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_M2P | STM32_DMA_CR_TCIE)
#define UART_DMA_RX_MODE(Chnl) (STM32_DMA_CR_CHSEL(Chnl) | DMA_PRIORITY_MEDIUM | STM32_DMA_CR_MSIZE_BYTE | STM32_DMA_CR_PSIZE_BYTE | STM32_DMA_CR_MINC | STM32_DMA_CR_DIR_P2M | STM32_DMA_CR_CIRC)

// ==== Npx ====
#define NPX_DMA         STM32_DMA_STREAM_ID(2, 2)  // SPI3 TX
#define NPX_DMA_REQ     3

// ==== ACG ====
#define ACG_DMA_TX      STM32_DMA_STREAM_ID(1, 5)
#define ACG_DMA_RX      STM32_DMA_STREAM_ID(1, 4)
#define ACG_DMA_CHNL    1

// ==== I2C ====
#define I2C1_DMA_TX     STM32_DMA_STREAM_ID(1, 6)
#define I2C1_DMA_RX     STM32_DMA_STREAM_ID(1, 7)
#define I2C1_DMA_CHNL   3

#if ADC_REQUIRED
#define ADC_DMA         STM32_DMA_STREAM_ID(1, 1)
#define ADC_DMA_MODE    STM32_DMA_CR_CHSEL(0) |   /* DMA1 Stream1 Channel0 */ \
                        DMA_PRIORITY_LOW | \
                        STM32_DMA_CR_MSIZE_HWORD | \
                        STM32_DMA_CR_PSIZE_HWORD | \
                        STM32_DMA_CR_MINC |       /* Memory pointer increase */ \
                        STM32_DMA_CR_DIR_P2M |    /* Direction is peripheral to memory */ \
                        STM32_DMA_CR_TCIE         /* Enable Transmission Complete IRQ */
#endif // ADC

#endif // DMA

#if 1 // ========================== USART ======================================
#define PRINTF_FLOAT_EN TRUE
#define UART_TXBUF_SZ   2048
#define UART_RXBUF_SZ   128
#define CMD_BUF_SZ      64

#define CMD_UART        USART1

#define CMD_UART_PARAMS \
    CMD_UART, UART_GPIO, UART_TX_PIN, UART_GPIO, UART_RX_PIN, \
    UART_DMA_TX, UART_DMA_RX, UART_DMA_TX_MODE(UART_DMA_CHNL), UART_DMA_RX_MODE(UART_DMA_CHNL), \
    uartclkHSI // Use independent clock

#endif

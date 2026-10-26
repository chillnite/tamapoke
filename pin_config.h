#pragma once

// Placa de destino. Por defecto la ESP32-S3-Touch-AMOLED-1.75C; para la 1.75
// original (con ranura microSD y RTC) compila con -DBOARD_AMOLED_175, p. ej.:
//   arduino-cli compile --build-property "compiler.cpp.extra_flags=-DBOARD_AMOLED_175" ...
#if !defined(BOARD_AMOLED_175) && !defined(BOARD_AMOLED_175C)
#define BOARD_AMOLED_175C
#endif
#if defined(BOARD_AMOLED_175) && defined(BOARD_AMOLED_175C)
#error "define solo una placa: BOARD_AMOLED_175 o BOARD_AMOLED_175C"
#endif

#define XPOWERS_CHIP_AXP2101

// Pantalla AMOLED 466x466, driver CO5300 por QSPI
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Táctil capacitivo CST9217 por I2C
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11

// Audio ES8311
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

#if defined(BOARD_AMOLED_175C)
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C
// (examples/arduino/libraries/board_config/pin_config.h). Mismos chips que la
// 1.75 pero con los reset en GPIO1/2, MCLK en GPIO16, sin ranura TF y sin RTC
// PCF85063. Los sprites van en la particion FAT de la flash de 32 MB.
#define LCD_RESET 1
#define TP_RESET 2
#define I2S_MCK_IO 16
#define HAS_SD_SLOT 0
#else
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)
#define LCD_RESET 39
#define TP_RESET 40
// NOTA: el MCLK real es GPIO42 (verificado en placa con el proyecto
// PlaneRadar2.0); el 16 que figuraba era erroneo. Aun asi el codec se
// configura con reloj derivado del BCLK, asi que el MCLK apenas importa.
#define I2S_MCK_IO 42
#define HAS_SD_SLOT 1

// Ranura TF
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41
#endif

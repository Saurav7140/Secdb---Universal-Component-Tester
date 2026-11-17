#ifndef SECDB_BOARD_H
#define SECDB_BOARD_H

#include "stm32f4xx_hal.h"

/* Shared analog input. */
#define SECDB_ADC_GPIO_Port              GPIOA
#define SECDB_ADC_Pin                    GPIO_PIN_0
#define SECDB_ADC_Channel                ADC_CHANNEL_0

/* BJT socket drive paths: E through 100 ohm, B through 1 Mohm, C through 1 kohm. */
#define SECDB_BJT_E_GPIO_Port            GPIOA
#define SECDB_BJT_E_Pin                  GPIO_PIN_1
#define SECDB_BJT_B_GPIO_Port            GPIOA
#define SECDB_BJT_B_Pin                  GPIO_PIN_2
#define SECDB_BJT_C_GPIO_Port            GPIOA
#define SECDB_BJT_C_Pin                  GPIO_PIN_3

/* TFT (ST7735, SPI1). */
#define SECDB_TFT_CS_GPIO_Port           GPIOA
#define SECDB_TFT_CS_Pin                 GPIO_PIN_4
#define SECDB_TFT_SCK_GPIO_Port          GPIOA
#define SECDB_TFT_SCK_Pin                GPIO_PIN_5
#define SECDB_TFT_MOSI_GPIO_Port         GPIOA
#define SECDB_TFT_MOSI_Pin               GPIO_PIN_7
#define SECDB_TFT_RST_GPIO_Port          GPIOB
#define SECDB_TFT_RST_Pin                GPIO_PIN_0
#define SECDB_TFT_DC_GPIO_Port           GPIOB
#define SECDB_TFT_DC_Pin                 GPIO_PIN_1

/* Bidirectional diode drive paths, each through 1 kohm. */
#define SECDB_DIO_A_GPIO_Port            GPIOA
#define SECDB_DIO_A_Pin                  GPIO_PIN_6
#define SECDB_DIO_B_GPIO_Port            GPIOA
#define SECDB_DIO_B_Pin                  GPIO_PIN_10

/* TMUX1104 U_SRC. PA8 is intentional: PB11 is absent on UFQFPN48. */
#define SECDB_SRC_A1_GPIO_Port           GPIOA
#define SECDB_SRC_A1_Pin                 GPIO_PIN_8
#define SECDB_SRC_A0_GPIO_Port           GPIOB
#define SECDB_SRC_A0_Pin                 GPIO_PIN_10
#define SECDB_SRC_EN_GPIO_Port           GPIOB
#define SECDB_SRC_EN_Pin                 GPIO_PIN_12

/* TMUX1108 U_SENSE. */
#define SECDB_SNS_EN_GPIO_Port           GPIOB
#define SECDB_SNS_EN_Pin                 GPIO_PIN_9
#define SECDB_SNS_A0_GPIO_Port           GPIOB
#define SECDB_SNS_A0_Pin                 GPIO_PIN_13
#define SECDB_SNS_A1_GPIO_Port           GPIOB
#define SECDB_SNS_A1_Pin                 GPIO_PIN_14
#define SECDB_SNS_A2_GPIO_Port           GPIOB
#define SECDB_SNS_A2_Pin                 GPIO_PIN_15

/* Local user interface. Buttons are active low with external 10 kohm pull-ups. */
#define SECDB_LED_GPIO_Port              GPIOB
#define SECDB_LED_Pin                    GPIO_PIN_8
#define SECDB_BTN_MODE_GPIO_Port         GPIOC
#define SECDB_BTN_MODE_Pin               GPIO_PIN_13
#define SECDB_BTN_UP_GPIO_Port           GPIOC
#define SECDB_BTN_UP_Pin                 GPIO_PIN_14
#define SECDB_BTN_DOWN_GPIO_Port         GPIOC
#define SECDB_BTN_DOWN_Pin               GPIO_PIN_15

/* USB device pins reserved by the board design. */
#define SECDB_USB_VBUS_GPIO_Port         GPIOA
#define SECDB_USB_VBUS_Pin               GPIO_PIN_9
#define SECDB_USB_DM_GPIO_Port           GPIOA
#define SECDB_USB_DM_Pin                 GPIO_PIN_11
#define SECDB_USB_DP_GPIO_Port           GPIOA
#define SECDB_USB_DP_Pin                 GPIO_PIN_12

#endif /* SECDB_BOARD_H */

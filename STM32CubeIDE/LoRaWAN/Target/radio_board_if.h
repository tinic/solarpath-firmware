/**
  ******************************************************************************
  * @file    radio_board_if.h
  * @author  MCD Application Team
  * @brief   Header for Radio interface configuration
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RADIO_BOARD_IF_H
#define RADIO_BOARD_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
/* USER CODE BEGIN include */

/* USER CODE END include */

/* Exported defines ----------------------------------------------------------*/
#if defined(USE_BSP_DRIVER)
/* code generated by MX does not support BSP */
/* In order to use BSP driver, add the correspondent files in the IDE workspace */
/* and define USE_BSP_DRIVER in the preprocessor definitions  or in platform.h */
#define RBI_CONF_RFO_LP_HP  RADIO_CONF_RFO_LP_HP
#define RBI_CONF_RFO_LP     RADIO_CONF_RFO_LP
#define RBI_CONF_RFO_HP     RADIO_CONF_RFO_HP
#else
/* USER CODE BEGIN Board Definition */
#define RBI_CONF_RFO_LP_HP  0
#define RBI_CONF_RFO_LP     1
#define RBI_CONF_RFO_HP     2
/* USER CODE END Board Definition */
#endif  /* USE_BSP_DRIVER  */

#if defined(USE_BSP_DRIVER)
/* code generated by MX does not support BSP */
/* In order to use BSP driver, add the correspondent files in the IDE workspace */
/* and define USE_BSP_DRIVER in the preprocessor definitions  or in platform.h */

#else
/* USER CODE BEGIN Exported Parameters */
#define RBI_CONF_RFO                        RBI_CONF_RFO_LP

/* Radio maximum wakeup time (in ms) */
#define RF_WAKEUP_TIME                     10U

/* Indicates whether or not TCXO is supported by the board
 * 0: TCXO not supported
 * 1: TCXO supported
 */
#define IS_TCXO_SUPPORTED                   0U

/* Indicates whether or not DCDC is supported by the board
 * 0: DCDC not supported
 * 1: DCDC supported
 */
#define IS_DCDC_SUPPORTED                   0U
/* USER CODE END Exported Parameters */
#endif  /* USE_BSP_DRIVER  */

#if defined(USE_BSP_DRIVER)
/* code generated by MX does not support BSP */
/* In order to use BSP driver, add the correspondent files in the IDE workspace */
/* and define USE_BSP_DRIVER in the preprocessor definitions  or in platform.h */

#elif defined(MX_)
#else
/* USER CODE BEGIN Exported PinMapping */
#define RF_SW_CTRL3_PIN                          GPIO_PIN_5
#define RF_SW_CTRL3_GPIO_PORT                    GPIOA
#define RF_SW_CTRL3_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOA_CLK_ENABLE()
#define RF_SW_CTRL3_GPIO_CLK_DISABLE()           __HAL_RCC_GPIOA_CLK_DISABLE()

#define RF_SW_CTRL1_PIN                          GPIO_PIN_9
#define RF_SW_CTRL1_GPIO_PORT                    GPIOA
#define RF_SW_CTRL1_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOA_CLK_ENABLE()
#define RF_SW_RX_GPIO_CLK_DISABLE()              __HAL_RCC_GPIOA_CLK_DISABLE()

#define RF_SW_CTRL2_PIN                          GPIO_PIN_7
#define RF_SW_CTRL2_GPIO_PORT                    GPIOA
#define RF_SW_CTRL2_GPIO_CLK_ENABLE()            __HAL_RCC_GPIOA_CLK_ENABLE()
#define RF_SW_CTRL2_GPIO_CLK_DISABLE()           __HAL_RCC_GPIOA_CLK_DISABLE()
/* USER CODE END Exported PinMapping */
#endif  /* USE_BSP_DRIVER  */

/* USER CODE BEGIN Exported_Defines */

/* USER CODE END Exported_Defines */

/* Exported types ------------------------------------------------------------*/

#if defined(USE_BSP_DRIVER)
/* code generated by MX does not support BSP */
/* In order to use BSP driver, add the correspondent files in the IDE workspace */
/* and define USE_BSP_DRIVER in the preprocessor definitions  or in platform.h */
typedef enum
{
  RBI_SWITCH_OFF    = RADIO_SWITCH_OFF,
  RBI_SWITCH_RX     = RADIO_SWITCH_RX,
  RBI_SWITCH_RFO_LP = RADIO_SWITCH_RFO_LP,
  RBI_SWITCH_RFO_HP = RADIO_SWITCH_RFO_HP,
} RBI_Switch_TypeDef;
#else
/* USER CODE BEGIN Exported Types */
typedef enum
{
  RBI_SWITCH_OFF    = 0,
  RBI_SWITCH_RX     = 1,
  RBI_SWITCH_RFO_LP = 2,
  RBI_SWITCH_RFO_HP = 3,
} RBI_Switch_TypeDef;
/* USER CODE END Exported Types */
#endif  /* USE_BSP_DRIVER */

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions ------------------------------------------------------- */
/**
  * @brief  Init Radio Switch
  * @return BSP status
  */
int32_t RBI_Init(void);

/**
  * @brief  DeInit Radio Switch
  * @return BSP status
  */
int32_t RBI_DeInit(void);

/**
  * @brief  Configure Radio Switch.
  * @param  Config: Specifies the Radio RF switch path to be set.
  *         This parameter can be one of following parameters:
  *           @arg RADIO_SWITCH_OFF
  *           @arg RADIO_SWITCH_RX
  *           @arg RADIO_SWITCH_RFO_LP
  *           @arg RADIO_SWITCH_RFO_HP
  * @return BSP status
  */
int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef Config);

/**
  * @brief  Return Board Configuration
  * @retval RBI_CONF_RFO_LP_HP
  * @retval RBI_CONF_RFO_LP
  * @retval RBI_CONF_RFO_HP
  */
int32_t RBI_GetTxConfig(void);

/**
  * @brief  Get Radio Wake Time
  * @return the wake upt time in ms
  */
int32_t RBI_GetWakeUpTime(void);

/**
  * @brief  Get If TCXO is to be present on board
  * @note   never remove called by MW,
  * @retval return 1 if present, 0 if not present
  */
int32_t RBI_IsTCXO(void);

/**
  * @brief  Get If DCDC is to be present on board
  * @note   never remove called by MW,
  * @retval return 1 if present, 0 if not present
  */
int32_t RBI_IsDCDC(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif

#endif /* RADIO_BOARD_IF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/**
  ******************************************************************************
  * @file    lora_app.c
  * @author  MCD Application Team
  * @brief   Application of the LRWAN Middleware
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

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "Region.h" /* Needed for LORAWAN_DEFAULT_DATA_RATE */
#include "sys_app.h"
#include "lora_app.h"
#include "stm32_seq.h"
#include "LmHandler.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "stm32_lpm.h"
#include "utilities_def.h"
#include "lora_info.h"
#include "solarpath.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
//#define DEBUG_MSG
/* USER CODE END PM */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  join event callback function
  * @param  params
  * @retval none
  */
static void OnJoinRequest(LmHandlerJoinParams_t *joinParams);

/**
  * @brief  tx event callback function
  * @param  params
  * @retval none
  */
static void OnTxData(LmHandlerTxParams_t *params);

/**
  * @brief callback when LoRa endNode has received a frame
  * @param appData
  * @param params
  * @retval None
  */
static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params);

/*!
 * Will be called each time a Radio IRQ is handled by the MAC layer
 *
 */
static void OnMacProcessNotify(void);

/* USER CODE BEGIN PFP */

static LmHandlerAppData_t TxData = { LORAWAN_USER_APP_PORT, 0, 0 };

static ActivationType_t ActivationType = LORAWAN_DEFAULT_ACTIVATION_TYPE;

static UTIL_TIMER_Object_t JoinNetworkTimer = { 0 };
static UTIL_TIMER_Object_t SendTxDataTimer = { 0 };

static void OnJoinNetworkTimerEvent(void *context);
static void OnSendTxDataTimerEvent(void *context);

static void JoinNetwork(void);
static void SendTxData(void);
/* USER CODE END PFP */

/* Private variables ---------------------------------------------------------*/
/**
  * @brief LoRaWAN handler Callbacks
  */
static LmHandlerCallbacks_t LmHandlerCallbacks =
{
  .GetBatteryLevel =           GetBatteryLevel,
  .GetTemperature =            GetTemperatureLevel,
  .OnMacProcess =              OnMacProcessNotify,
  .OnJoinRequest =             OnJoinRequest,
  .OnTxData =                  OnTxData,
  .OnRxData =                  OnRxData
};

/* USER CODE BEGIN PV */
static LmHandlerParams_t LmHandlerParams =
{
  .ActiveRegion =             ACTIVE_REGION,
  .DefaultClass =             LORAWAN_DEFAULT_CLASS,
  .AdrEnable =                LORAWAN_ADR_STATE,
  .TxDatarate =               LORAWAN_DEFAULT_DATA_RATE,
  .PingPeriodicity =          LORAWAN_DEFAULT_PING_SLOT_PERIODICITY
};

/* USER CODE END PV */

/* Exported functions ---------------------------------------------------------*/
/* USER CODE BEGIN EF */

/* USER CODE END EF */

void LoRaWAN_Init(void)
{
  /* USER CODE BEGIN LoRaWAN_Init_1 */
  uint8_t devEui[8];
  GetUniqueId(devEui);

  printf("DevEUI: ");
  for(size_t c = 0; c < 8; c++) {
	  printf("%02x ",devEui[c]);
  }
  printf("\n");

  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_LmHandlerProcess), UTIL_SEQ_RFU, LmHandlerProcess);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_JoinNetworkTimer), UTIL_SEQ_RFU, JoinNetwork);
  UTIL_SEQ_RegTask((1 << CFG_SEQ_Task_SendTxTimer), UTIL_SEQ_RFU, SendTxData);

  LoraInfo_Init();
  /* USER CODE END LoRaWAN_Init_1 */

  /* Init the Lora Stack*/
  LmHandlerInit(&LmHandlerCallbacks);

  /* USER CODE BEGIN LoRaWAN_Init_Last */
  LmHandlerConfigure(&LmHandlerParams);

  LmHandlerJoin(ActivationType);

  UTIL_TIMER_Create(&JoinNetworkTimer, 0xFFFFFFFFU, UTIL_TIMER_ONESHOT, OnJoinNetworkTimerEvent, NULL);
  UTIL_TIMER_SetPeriod(&JoinNetworkTimer, APP_TX_DUTYCYCLE);

  UTIL_TIMER_Create(&SendTxDataTimer, 0xFFFFFFFFU, UTIL_TIMER_PERIODIC, OnSendTxDataTimerEvent, NULL);
  UTIL_TIMER_SetPeriod(&SendTxDataTimer, APP_TX_DUTYCYCLE);

  UTIL_LPM_Init();
  UTIL_LPM_SetOffMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
  UTIL_LPM_SetStopMode((1 << CFG_LPM_APPLI_Id), UTIL_LPM_DISABLE);
  /* USER CODE END LoRaWAN_Init_Last */
}

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */
static void OnSendTxDataTimerEvent(void *context) {
	UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_SendTxTimer), CFG_SEQ_Prio_0);
}

static void OnJoinNetworkTimerEvent(void *context) {
	UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_JoinNetworkTimer), CFG_SEQ_Prio_0);
}

static void JoinNetwork() {
#ifdef DEBUG_MSG
	printf("Re-Attempt join!\n");
#endif  // #ifdef DEBUG_MSG
	LmHandlerJoin(ActivationType);
}

static void SendTxData(void) {
	TxData.Buffer = (uint8_t *)encode_packet(&TxData.BufferSize);
	UTIL_TIMER_Time_t nextTxIn = 0;
	if (LORAMAC_HANDLER_SUCCESS == LmHandlerSend(&TxData, LORAWAN_DEFAULT_CONFIRMED_MSG_STATE, &nextTxIn, false)) {
#ifdef DEBUG_MSG
		printf("SendTxData Success!\n");
	} else if (nextTxIn > 0) {
		printf("SendTxData Early!\n");
	} else {
		printf("SendTxData Fail!\n");
#endif // #ifdef DEBUG_MSG
	}
	system_update();
}

/* USER CODE END PrFD */

static void OnRxData(LmHandlerAppData_t *appData, LmHandlerRxParams_t *params)
{
  /* USER CODE BEGIN OnRxData_1 */
	if ((appData != NULL) && (params != NULL)) {
		switch (appData->Port) {
			case LORAWAN_SWITCH_CLASS_PORT: {
				if (appData->BufferSize == 1) {
					switch (appData->Buffer[0]) {
						case 0: {
							LmHandlerRequestClass(CLASS_A);
#ifdef DEBUG_MSG
							printf("CLASS_A!\n");
#endif  // #ifdef DEBUG_MSG
						}
						break;
						case 1: {
							LmHandlerRequestClass(CLASS_B);
#ifdef DEBUG_MSG
							printf("CLASS_B!\n");
#endif  // #ifdef DEBUG_MSG
						}
						break;
						case 2: {
							LmHandlerRequestClass(CLASS_C);
#ifdef DEBUG_MSG
							printf("CLASS_C!\n");
#endif  // #ifdef DEBUG_MSG
						}
						break;
						default: {
							break;
						}
					}
				}
			}
			break;
			case LORAWAN_USER_APP_PORT: {
				decode_packet(appData->Buffer, appData->BufferSize);
				system_update();
			}
			break;
			default: {
			} break;
		}
	}
  /* USER CODE END OnRxData_1 */
}

static void OnTxData(LmHandlerTxParams_t *params)
{
  /* USER CODE BEGIN OnTxData_1 */
	if ((params != NULL) && (params->IsMcpsConfirm != 0)) {
		if (params->MsgType == LORAMAC_HANDLER_CONFIRMED_MSG) {
#ifdef DEBUG_MSG
			printf("OnTxData confirmed!!\n");
		} else {
			printf("OnTxData unconfirmed!\n");
#endif // #ifdef DEBUG_MSG
		}
	}
  /* USER CODE END OnTxData_1 */
}

static void OnJoinRequest(LmHandlerJoinParams_t *joinParams)
{
  /* USER CODE BEGIN OnJoinRequest_1 */
	if (joinParams != NULL) {
		if (joinParams->Status == LORAMAC_HANDLER_SUCCESS) {
			if (joinParams->Mode == ACTIVATION_TYPE_OTAA) {
#ifdef DEBUG_MSG
				printf("OnJoinRequest confirmed!!\n");
#endif  // #ifdef DEBUG_MSG
				UTIL_TIMER_Start(&SendTxDataTimer);
			}
		} else {
#ifdef DEBUG_MSG
			printf("OnJoinRequest retry\n");
#endif  // #ifdef DEBUG_MSG
			UTIL_TIMER_Start(&JoinNetworkTimer);
		}
	}
  /* USER CODE END OnJoinRequest_1 */

  /* USER CODE BEGIN OnJoinRequest_2 */

  /* USER CODE END OnJoinRequest_2 */
}

static void OnMacProcessNotify(void)
{
  /* USER CODE BEGIN OnMacProcessNotify_1 */
  UTIL_SEQ_SetTask((1 << CFG_SEQ_Task_LmHandlerProcess), CFG_SEQ_Prio_0);
  /* USER CODE END OnMacProcessNotify_1 */
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

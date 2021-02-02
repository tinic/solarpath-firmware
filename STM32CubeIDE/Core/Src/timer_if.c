/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    timer_if.c
  * @author  MCD Application Team
  * @brief   Configure RTC Alarm, Tick and Calendar manager
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include "timer_if.h"

/* USER CODE BEGIN Includes */
#include "main.h"
#include "stm32_lpm.h"
#include "utilities_def.h"
#include "stm32wlxx_ll_rtc.h"
/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/**
  * @brief Timer driver callbacks handler
  */
const UTIL_TIMER_Driver_s UTIL_TimerDriver =
{
  TIMER_IF_Init,
  NULL,

  TIMER_IF_StartTimer,
  TIMER_IF_StopTimer,

  TIMER_IF_SetTimerContext,
  TIMER_IF_GetTimerContext,

  TIMER_IF_GetTimerElapsedTime,
  TIMER_IF_GetTimerValue,
  TIMER_IF_GetMinimumTimeout,

  TIMER_IF_Convert_ms2Tick,
  TIMER_IF_Convert_Tick2ms,
};

/**
  * @brief SysTime driver callbacks handler
  */
const UTIL_SYSTIM_Driver_s UTIL_SYSTIMDriver =
{
  TIMER_IF_BkUp_Write_Seconds,
  TIMER_IF_BkUp_Read_Seconds,
  TIMER_IF_BkUp_Write_SubSeconds,
  TIMER_IF_BkUp_Read_SubSeconds,
  TIMER_IF_GetTime,
};

/* USER CODE BEGIN EV */
extern RTC_HandleTypeDef hrtc;
/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TIMER_IF_DBG_PRINTF(...)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MIN_ALARM_DELAY    3
#define RTC_BKP_SECONDS    RTC_BKP_DR0
#define RTC_BKP_SUBSECONDS RTC_BKP_DR1
#define RTC_BKP_MSBTICKS   RTC_BKP_DR2
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static bool RTC_Initialized = false;
static uint32_t RtcTimerContext = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void TIMER_IF_BkUp_Write_MSBticks(uint32_t MSBticks);
static uint32_t TIMER_IF_BkUp_Read_MSBticks(void);
static inline uint32_t GetTimerTicks(void);
/* USER CODE END PFP */

/* Exported functions ---------------------------------------------------------*/
UTIL_TIMER_Status_t TIMER_IF_Init(void)
{
  UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
  /* USER CODE BEGIN TIMER_IF_Init */
  if (RTC_Initialized == false)
  {
    hrtc.IsEnabled.RtcFeatures = UINT32_MAX;
    /*Init RTC*/
    MX_RTC_Init();
    /*Stop Timer */
    TIMER_IF_StopTimer();
    /** DeActivate the Alarm A enabled by MX during MX_RTC_Init() */
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    /*overload RTC feature enable*/
    hrtc.IsEnabled.RtcFeatures = UINT32_MAX;

    /*Enable Direct Read of the calendar registers (not through Shadow) */
    HAL_RTCEx_EnableBypassShadow(&hrtc);
    /*Initialise MSB ticks*/
    TIMER_IF_BkUp_Write_MSBticks(0);

    TIMER_IF_SetTimerContext();

    RTC_Initialized = true;
  }

  /* USER CODE END TIMER_IF_Init */
  return ret;
}

UTIL_TIMER_Status_t TIMER_IF_StartTimer(uint32_t timeout)
{
  UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
  /* USER CODE BEGIN TIMER_IF_StartTimer */
  RTC_AlarmTypeDef sAlarm = {0};
  /*Stop timer if one is already started*/
  TIMER_IF_StopTimer();
  timeout += RtcTimerContext;

  TIMER_IF_DBG_PRINTF("Start timer: time=%d, alarm=%d\n\r",  GetTimerTicks(), timeout);
  /* starts timer*/
  sAlarm.BinaryAutoClr = RTC_ALARMSUBSECONDBIN_AUTOCLR_NO;
  sAlarm.AlarmTime.SubSeconds = UINT32_MAX - timeout;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDBINMASK_NONE;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END TIMER_IF_StartTimer */
  return ret;
}

UTIL_TIMER_Status_t TIMER_IF_StopTimer(void)
{
  UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
  /* USER CODE BEGIN TIMER_IF_StopTimer */
  __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
  /* Disable the Alarm A interrupt */
  HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
  /*overload RTC feature enable*/
  hrtc.IsEnabled.RtcFeatures = UINT32_MAX;

  /* USER CODE END TIMER_IF_StopTimer */
  return ret;
}

uint32_t TIMER_IF_SetTimerContext(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_SetTimerContext */
  RtcTimerContext = ret = GetTimerTicks();

  /* USER CODE END TIMER_IF_SetTimerContext */
  return ret;
}

uint32_t TIMER_IF_GetTimerContext(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTimerContext */
  ret = RtcTimerContext;
  /* USER CODE END TIMER_IF_GetTimerContext */
  return ret;
}

uint32_t TIMER_IF_GetTimerElapsedTime(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTimerElapsedTime */
  return ((uint32_t)(GetTimerTicks() - RtcTimerContext));
  /* USER CODE END TIMER_IF_GetTimerElapsedTime */
  return ret;
}

uint32_t TIMER_IF_GetTimerValue(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTimerValue */
  if (RTC_Initialized == true)
  {
    ret = GetTimerTicks();
  }
  else
  {
    ret = 0;
  }
  /* USER CODE END TIMER_IF_GetTimerValue */
  return ret;
}

uint32_t TIMER_IF_GetMinimumTimeout(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetMinimumTimeout */
  ret = MIN_ALARM_DELAY;

  /* USER CODE END TIMER_IF_GetMinimumTimeout */
  return ret;
}

uint32_t TIMER_IF_Convert_ms2Tick(uint32_t timeMilliSec)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_Convert_ms2Tick */
#if RTC_USE_HSE
  ret = ( timeMilliSec * 31250 ) / 1000;
#else // #ifdef USE_RTC_CLOCK
  ret = ((uint32_t)((((uint64_t) timeMilliSec) << RTC_N_PREDIV_S) / 1000));
#endif  // #ifdef USE_RTC_CLOCK
  /* USER CODE END TIMER_IF_Convert_ms2Tick */
  return ret;
}

uint32_t TIMER_IF_Convert_Tick2ms(uint32_t tick)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_Convert_Tick2ms */
#if RTC_USE_HSE
  ret = ( tick * 1000 ) / 31250;
#else  // #ifdef USE_RTC_CLOCK
  ret = ((uint32_t)((((uint64_t)(tick)) * 1000) >> RTC_N_PREDIV_S));
#endif  // #ifdef USE_RTC_CLOCK
  /* USER CODE END TIMER_IF_Convert_Tick2ms */
    return ret;
}

void TIMER_IF_DelayMs(uint32_t delay)
{
  /* USER CODE BEGIN TIMER_IF_DelayMs */
  uint32_t delayTicks = TIMER_IF_Convert_ms2Tick(delay);
  uint32_t timeout = GetTimerTicks();

  /* Wait delay ms */
  while (((GetTimerTicks() - timeout)) < delayTicks)
  {
	__NOP();
  }
  /* USER CODE END TIMER_IF_DelayMs */
}

uint32_t TIMER_IF_GetTime(uint16_t *mSeconds)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_GetTime */
  uint64_t ticks;
  uint32_t timerValueLsb = GetTimerTicks();
  uint32_t timerValueMSB = TIMER_IF_BkUp_Read_MSBticks();

  ticks = (((uint64_t) timerValueMSB) << 32) + timerValueLsb;

#if RTC_USE_HSE
  uint32_t seconds = (uint32_t)(ticks / 31250);
  ticks = (uint32_t) ticks % 31250;
#else  // #if USE_RTC_CLOCK
  uint32_t seconds = (uint32_t)(ticks >> RTC_N_PREDIV_S);
  ticks = (uint32_t) ticks & RTC_PREDIV_S;
#endif  // #if USE_RTC_CLOCK

  *mSeconds = TIMER_IF_Convert_Tick2ms(ticks);
  ret = seconds;
  /* USER CODE END TIMER_IF_GetTime */
    return ret;
}

void TIMER_IF_BkUp_Write_Seconds(uint32_t Seconds)
{
  /* USER CODE BEGIN TIMER_IF_BkUp_Write_Seconds */
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_SECONDS, Seconds);
  /* USER CODE END TIMER_IF_BkUp_Write_Seconds */
}

void TIMER_IF_BkUp_Write_SubSeconds(uint32_t SubSeconds)
{
  /* USER CODE BEGIN TIMER_IF_BkUp_Write_SubSeconds */
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_SUBSECONDS, SubSeconds);
  /* USER CODE END TIMER_IF_BkUp_Write_SubSeconds */
}

uint32_t TIMER_IF_BkUp_Read_Seconds(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_BkUp_Read_Seconds */
  ret = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_SECONDS);
  /* USER CODE END TIMER_IF_BkUp_Read_Seconds */
    return ret;
}

uint32_t TIMER_IF_BkUp_Read_SubSeconds(void)
{
  uint32_t ret = 0;
  /* USER CODE BEGIN TIMER_IF_BkUp_Read_SubSeconds */
  ret = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_SUBSECONDS);
  /* USER CODE END TIMER_IF_BkUp_Read_SubSeconds */
    return ret;
}

/* USER CODE BEGIN EF */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* USER CODE BEGIN HAL_RTC_AlarmAEventCallback */

  /* USER CODE END HAL_RTC_AlarmAEventCallback */
  UTIL_TIMER_IRQ_Handler();
  /* USER CODE BEGIN HAL_RTC_AlarmAEventCallback_Last */

  /* USER CODE END HAL_RTC_AlarmAEventCallback_Last */
}

void HAL_RTCEx_SSRUEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* USER CODE BEGIN HAL_RTCEx_SSRUEventCallback */

  /* USER CODE END HAL_RTCEx_SSRUEventCallback */
  /*called every 48 days with 1024 ticks per seconds*/
  TIMER_IF_DBG_PRINTF(">>Handler SSRUnderflow at %d\n\r", GetTimerTicks());
  /*Increment MSBticks*/
  uint32_t MSB_ticks = TIMER_IF_BkUp_Read_MSBticks();
  TIMER_IF_BkUp_Write_MSBticks(MSB_ticks + 1);
  /* USER CODE BEGIN HAL_RTCEx_SSRUEventCallback_Last */

  /* USER CODE END HAL_RTCEx_SSRUEventCallback_Last */
}
/* USER CODE END EF */

/* Private functions ---------------------------------------------------------*/
/* USER CODE BEGIN PrFD */
static void TIMER_IF_BkUp_Write_MSBticks(uint32_t MSBticks)
{
  /* USER CODE BEGIN TIMER_IF_BkUp_Write_MSBticks */

  /* USER CODE END TIMER_IF_BkUp_Write_MSBticks */
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_MSBTICKS, MSBticks);
  /* USER CODE BEGIN TIMER_IF_BkUp_Write_MSBticks_Last */

  /* USER CODE END TIMER_IF_BkUp_Write_MSBticks_Last */
}

static uint32_t TIMER_IF_BkUp_Read_MSBticks(void)
{
  /* USER CODE BEGIN TIMER_IF_BkUp_Read_MSBticks */

  /* USER CODE END TIMER_IF_BkUp_Read_MSBticks */
  uint32_t MSBticks;
  MSBticks = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_MSBTICKS);
  return MSBticks;
  /* USER CODE BEGIN TIMER_IF_BkUp_Read_MSBticks_Last */

  /* USER CODE END TIMER_IF_BkUp_Read_MSBticks_Last */
}

static inline uint32_t GetTimerTicks(void)
{
  /* USER CODE BEGIN GetTimerTicks */

  /* USER CODE END GetTimerTicks */
  return (UINT32_MAX - LL_RTC_TIME_GetSubSecond(RTC));
  /* USER CODE BEGIN GetTimerTicks_Last */

  /* USER CODE END GetTimerTicks_Last */
}

/* USER CODE END PrFD */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

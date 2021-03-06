/**
 * Copyright (c) 2011 - 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

/*==============================================================================
  Edit History

  This section contains comments describing changes made to the module. Notice
  that changes are listed in reverse chronological order. Please use ISO format
  for dates.

  $Header: //components/rel/ssc.adsp/2.4.1/adsp_proc/Sensors/ddf/src/8084/sns_ddf_signal.c#2 $
  $DateTime: 2014/06/11 17:17:47 $

  when       who  what, where, why
  ---------- ---  -----------------------------------------------------------
  2014-06-03 rt   Fixed race condition by using QURT atomic ops  
  2014-05-12 ak   Fixed GPIO config for interrupt
  2014-02-26 tc   Fixed pending_flags check in get_free_slot
  2014-01-24 pn   Ported changes from 8974
  2014-01-27 hw   Add QDSS logging
  2013-12-18 MW   Added sns_ddf_signal_gpio_read
  2013-06-12 pn   Added debug logging to catch first time interrupt is triggered
  2013-06-11 ae   Added QDSP SIM playback support
  2013-04-06 ag   Remove APDS interrupt workaround
  2013-01-18 ag   
             pn   APDS workaround
  2012-11-07 ag   Removed debug printf
  2012-10-25 vh   Eliminated compiler warnings
  2012-10-19 ag   Port code to work on ADSP
  2012-09-13 vh   Eliminated compiler warnings
  2012-04-13 br   Fixed racing condition between ISR and sns_ddf_signal_dispatch()
  2012-03-05 br   change the type fo gpio_num from 32 bits to 16 bits
  2012-02-03 sd   moved DRI gpio control to DD
  2011-09-01 sd   Changed MD int detect GPIO bin to 67 to use INT2
==============================================================================*/
#include "sns_ddf_signal.h"
#include "sns_ddf_smgr_if.h"
#include "sns_ddf_util.h"
#include "sns_smgr_sensor_config.h"

#if !defined(SNS_PCSIM)
#include "DALSys.h"
#include "DDITlmm.h"
#include "DALDeviceId.h"
#include "DDIGPIOInt.h"
#endif

#include  "sns_debug_str.h"
#include "sns_profiling.h"

// Each driver is allowed up to 2 GPIOs, however, in practice there are no
// devices that use more than one (and some even share this one for multiple
// interrupts). Therefore we save some memory by restricting the number of
// listeners to a more realistic upper limit.
#define SNS_DDF_SIGNAL_MAX_CNT  (SNS_SMGR_NUM_SENSORS_DEFINED+1)

// Data structure describing a registered user of the DDF signal service.
typedef struct
{
  bool                  is_registered;  /* TRUE if this signal is registered */
  bool                  int_fired;  /* for debug only */
  uint16_t              gpio_num;
  uint16_t              warning_cnt;
  sns_ddf_handle_t      handle;
  sns_ddf_driver_if_s*  driver;
  uint32_t              gpio_pin_cfg;
  uint32_t              timestamp;
  uint32_t              prev_timestamp;
  sns_ddf_signal_irq_trigger_e trigger;
} sns_ddf_sig_info_s;

#if !defined(SNS_PCSIM)
typedef struct
{
  DalDeviceHandle       *tlmm_handle;
  DalDeviceHandle       *gpio_int_handle;
  DALSYSSyncHandle      sync_handle;
  uint32_t              pending_flags;  /* signal pending flags */
  sns_ddf_sig_info_s    sig_info[SNS_DDF_SIGNAL_MAX_CNT];
} sns_ddf_sig_tb_s;

static sns_ddf_sig_tb_s sns_ddf_sig_tb;
#endif

extern sns_ddf_driver_if_s sns_dd_qdsp_playback_if;
/**
 * Initializes the signal service's internal structures.
 */
static void sns_ddf_signal_init(void)
{
#if !defined(SNS_PCSIM)
  uint8_t i;
  sns_ddf_sig_tb.pending_flags = 0;
  for(i = 0; i < SNS_DDF_SIGNAL_MAX_CNT; i++)
  {
    sns_ddf_sig_tb.sig_info[i].is_registered = false;
    sns_ddf_sig_tb.sig_info[i].warning_cnt = 0;
    sns_ddf_sig_tb.sig_info[i].int_fired = false;
  }

  if(DAL_SUCCESS != DAL_DeviceAttach(DALDEVICEID_TLMM, &sns_ddf_sig_tb.tlmm_handle))
  {
    SNS_ASSERT(0);
  }
  if (DAL_SUCCESS != DALSYS_SyncCreate(DALSYS_SYNC_ATTR_NO_PREEMPTION,
                                       &sns_ddf_sig_tb.sync_handle, NULL))
  {
    SNS_ASSERT(0);
  }
  if(DAL_SUCCESS != DAL_DeviceAttach(DALDEVICEID_GPIOINT, &sns_ddf_sig_tb.gpio_int_handle))
  {
    SNS_ASSERT(0);
  }
#endif
}


/**
 * Returns the index of the signal informaiton registered for a particular GPIO, or -1
 * if none.
 */
static int8_t sns_ddf_signal_find_sig_index (uint16_t gpio_num)
{
#if !defined(SNS_PCSIM)
  int8_t i;
  for(i = 0; i < SNS_DDF_SIGNAL_MAX_CNT; i++) {
    if(sns_ddf_sig_tb.sig_info[i].is_registered && (sns_ddf_sig_tb.sig_info[i].gpio_num == gpio_num))
      return i;
  }
#endif
  return -1;
}

/**
 * Returns the index of the next free slot in the signal table, or -1 if
 * none.
 */
static int8_t sns_ddf_signal_get_free_slot (void)
{
#if !defined(SNS_PCSIM)
  int8_t i;
  for(i = 0; i < SNS_DDF_SIGNAL_MAX_CNT; i++) {
    if( !sns_ddf_sig_tb.sig_info[i].is_registered &&
        !(sns_ddf_sig_tb.pending_flags & (1 << i)) )
    {
      return i;
    }
  }
#endif
  return -1;
}


bool sns_ddf_signal_irq_enabled (void)
{
    return true;
}

/*===========================================================================

  FUNCTION:   sns_ddf_signal_gpio_enable

===========================================================================*/
/*!
  @brief  Enable/Disable GPIO pin
  @detail

  @param[i] gpio_handle the handle
  @param[i] gpio_pin_cfg gpio config
  @param[i] enable_flag DAL_TLMM_GPIO_ENABLE or DAL_TLMM_GPIO_DISABLE
  @return
   DAL_SUCCESS -- GPIO was successfully configured as requested.\n
   DAL_ERROR -- Either an invalid GPIO number in the configuration or ownership
   cannot be obtained from a secure root.
 */
/*=========================================================================*/
#if !defined(SNS_PCSIM)
int sns_ddf_signal_gpio_enable (DalDeviceHandle *tlmm_handle, uint32_t gpio_pin_cfg, DALGpioEnableType enable_flag)
{
  return DalTlmm_ConfigGpio(tlmm_handle, gpio_pin_cfg, enable_flag);
}
#endif

/*===========================================================================

  FUNCTION:   sns_ddf_signal_gpio_config

===========================================================================*/
/*!
  @brief Configuring GPIO pin

  @detail

  @param gpio_num gpio number
  @return
     gpio configuration
 */
/*=========================================================================*/
uint32_t sns_ddf_signal_gpio_config (uint32_t  gpio_num)
{
#if !defined(SNS_PCSIM)

    /* This configuration should be provided by HW team based on h/w schematic */
    /* Typically, interrupts are INPUT, NO_PULL */
  return DAL_GPIO_CFG(gpio_num, 0, DAL_GPIO_INPUT, DAL_GPIO_NO_PULL, DAL_GPIO_2MA);
#else
	return 0;
#endif
}

/*===========================================================================

  FUNCTION:   sns_ddf_signal_register

===========================================================================*/
/*!
  @brief This function registers interrupt 

  @detail 

  @param[i]  none
  @return   none
 */
/*=========================================================================*/
sns_ddf_status_e sns_ddf_signal_register (
    uint32_t                      gpio_num,
    sns_ddf_handle_t              dd_handle,
    sns_ddf_driver_if_s*          dd_interface,
    sns_ddf_signal_irq_trigger_e  trigger)
{
#if !defined(SNS_PCSIM)
    int8_t sig_idx;

    // Lazy initialization of the DDF signal service
    static bool signal_service_initialized = false;
    if(!signal_service_initialized)
    {
      sns_ddf_signal_init();
      signal_service_initialized = true;
    }

    sig_idx = sns_ddf_signal_find_sig_index(gpio_num);
    if(sig_idx == -1)
    {
      sig_idx = sns_ddf_signal_get_free_slot();
      if(sig_idx == -1)
        return SNS_DDF_EFAIL;
    }
    MSG_3(MSG_SSID_SNS,DBG_HIGH_PRIO,"sig_reg - idx(%d) gpio(%d) reg(%d)", 
          sig_idx, gpio_num, sns_ddf_sig_tb.sig_info[sig_idx].is_registered);
    
    DALSYS_SyncEnter(sns_ddf_sig_tb.sync_handle);
  
    sns_ddf_sig_tb.sig_info[sig_idx].gpio_num = gpio_num;
    sns_ddf_sig_tb.sig_info[sig_idx].handle = dd_handle;
    sns_ddf_sig_tb.sig_info[sig_idx].driver = dd_interface;
    sns_ddf_sig_tb.sig_info[sig_idx].trigger = trigger;
    sns_ddf_sig_tb.sig_info[sig_idx].gpio_pin_cfg= sns_ddf_signal_gpio_config(gpio_num);
    if ( DAL_ERROR ==sns_ddf_signal_gpio_enable(sns_ddf_sig_tb.tlmm_handle,
                                                sns_ddf_sig_tb.sig_info[sig_idx].gpio_pin_cfg, 
                                                DAL_TLMM_GPIO_ENABLE) )
    {
      //log error;
      DALSYS_SyncLeave(sns_ddf_sig_tb.sync_handle);
      return SNS_DDF_EFAIL;
    }

    if(!sns_ddf_sig_tb.sig_info[sig_idx].is_registered)
    {
      GPIOInt_RegisterIsr(
          sns_ddf_sig_tb.gpio_int_handle,
          gpio_num,
          (GPIOIntTriggerType)trigger,
          (GPIOINTISR)sns_ddf_smgr_notify_irq,
          sig_idx);
      sns_ddf_sig_tb.sig_info[sig_idx].is_registered = true;
    }
    DALSYS_SyncLeave(sns_ddf_sig_tb.sync_handle);
#endif
    return SNS_DDF_SUCCESS;
}


/*===========================================================================

  FUNCTION:   sns_ddf_signal_deregister

===========================================================================*/
/*!
  @brief This function de-registers interrupt 

  @detail 

  @param[i]  none
  @return   none
 */
/*=========================================================================*/
sns_ddf_status_e sns_ddf_signal_deregister (uint32_t gpio_num)
{
#if !defined(SNS_PCSIM)
    int8_t sig_idx;

    sig_idx = sns_ddf_signal_find_sig_index(gpio_num);
    if(sig_idx == -1)
        return SNS_DDF_EINVALID_PARAM;
    
    MSG_3(MSG_SSID_SNS,DBG_HIGH_PRIO,"sig_dereg - idx(%d) gpio(%d) fired(%d)", 
          sig_idx, sns_ddf_sig_tb.sig_info[sig_idx].gpio_num, 
          sns_ddf_sig_tb.sig_info[sig_idx].int_fired); 

    DALSYS_SyncEnter(sns_ddf_sig_tb.sync_handle);
    //tramp_gpio_deregister_isr(
    GPIOInt_DeregisterIsr(
        sns_ddf_sig_tb.gpio_int_handle,
        gpio_num, 
        (GPIOINTISR)sns_ddf_smgr_notify_irq);

    sns_ddf_sig_tb.sig_info[sig_idx].int_fired = false;
    sns_ddf_sig_tb.sig_info[sig_idx].is_registered = false;
    qurt_atomic_clear_bit(&sns_ddf_sig_tb.pending_flags, sig_idx);
    DALSYS_SyncLeave(sns_ddf_sig_tb.sync_handle);
#endif
    return SNS_DDF_SUCCESS;
}

/*===========================================================================

  FUNCTION:   sns_ddf_signal_post

===========================================================================*/
/*!
  @brief This function post the signal into the siganl database 

  @detail also this function saves the timestamp

  @param[i]  none
  @return   none
 */
/*=========================================================================*/
void sns_ddf_signal_post (uint32_t sig_num)
{
#if !defined(SNS_PCSIM) && !defined(SNS_QDSP_SIM)
  SNS_ASSERT(sig_num < SNS_DDF_SIGNAL_MAX_CNT);
  qurt_atomic_set_bit(&sns_ddf_sig_tb.pending_flags, sig_num);
  sns_ddf_sig_tb.sig_info[sig_num].timestamp = sns_em_get_timestamp();
  if (!sns_ddf_sig_tb.sig_info[sig_num].is_registered)
  {
     sns_ddf_sig_tb.sig_info[sig_num].warning_cnt++; 
     MSG_3(MSG_SSID_SNS,DBG_ERROR_PRIO,"sig_post - idx(%d) gpio(%d) warns(%d)", 
           sig_num, sns_ddf_sig_tb.sig_info[sig_num].gpio_num, 
           sns_ddf_sig_tb.sig_info[sig_num].warning_cnt); 
  }
  if (!sns_ddf_sig_tb.sig_info[sig_num].int_fired)
  {
    MSG_3(MSG_SSID_SNS,DBG_HIGH_PRIO,"sig_post - idx(%d) gpio(%d) ts(%u)", 
          sig_num, sns_ddf_sig_tb.sig_info[sig_num].gpio_num, 
          sns_ddf_sig_tb.sig_info[sig_num].timestamp); 
  }
#endif
}

/*===========================================================================

  FUNCTION:   get_bit_position

===========================================================================*/
/*!
  @brief This function returns the position of lowest bit set 

  @detail

  @param[i]  none
  @return   none
 */
/*=========================================================================*/
static uint32_t get_bit_position (uint32_t flags)
{
  uint32_t bit_pos = 0;

  while (!(flags & 1)) 
  {   
    flags >>= 1;
    ++bit_pos;
  }
  return bit_pos;
}

/*===========================================================================

  FUNCTION:   sns_ddf_signal_dispatch

===========================================================================*/
/*!
  @brief see the description in the header file

 */
/*=========================================================================*/
void sns_ddf_signal_dispatch (void)
{
#if !defined(SNS_PCSIM) && !defined(SNS_QDSP_SIM)
  uint32_t flags;

  //MSG(MSG_SSID_QDSP6,DBG_HIGH_PRIO,"In DDF Signal Dispatch"); 
  while (0 != (flags = sns_ddf_sig_tb.pending_flags))
  {
    uint32_t sig_num;
    uint32_t timestamp;
    sig_num = get_bit_position(flags);    /* will return the bit position which is set */
    sns_profiling_log_qdss(SNS_SMGR_DD_ENTER_HANDLE_IRQ, 1, sns_ddf_sig_tb.sig_info[sig_num].gpio_num);
    timestamp = sns_ddf_sig_tb.sig_info[sig_num].timestamp;
    if ( sns_latency.ltcy_measure_enabled )
    {
      /* latency measurement under DRI mode */
      sns_latency.dri_get_data_start_ts = sns_em_get_timestamp(); 
    }
    if (!sns_ddf_sig_tb.sig_info[sig_num].int_fired)
    {
      sns_ddf_sig_tb.sig_info[sig_num].int_fired = true;
      MSG_3(MSG_SSID_SNS,DBG_HIGH_PRIO,"sig_dispatch - idx(%d) gpio(%d) ts(%u)", 
            sig_num, sns_ddf_sig_tb.sig_info[sig_num].gpio_num, timestamp); 
    }
#if 1
    if (sns_ddf_sig_tb.sig_info[sig_num].prev_timestamp == timestamp)
    {
      MSG_2(MSG_SSID_SNS, DBG_ERROR_PRIO,
            "sig_dispatch - idx(%d) repeated ts(%u)", sig_num, timestamp);
      continue;
    }
    sns_ddf_sig_tb.sig_info[sig_num].prev_timestamp = timestamp;
#endif
    qurt_atomic_clear_bit(&sns_ddf_sig_tb.pending_flags, sig_num);
    sns_ddf_sig_tb.sig_info[sig_num].driver->handle_irq(
          sns_ddf_sig_tb.sig_info[sig_num].handle,
          sns_ddf_sig_tb.sig_info[sig_num].gpio_num,
          timestamp);
    sns_profiling_log_qdss(SNS_SMGR_DD_EXIT, 1, SNS_QDSS_DD_HANDLE_IRQ);
  }
  return;
#elif defined(SNS_QDSP_SIM)
  (void)get_bit_position(1);    /* To avoid compiler warnings */
  sns_dd_qdsp_playback_if.handle_irq(NULL,0,0);
  return;
#endif  
}


/*===========================================================================

  FUNCTION:   sns_ddf_signal_gpio_read

===========================================================================*/
/*!
  @brief  Read GPIO pin
  @detail

  @param[i] gpio number
  @param[0] gpio status
  @return
   DAL_SUCCESS -- GPIO was successfully configured as requested.\n
   DAL_ERROR -- Either an invalid GPIO number in the configuration or ownership
   cannot be obtained from a secure root.
 */
/*=========================================================================*/
sns_ddf_status_e sns_ddf_signal_gpio_read(uint16_t gpio_num, uint32_t *gpio_status)
{
#if !defined(SNS_PCSIM)
    uint32_t gpio_config;
    DALGpioValueType gpio_val;

    DALSYS_SyncEnter(sns_ddf_sig_tb.sync_handle);
  
    gpio_config = sns_ddf_signal_gpio_config(gpio_num);

    if ( DAL_ERROR == DalTlmm_GpioIn(sns_ddf_sig_tb.tlmm_handle,
                                                gpio_config, 
                                                &gpio_val) )
    {
      //log error;
      DALSYS_SyncLeave(sns_ddf_sig_tb.sync_handle);
      return SNS_DDF_EFAIL;
    }

    *gpio_status = (uint32_t)gpio_val;

    DALSYS_SyncLeave(sns_ddf_sig_tb.sync_handle);
#endif
    return SNS_DDF_SUCCESS;  
}

/******************************************************************************
  @file    qcrili.h
  @brief   qcril qmi - internal interfaces

  DESCRIPTION

  ---------------------------------------------------------------------------

  Copyright (c) 2012-2014 Qualcomm Technologies, Inc.  All Rights Reserved.
  Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------
******************************************************************************/


#ifndef QCRILI_H
#define QCRILI_H

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "ril.h"
#include "ril_cdma_sms.h"
#include "IxErrno.h"
#include "qcril_uim_srvc.h"
#include "qcril_log.h"
#include <pthread.h>
#include <stdint.h>

#ifdef QCRIL_PROTOBUF_BUILD_ENABLED
#include "imsIF.pb-c.h"
#else
//#include "imsIF.without-pb-c.h"
#endif
#ifndef QMI_RIL_UTF
#include "qcril_qmi_ims_if_pb.h"
#else
#define ERANGE      34
int errno;
#endif
/*===========================================================================

                   EXTERNAL DEFINITIONS AND TYPES

===========================================================================*/

enum
{
    QMI_RIL_FEATURE_FUSION_CSFB,
    QMI_RIL_FEATURE_DSDA,
    QMI_RIL_FEATURE_DSDS,
    QMI_RIL_FEATURE_TSTS,
    QMI_RIL_FEATURE_APQ,
    QMI_RIL_FEATURE_MSM,
    QMI_RIL_FEATURE_SVLTE2,
    QMI_RIL_FEATURE_SGLTE,
    QMI_RIL_FEATURE_SGLTE2,
    QMI_RIL_FEATURE_SGLTE_CSFB,
    QMI_RIL_FEATURE_KDDI_HOLD_ANSWER,
    QMI_RIL_FEATURE_8960,
    QMI_RIL_FEATURE_ICS,
    QMI_RIL_FEATURE_IMS,
    QMI_RIL_FEATURE_VOIP_VT,
    QMI_RIL_FEATURE_7627A,
    QMI_RIL_FEATURE_JB,
    QMI_RIL_FEATURE_KK,
    QMI_RIL_FEATURE_8974,
    QMI_RIL_FEATURE_8226,
    QMI_RIL_FEATURE_8610,
    QMI_RIL_FEATURE_IMS_RETRY_3GPP,
    QMI_RIL_FEATURE_IMS_RETRY_3GPP2,
    QMI_RIL_FEATURE_PLAIN_ANDROID,
    QMI_RIL_FEATURE_OEM_SOCKET,
    QMI_RIL_FEATURE_OEM_IND_TO_BOTH,
    QMI_RIL_FEATURE_DSDA2,
    QMI_RIL_FEATURE_TDSCDMA_UI,
    QMI_RIL_FEATURE_POSIX_CLOCKS,
    QMI_RIL_FEATURE_LEGACY_RAT,
    QMI_RIL_FEATURE_COMBINE_RAT,
    QMI_RIL_FEATURE_SAP_SILENT_PIN_VERIFY
};

#define QCRIL_FEATURE_KDDI_HOLD_ANSWER_ON "persist.radio.kddi_hold_answ_on"
#define QMI_RIL_SYS_PROP_NAME_MULTI_SIM   "persist.radio.multisim.config"
#define QMI_RIL_SYS_PROP_LENGTH_MULTI_SIM 4

#define QCRIL_FEATURE_SAP_SILENT_PIN_VERIFY "persist.radio.sap_silent_pin"

extern int qmi_ril_is_feature_supported(int feature);
extern int qmi_ril_is_qcom_ril_version_supported(int version);
extern int qmi_ril_is_multi_sim_feature_supported();

#define QCRIL_NW_SCAN_SELECTION_RADIO_SUPPORT() \
    (qmi_ril_is_feature_supported(QMI_RIL_FEATURE_LEGACY_RAT) ||\
    qmi_ril_is_feature_supported(QMI_RIL_FEATURE_COMBINE_RAT))

#define QCRIL_IS_DSDA_COEX_ENABLED() \
    (qmi_ril_is_feature_supported(QMI_RIL_FEATURE_DSDA) &&\
    (qmi_ril_get_process_instance_id() == QCRIL_DEFAULT_INSTANCE_ID))

#define QCRIL_IS_COEX_ENABLED() \
    (qmi_ril_is_feature_supported(QMI_RIL_FEATURE_8960) ||\
    qmi_ril_is_feature_supported(QMI_RIL_FEATURE_APQ) ||\
    qmi_ril_is_feature_supported(QMI_RIL_FEATURE_8974) ||\
    qmi_ril_is_feature_supported(QMI_RIL_FEATURE_8226) ||\
    QCRIL_IS_DSDA_COEX_ENABLED())

#ifdef _MSC_VER
#define QCRIL_SNPRINTF _snprintf
#else
#define QCRIL_SNPRINTF snprintf
#endif

#define QMI_RIL_ZERO                     (0)
#define QMI_RIL_FF                       (0xFF)
#define QMI_RIL_FFFF                     (0xFFFF)
#define QMI_RIL_SINGLE                   (1)
#define QMI_RIL_FOUR_BYTES               (4)

#define QCRIL_HOOK_HEADER_SIZE           16
#define QCRIL_HOOK_OEM_NAME              "QOEMHOOK"

/* Emergency number ECC list System Property */
#define QCRIL_ECC_LIST                   "ril.ecclist"
#define QCRIL_ECC_LIST_1                 "ril1.ecclist"
#define QCRIL_TRACK_HEAP_MEM             "persist.radio.track_heap_mem"

#define QMI_RIL_QMI_RILD_THREAD_NAME                    "rild"
#define QMI_RIL_QMI_MAIN_THREAD_NAME                    "main"
#define QMI_RIL_QMI_CALLBACK_THREAD_NAME                "qmi_cb"
#define QMI_RIL_EVENT_THREAD_NAME                       "event"
#define QMI_RIL_CORE_INIT_KICKER_THREAD_NAME            "core_init"
#define QMI_RIL_CORE_INIT_IMSA_THREAD_NAME              "imsa_init"
#define QMI_RIL_RESUMER_DEFERRED_ACTION_THREAD_NAME     "resumer_def"
#define QMI_RIL_IPC_RECEIVER_THREAD_NAME                "ipc_receiver"
#define QMI_RIL_LTE_COEX_RIVA_THREAD_NAME               "coex_opener"
#define QMI_RIL_VOICE_SPEECH_CODEC_THREAD_NAME          "speech_codec"
#define QMI_RIL_IMS_SOCKET_THREAD_NAME                  "ims_socket"
#define QMI_RIL_OEM_SOCKET_THREAD_NAME                  "oem_socket"
#define QMI_RIL_IMS_PIL_MONITOR_THREAD_NAME             "pil_monitor"

#define QMI_RIL_SYS_PROP_NAME_MULTI_SIM         "persist.radio.multisim.config"
#define QMI_RIL_SYS_PROP_LENGTH_MULTI_SIM       4
#define QMI_RIL_NUM_DUAL_SUB              2
#define QMI_RIL_NUM_TRIPLE_SUB            3
#define QMI_RIL_NUM_SINGLE_ACTIVE         1
#define QMI_RIL_NUM_DUAL_ACTIVE           2

#define QCRIL_RESTORE_MODE_PREF_SUPPORT                 "persist.radio.restore_mode_pref"

#define QCRIL_ARR_SIZE( a ) ( sizeof( ( a ) ) / sizeof( ( a[ 0 ] ) ) )

#define QCRIL_ASSERT( xx_exp ) QCRIL_LOG_ASSERT( xx_exp )

#define QCRIL_MUTEX_LOCK( mutex, log_str )                                                        \
  {                                                                                               \
    pthread_mutex_lock( mutex );                                                                  \
  }

#define QCRIL_MUTEX_UNLOCK( mutex, log_str )                                                      \
  {                                                                                               \
    pthread_mutex_unlock( mutex );                                                                \
  }

/* Most Significant 16 bits are the Instance ID + Modem ID and Least Significant 16 bits are the user data */
#define QCRIL_COMPOSE_USER_DATA( instance_id, modem_id, user_data )                               \
  ( ( ( instance_id & 0xFF ) << 24 ) | ( ( modem_id & 0xFF ) << 16 ) | ( user_data & 0xFFFF ) )

#define QCRIL_EXTRACT_INSTANCE_ID_FROM_USER_DATA( user_data )                                     \
  ( ( user_data & 0xFF000000 ) >> 24 )

#define QCRIL_EXTRACT_MODEM_ID_FROM_USER_DATA( user_data )                                        \
  ( ( user_data & 0xFF0000 ) >> 16 )

#define QCRIL_EXTRACT_USER_ID_FROM_USER_DATA( user_data )                                         \
  ( user_data & 0xFFFF )

#define QCRIL_NOTUSED( var ) ((var) == (var))
/* Internal Token ID */
#define QCRIL_TOKEN_ID_INTERNAL         0xFFFF

/* Internal Request ID */
#define QCRIL_REQ_ID_INTERNAL           0xFFFF

/* QCRIL Instance ID */
typedef enum
{
  QCRIL_DEFAULT_INSTANCE_ID = 0,
  QCRIL_SECOND_INSTANCE_ID  = 1,
  QCRIL_THIRD_INSTANCE_ID  = 2,
  QCRIL_MAX_INSTANCE_ID
} qcril_instance_id_e_type;

/* DSDS QCRIL Instance Pair */
#define QCRIL_DSDS_INSTANCE_PAIR( instance_id ) ( ( instance_id + 1 ) % 2 )

#define QCRIL_MAX_MODEM_NUM 2
#define QCRIL_MAX_MODEM_ID ( (qmi_ril_is_feature_supported(QMI_RIL_FEATURE_SVLTE2) || qmi_ril_is_feature_supported(QMI_RIL_FEATURE_FUSION_CSFB) ) ? 2 : 1)

/* QCRIL Modem ID */
typedef enum
{
  QCRIL_DEFAULT_MODEM_ID = 0,   /* MSM */
  QCRIL_SECOND_MODEM_ID  = 1,   /* MDM */
} qcril_modem_id_e_type;

/* QCRIL Event Data Type */
typedef enum
{
  QCRIL_DATA_NOT_ON_STACK = 0,
  QCRIL_DATA_ON_STACK     = 1
} qcril_data_src_e_type;

/* Modem states */
typedef enum
{
  QCRIL_MODEM_STATE_UNAVAILABLE, /*!< Modem is resetting or booting */
  QCRIL_MODEM_STATE_OFF,         /*!< Modem is in low power mode */
  QCRIL_MODEM_STATE_ON           /*!< Modem is in online mode */
} qcril_modem_state_e_type;

/* SIM or RUIM states */
typedef enum
{
  QCRIL_SIM_STATE_ABSENT                  = 0,
  QCRIL_SIM_STATE_NOT_READY               = 1,
  QCRIL_SIM_STATE_READY                   = 2,
  QCRIL_SIM_STATE_PIN                     = 3,
  QCRIL_SIM_STATE_PUK                     = 4,
  QCRIL_SIM_STATE_NETWORK_PERSONALIZATION = 5,
  QCRIL_SIM_STATE_CARD_ERROR              = 6,
  QCRIL_SIM_STATE_ILLEGAL                 = 7
} qcril_sim_state_e_type;

/* Radio technology */
typedef enum
{
  QCRIL_RADIO_TECH_NONE   = 0, /*!< Indicates that modem is not on any system yet */
  QCRIL_RADIO_TECH_3GPP   = 1, /*!< Indicates that modem is on 1XEVDO system */
  QCRIL_RADIO_TECH_3GPP2  = 2, /*!< Indicates that modem is on GWL system */
  QCRIL_RADIO_TECH_GLOBAL = 3  /*!< Indicates that modem is on global mode but no service */
} qcril_radio_tech_e_type;

/* Card status */
typedef enum
{
  QCRIL_CARD_STATUS_UNKNOWN          = 0, /*!< Internal use only */
  QCRIL_CARD_STATUS_NOT_APPLICABLE   = 1, /*!< Indicates that NV is the only subscription source */
  QCRIL_CARD_STATUS_DOWN             = 2, /*!< Indicates that card had been power down */
  QCRIL_CARD_STATUS_UP               = 3, /*!< Indicates that card has been power up */
  QCRIL_CARD_STATUS_NOT_ACCESSIBLE   = 4, /*!< Indicates that card is not accessible */
  QCRIL_CARD_STATUS_REFRESH          = 5, /*!< Indicates that card refresh */
  QCRIL_CARD_STATUS_POWERUP_FAILED   = 6, /*!< Indicates that card powerup failed */
  QCRIL_CARD_STATUS_POWERDOWN_FAILED = 7, /*!< Indicates that card powerdown failure */
  QCRIL_CARD_STATUS_ILLEGAL          = 8, /*!< Indicates that card is illegal, for internal use */
  QCRIL_CARD_STATUS_ABSENT           = 9, /*!< Indicates that card is absent */
} qcril_card_status_e_type;

/* Provision status */
typedef enum
{
  QCRIL_PROVISION_STATUS_FAILURE  = 0, /*!< Provision success */
  QCRIL_PROVISION_STATUS_SUCCESS  = 1,  /*!< Provision failure */
  QCRIL_PROVISION_STATUS_IN_PROGRESS = 2 /*!< Provision in progress */
} qcril_provision_status_e_type;

/* Unsolicited notification status */
typedef enum
{
  QCRIL_UNSOL_NOTIFICATION_STATUS_NO_CHANGE, /* Indicates that no change in unsolicited notification status */
  QCRIL_UNSOL_NOTIFICATION_STATUS_ON,        /* Indicates that enabled unsolicited notification */
  QCRIL_UNSOL_NOTIFICATION_STATUS_OFF        /* Indicates that disabled unsolicited notification */
} qcril_unsol_notification_status_e_type;

/* Serving system info changed notification status */
typedef enum
{
  QCRIL_SSIC_NOTIFICATION_STATUS_NO_CHANGE,   /* Indicates that no need to send unsolicited network state changed notification */
  QCRIL_SSIC_NOTIFICATION_STATUS_INFO,        /* SS_INFO, may need to send unsolicited network state changed notification */
  QCRIL_SSIC_NOTIFICATION_STATUS_RSSI,        /* RSSI, may need to send unsolicited network state changed notification */
  QCRIL_SSIC_NOTIFICATION_STATUS_SRV_CHANGED, /* SRV_CHANGED, may need to send unsolicited network state changed notification */
  QCRIL_SSIC_NOTIFICATION_STATUS_REG_REJECT   /* REG-REJECT, may need to send unsolicited network state changed notification */
} qcril_ssic_notification_status_e_type;

typedef enum
{
    QMI_RIL_GEN_OPERATIONAL_STATUS_UNKNOWN = 0,
    QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_PENDING,
    QMI_RIL_GEN_OPERATIONAL_STATUS_INIT_ONGOING,
    QMI_RIL_GEN_OPERATIONAL_STATUS_UNRESTRICTED,
    QMI_RIL_GEN_OPERATIONAL_STATUS_SUSPENDING,
    QMI_RIL_GEN_OPERATIONAL_STATUS_SUSPENDED,
    QMI_RIL_GEN_OPERATIONAL_STATUS_RESUME_PENDING,
    QMI_RIL_GEN_OPERATIONAL_STATUS_RESUMING,
    QMI_RIL_GEN_OPERATIONAL_STATUS_RESUME_RETRY,
    QMI_RIL_GEN_OPERATIONAL_STATUS_HALTED
} qmi_ril_gen_operational_status_type;

typedef enum
{
    QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_NONE,
    QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_VT,
    QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_EMBMS,
    QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_PRESENCE,
    QMI_RIL_OEM_HOOK_QMI_TUNNELING_SERVICE_QTUNER
} qmi_ril_oem_hook_qmi_tunneling_service_id_type;

/*! @brief Card status for a Slot
*/
typedef struct
{
  int slot;                              /* Slot where the card resides */
  qcril_card_status_e_type status;       /* Status of the card */
} qcril_card_info_type;

/*! @brief Slot IDs List
*/
typedef struct
{
  uint8 num_of_slots;
  uint8 slot_id[ QMI_UIM_MAX_CARD_COUNT ];
} qcril_slot_ids_list_type;

/*! @brief Provision status
*/
typedef struct
{
  qcril_provision_status_e_type status;  /* Status of the provision */
  qmi_uim_session_type     session_type; /* UIM session type on which this subscription is currently provisioned */
  RIL_Errno err_code;
} qcril_provision_info_type;

/*! @brief Modem IDs List
*/
typedef struct
{
  uint8 num_of_modems;
  qcril_modem_id_e_type modem_id[ QCRIL_MAX_MODEM_NUM ];
} qcril_modem_ids_list_type;

/* RIL info */
typedef struct
{
  boolean pri_gw_sim_state_changed;                                    /*!< Indicates if primary GW SIM state changed */
  boolean pri_cdma_sim_state_changed;                                  /*!< Indicates if primary CDMA SIM state changed */
  boolean sec_gw_sim_state_changed;                                    /*!< Indicates if secondary GW SIM state changed */
  boolean sec_cdma_sim_state_changed;                                  /*!< Indicates if secondary CDMA SIM state changed */
  boolean ter_gw_sim_state_changed;                                    /*!< Indicates if tertiary GW SIM state changed */
  boolean ter_cdma_sim_state_changed;                                  /*!< Indicates if tertiary CDMA SIM state changed */
  qcril_modem_state_e_type next_modem_state;                           /*!< Next Modem state if modem_state_changed is set */
  qcril_sim_state_e_type next_pri_gw_sim_state;                        /*!< Next primary GW SIM state if gw_sim_state_changed is set */
  qcril_sim_state_e_type next_pri_cdma_sim_state;                      /*!< Next primary CDMA SIM state if cdma_sim_state_changed is set */
  qcril_sim_state_e_type next_sec_gw_sim_state;                        /*!< Next secondary GW SIM state if gw_sim_state_changed is set */
  qcril_sim_state_e_type next_sec_cdma_sim_state;                      /*!< Next secondary CDMA SIM state if cdma_sim_state_changed is set */
  qcril_sim_state_e_type next_ter_gw_sim_state;                        /*!< Next tertiary GW SIM state if ter_gw_sim_state_changed is set */
  qcril_sim_state_e_type next_ter_cdma_sim_state;                      /*!< Next tertiary CDMA SIM state if ter_cdma_sim_state_changed is set */

} qcril_request_return_type;

/* AMDD event IDs */
typedef enum
{
  /* 0 - 0xFFFF reserved for RIL requests defined in ril.h */
  QCRIL_EVT_BASE                               = 0x10000,

  /* AMSS(CM) to QCRIL(CM) events */
  QCRIL_EVT_CM_BASE                            = 0x10000,
  QCRIL_EVT_CM_COMMAND_CALLBACK,

  QCRIL_EVT_CM_PH_BASE                         = 0x11000,
  QCRIL_EVT_CM_SS_BASE                         = 0x12000,

  QCRIL_EVT_CM_CALL_BASE                       = 0x13000,

  QCRIL_EVT_CM_INBAND_BASE                     = 0x14000,
  QCRIL_EVT_CM_SUPS_BASE                       = 0x15000,

  QCRIL_EVT_CM_STATS_BASE                      = 0x16000,

  /* Other QCRIL sub-modules to QCRIL(CM) events */
  QCRIL_EVT_CM_INTERNAL_BASE                   = 0x17000,
  QCRIL_EVT_CM_CARD_STATUS_UPDATED,
  QCRIL_EVT_CM_ACTIVATE_PROVISION_STATUS,
  QCRIL_EVT_CM_DEACTIVATE_PROVISION_STATUS,
  QCRIL_EVT_CM_UPDATE_FDN_STATUS,
  QCRIL_EVT_CM_MAX,

  /* Internal events */
  QCRIL_EVT_SMS_BASE                              = 0x20000,
  QCRIL_EVT_SMS_RAW_READ                          = QCRIL_EVT_SMS_BASE + 1,
  QCRIL_EVT_SMS_PERFORM_INITIAL_CONFIGURATION     = QCRIL_EVT_SMS_BASE + 2,
  QCRIL_EVT_SMS_MAX,

  /* AMSS(UIM) to QCRIL(UIM) events */
  QCRIL_EVT_MMGSDI_BASE                        = 0x30000,

  QCRIL_EVT_UIM_QMI_COMMAND_CALLBACK,
  QCRIL_EVT_UIM_QMI_INDICATION,

  QCRIL_EVT_MMGSDI_IMSI_COMMAND_CALLBACK,

  /* Other QCRIL components to QCRIL(UIM) events */
  QCRIL_EVT_MMGSDI_INTERNAL_BASE              = 0x31000,
  QCRIL_EVT_INTERNAL_UIM_VERIFY_PIN_COMMAND_CALLBACK,
  QCRIL_EVT_INTERNAL_MMGSDI_CARD_POWER_UP,
  QCRIL_EVT_INTERNAL_MMGSDI_CARD_POWER_DOWN,
  QCRIL_EVT_INTERNAL_MMGSDI_GET_FDN_STATUS,
  QCRIL_EVT_INTERNAL_MMGSDI_SET_FDN_STATUS,
  QCRIL_EVT_INTERNAL_MMGSDI_GET_PIN1_STATUS,
  QCRIL_EVT_INTERNAL_MMGSDI_SET_PIN1_STATUS,
  QCRIL_EVT_INTERNAL_MMGSDI_FDN_PBM_RECORD_UPDATE,
  QCRIL_EVT_INTERNAL_MMGSDI_READ_UST_VALUE,
  QCRIL_EVT_INTERNAL_MMGSDI_ACTIVATE_SUBS,
  QCRIL_EVT_INTERNAL_MMGSDI_DEACTIVATE_SUBS,
  QCRIL_EVT_INTERNAL_MMGSDI_MODEM_RESTART_START,
  QCRIL_EVT_INTERNAL_MMGSDI_MODEM_RESTART_COMPLETE,
  QCRIL_EVT_MMGSDI_MAX,

  /* AMSS(GSTK) to QCRIL(GSTK) events */
  QCRIL_EVT_GSTK_BASE                          = 0x40000,

  QCRIL_EVT_GSTK_QMI_CAT_INDICATION,
  QCRIL_EVT_GSTK_QMI_RAW_COMMAND_CALLBACK,
  QCRIL_EVT_GSTK_QMI_COMMAND_CALLBACK,
  QCRIL_EVT_GSTK_QMI_NOTIFY_CARD_ERROR,

  /* Other QCRIL components to QCRIL(GSTK) events */
  QCRIL_EVT_GSTK_INTERNAL_BASE                 = 0x41000,

  QCRIL_EVT_GSTK_QMI_NOTIFY_RIL_IS_READY,

  QCRIL_EVT_GSTK_MAX,

  /* AMSS(Data) to QCRIL(Data) events */
  QCRIL_EVT_DATA_BASE                          = 0x50000,
  QCRIL_EVT_DATA_COMMAND_CALLBACK,
  QCRIL_EVT_DATA_EVENT_CALLBACK,
  QCRIL_EVT_DATA_WDS_EVENT_CALLBACK,
  QCRIL_EVT_DATA_DSD_EVENT_CALLBACK,
  QCRIL_EVT_DATA_MAX,

  /* AMSS(PBM) to QCRIL(PBM) events */
  QCRIL_EVT_PBM_BASE                           = 0x60000,

  /* Other QCRIL components to QCRIL(PBM) events */
  QCRIL_EVT_PBM_INTERNAL_BASE                  = 0x61000,
  QCRIL_EVT_PBM_CARD_INSERTED,
  QCRIL_EVT_PBM_CARD_INIT_COMPLETED,
  QCRIL_EVT_PBM_CARD_ERROR,
  QCRIL_EVT_PBM_UPDATE_OTA_ECC_LIST,
  QCRIL_EVT_PBM_MAX,

  QCRIL_EVT_OTHER_BASE                         = 0x70000,
  QCRIL_EVT_OTHER_MAX,

  /* OEM Hook events */
  QCRIL_EVT_HOOK_BASE                          = 0x80000,
  QCRIL_EVT_HOOK_NV_READ                       = QCRIL_EVT_HOOK_BASE + 1,
  QCRIL_EVT_HOOK_NV_WRITE                      = QCRIL_EVT_HOOK_BASE + 2,
  QCRIL_EVT_HOOK_DATA_GO_DORMANT               = QCRIL_EVT_HOOK_BASE + 3,
  QCRIL_EVT_HOOK_ME_DEPERSONALIZATION          = QCRIL_EVT_HOOK_BASE + 4,
  QCRIL_EVT_HOOK_SET_TUNE_AWAY                 = QCRIL_EVT_HOOK_BASE + 5,
  QCRIL_EVT_HOOK_GET_TUNE_AWAY                 = QCRIL_EVT_HOOK_BASE + 6,
  QCRIL_EVT_HOOK_SET_PAGING_PRIORITY           = QCRIL_EVT_HOOK_BASE + 7,
  QCRIL_EVT_HOOK_GET_PAGING_PRIORITY           = QCRIL_EVT_HOOK_BASE + 8,
  QCRIL_EVT_HOOK_GET_NEIGHBORING_CELLS_INFO    = QCRIL_EVT_HOOK_BASE + 9,
  QCRIL_EVT_HOOK_INFORM_SHUTDOWN               = QCRIL_EVT_HOOK_BASE + 10,
  QCRIL_EVT_HOOK_SET_CDMA_SUB_SRC_WITH_SPC     = QCRIL_EVT_HOOK_BASE + 11,
  QCRIL_EVT_HOOK_SET_DEFAULT_VOICE_SUB         = QCRIL_EVT_HOOK_BASE + 12,
  QCRIL_EVT_HOOK_SET_LOCAL_CALL_HOLD           = QCRIL_EVT_HOOK_BASE + 13,
  QCRIL_EVT_HOOK_CDMA_AVOID_CUR_NWK            = QCRIL_EVT_HOOK_BASE + 14,
  QCRIL_EVT_HOOK_CDMA_CLEAR_AVOIDANCE_LIST     = QCRIL_EVT_HOOK_BASE + 15,
  QCRIL_EVT_HOOK_CDMA_GET_AVOIDANCE_LIST       = QCRIL_EVT_HOOK_BASE + 16,
  QCRIL_EVT_HOOK_SET_BUILTIN_PLMN_LIST         = QCRIL_EVT_HOOK_BASE + 17,
  QCRIL_EVT_HOOK_PERFORM_INCREMENTAL_NW_SCAN   = QCRIL_EVT_HOOK_BASE + 18,
  QCRIL_EVT_HOOK_ENABLE_ENGINEER_MODE          = QCRIL_EVT_HOOK_BASE + 19,
  QCRIL_EVT_HOOK_REJECT_INCOMING_CALL_CAUSE_21 = QCRIL_EVT_HOOK_BASE + 20,
  QCRIL_EVT_HOOK_SET_MODEM_TEST_MODE           = QCRIL_EVT_HOOK_BASE + 21,
  QCRIL_EVT_HOOK_QUERY_MODEM_TEST_MODE         = QCRIL_EVT_HOOK_BASE + 22,
  QCRIL_EVT_HOOK_GET_AVAILABLE_CONFIGS         = QCRIL_EVT_HOOK_BASE + 23,
  QCRIL_EVT_HOOK_SET_PREFERRED_NETWORK_ACQ_ORDER = QCRIL_EVT_HOOK_BASE + 27,
  QCRIL_EVT_HOOK_GET_PREFERRED_NETWORK_ACQ_ORDER = QCRIL_EVT_HOOK_BASE + 28,
  QCRIL_EVT_HOOK_CLEANUP_LOADED_CONFIGS        = QCRIL_EVT_HOOK_BASE + 31,

  /* generic request id for all VT/Presence/EMBMS requests */
  QCRIL_EVT_HOOK_REQ_GENERIC                   = QCRIL_EVT_HOOK_BASE + 100,

  QCRIL_EVT_HOOK_CSG_PERFORM_NW_SCAN           = QCRIL_EVT_HOOK_BASE + 150,
  QCRIL_EVT_HOOK_CSG_SET_SYS_SEL_PREF          = QCRIL_EVT_HOOK_BASE + 151,
  QCRIL_EVT_HOOK_CSG_GET_SYS_INFO              = QCRIL_EVT_HOOK_BASE + 152,

  QCRIL_EVT_HOOK_GET_SAR_REV_KEY              = QCRIL_EVT_HOOK_BASE + 200,
  QCRIL_EVT_HOOK_SET_TRANSMIT_POWER           = QCRIL_EVT_HOOK_BASE + 201,

  QCRIL_EVT_HOOK_UNSOL_EXTENDED_DBM_INTL       = QCRIL_EVT_HOOK_BASE + 1000,
  QCRIL_EVT_HOOK_UNSOL_CDMA_BURST_DTMF         = QCRIL_EVT_HOOK_BASE + 1001,
  QCRIL_EVT_HOOK_UNSOL_CDMA_CONT_DTMF_START    = QCRIL_EVT_HOOK_BASE + 1002,
  QCRIL_EVT_HOOK_UNSOL_CDMA_CONT_DTMF_STOP     = QCRIL_EVT_HOOK_BASE + 1003,
  QCRIL_EVT_HOOK_UNSOL_CALL_EVT_PROGRESS_INFO_IND = QCRIL_EVT_HOOK_BASE + 1004,
  QCRIL_EVT_HOOK_UNSOL_NSS_RELEASE             = QCRIL_EVT_HOOK_BASE + 1005,
  QCRIL_EVT_HOOK_NEIGHBOR_CELL_INFO_RCVD       = QCRIL_EVT_HOOK_BASE + 1006,
  QCRIL_EVT_HOOK_UNSOL_EUTRA_STATUS         = QCRIL_EVT_HOOK_BASE + 1007,
  QCRIL_EVT_HOOK_UNSOL_LTE_COEX             = QCRIL_EVT_HOOK_BASE + 1008,
  QCRIL_EVT_HOOK_UNSOL_WMS_READY               = QCRIL_EVT_HOOK_BASE + 1009,
  QCRIL_EVT_HOOK_UNSOL_VOICE_SYSTEM_ID         = QCRIL_EVT_HOOK_BASE + 1010,
  QCRIL_EVT_HOOK_UNSOL_INCREMENTAL_NW_SCAN_IND = QCRIL_EVT_HOOK_BASE + 1011,
  QCRIL_EVT_HOOK_UNSOL_ENGINEER_MODE        = QCRIL_EVT_HOOK_BASE + 1012,
  QCRIL_EVT_HOOK_UNSOL_SS_ERROR_CODE           = QCRIL_EVT_HOOK_BASE + 1013,
  QCRIL_EVT_HOOK_UNSOL_PDC_CONFIG              = QCRIL_EVT_HOOK_BASE + 1014,
  QCRIL_EVT_HOOK_UNSOL_AUDIO_STATE_CHANGED     = QCRIL_EVT_HOOK_BASE + 1015,
  QCRIL_EVT_HOOK_UNSOL_PDC_CLEAR_CONFIGS       = QCRIL_EVT_HOOK_BASE + 1017,
  QCRIL_EVT_HOOK_UNSOL_SIM_REFRESH             = QCRIL_EVT_HOOK_BASE + 1016,

  //Others reserved for future use
  QCRIL_EVT_HOOK_UNSOL_CSG_SYS_INFO_IND     = QCRIL_EVT_HOOK_BASE + 1050,
  QCRIL_EVT_HOOK_UNSOL_CSG_NETWORK_REJ_IND  = QCRIL_EVT_HOOK_BASE + 1051,

  QCRIL_EVT_HOOK_UNSOL_PCSCF_ADDR            = QCRIL_EVT_HOOK_BASE + 1060,
  QCRIL_EVT_HOOK_UNSOL_OPERATOR_RESERVED_PCO = QCRIL_EVT_HOOK_BASE + 1061,

  /* generic Id for all VT/Presence/EMBMS hook indications*/
  QCRIL_EVT_HOOK_UNSOL_GENERIC                 = QCRIL_EVT_HOOK_BASE + 1100,

  QCRIL_EVT_HOOK_VT_BASE                       = QCRIL_EVT_HOOK_BASE + 6000,
  QCRIL_EVT_HOOK_VT_DIAL_CALL                  = QCRIL_EVT_HOOK_VT_BASE + 1,
  QCRIL_EVT_HOOK_VT_END_CALL                   = QCRIL_EVT_HOOK_VT_BASE + 2,
  QCRIL_EVT_HOOK_VT_ANSWER_CALL                = QCRIL_EVT_HOOK_VT_BASE + 3,
  QCRIL_EVT_HOOK_VT_GET_CALL_INFO              = QCRIL_EVT_HOOK_VT_BASE + 4,
  QCRIL_EVT_HOOK_VT_UNSOL_CALL_STATUS_IND      = QCRIL_EVT_HOOK_VT_BASE + 5,

  QCRIL_EVT_HOOK_PRESENCE_BASE                   =  QCRIL_EVT_HOOK_BASE + 50,
  QCRIL_EVT_HOOK_IMS_ENABLER_STATE_REQ           =  QCRIL_EVT_HOOK_PRESENCE_BASE + 1,
  QCRIL_EVT_HOOK_IMS_SEND_PUBLISH_REQ            =  QCRIL_EVT_HOOK_PRESENCE_BASE + 2,
  QCRIL_EVT_HOOK_IMS_SEND_PUBLISH_XML_REQ        =  QCRIL_EVT_HOOK_PRESENCE_BASE + 3,
  QCRIL_EVT_HOOK_IMS_SEND_UNPUBLISH_REQ          =  QCRIL_EVT_HOOK_PRESENCE_BASE + 4,
  QCRIL_EVT_HOOK_IMS_SEND_SUBSCRIBE_REQ          =  QCRIL_EVT_HOOK_PRESENCE_BASE + 5,
  QCRIL_EVT_HOOK_IMS_SEND_SUBSCRIBE_XML_REQ      =  QCRIL_EVT_HOOK_PRESENCE_BASE + 6,
  QCRIL_EVT_HOOK_IMS_SEND_UNSUBSCRIBE_REQ        =  QCRIL_EVT_HOOK_PRESENCE_BASE + 7,
  QCRIL_EVT_HOOK_IMS_SET_NOTIFY_FMT_REQ_V01      =  QCRIL_EVT_HOOK_PRESENCE_BASE + 8,
  QCRIL_EVT_HOOK_IMS_GET_NOTIFY_FMT_REQ_V01      =  QCRIL_EVT_HOOK_PRESENCE_BASE + 9,
  QCRIL_EVT_HOOK_IMS_SET_EVENT_REPORT_REQ_V01    =  QCRIL_EVT_HOOK_PRESENCE_BASE + 10,
  QCRIL_EVT_HOOK_IMS_GET_EVENT_REPORT_REQ_V01    =  QCRIL_EVT_HOOK_PRESENCE_BASE + 11,

  QCRIL_EVT_HOOK_IMS_PUBLISH_TRIGGER_IND_V01     = QCRIL_EVT_HOOK_PRESENCE_BASE + 30,
  QCRIL_EVT_HOOK_IMS_NOTIFY_XML_IND_V01          = QCRIL_EVT_HOOK_PRESENCE_BASE + 31,
  QCRIL_EVT_HOOK_IMS_NOTIFY_IND_V01              = QCRIL_EVT_HOOK_PRESENCE_BASE + 32,
  QCRIL_EVT_HOOK_IMS_ENABLER_STATUS_IND          = QCRIL_EVT_HOOK_PRESENCE_BASE + 33,



  QCRIL_EVT_HOOK_EMBMS_BASE                    = QCRIL_EVT_HOOK_BASE + 6100,
  QCRIL_EVT_HOOK_EMBMS_ENABLE                  = QCRIL_EVT_HOOK_EMBMS_BASE + 1,
  QCRIL_EVT_HOOK_EMBMS_DISABLE                 = QCRIL_EVT_HOOK_EMBMS_BASE + 2,
  QCRIL_EVT_HOOK_EMBMS_ACTIVATE_TMGI           = QCRIL_EVT_HOOK_EMBMS_BASE + 3,
  QCRIL_EVT_HOOK_EMBMS_DEACTIVATE_TMGI         = QCRIL_EVT_HOOK_EMBMS_BASE + 4,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_RSSI_IND          = QCRIL_EVT_HOOK_EMBMS_BASE + 5,
  QCRIL_EVT_HOOK_EMBMS_GET_AVAILABLE_TMGI      = QCRIL_EVT_HOOK_EMBMS_BASE + 6,
  QCRIL_EVT_HOOK_EMBMS_GET_ACTIVE_TMGI         = QCRIL_EVT_HOOK_EMBMS_BASE + 7,
  QCRIL_EVT_HOOK_EMBMS_ENABLE_RSSI             = QCRIL_EVT_HOOK_EMBMS_BASE + 8,
  QCRIL_EVT_HOOK_EMBMS_DISABLE_RSSI            = QCRIL_EVT_HOOK_EMBMS_BASE + 9,
  QCRIL_EVT_HOOK_EMBMS_GET_COVERAGE_STATE      = QCRIL_EVT_HOOK_EMBMS_BASE + 10,
  QCRIL_EVT_HOOK_EMBMS_GET_RSSI                = QCRIL_EVT_HOOK_EMBMS_BASE + 11,
  QCRIL_EVT_HOOK_EMBMS_GET_EMBMS_SVC_STATE     = QCRIL_EVT_HOOK_EMBMS_BASE + 12,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_SVC_STATE         = QCRIL_EVT_HOOK_EMBMS_BASE + 14,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_ACTIVE_TMGI       = QCRIL_EVT_HOOK_EMBMS_BASE + 15,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_COVERAGE          = QCRIL_EVT_HOOK_EMBMS_BASE + 16,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_AVAILABLE_TMGI    = QCRIL_EVT_HOOK_EMBMS_BASE + 17,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_OSS_WARNING       = QCRIL_EVT_HOOK_EMBMS_BASE + 18,
  QCRIL_EVT_HOOK_EMBMS_ACTIVATE_DEACTIVATE_TMGI = QCRIL_EVT_HOOK_EMBMS_BASE + 19,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_CELL_INFO_CHANGED = QCRIL_EVT_HOOK_EMBMS_BASE + 20,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_RADIO_STATE_CHANGED = QCRIL_EVT_HOOK_EMBMS_BASE + 21,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_SAI_LIST            = QCRIL_EVT_HOOK_EMBMS_BASE + 22,
  QCRIL_EVT_HOOK_EMBMS_SET_SNTP_TIME             = QCRIL_EVT_HOOK_EMBMS_BASE + 23,
  QCRIL_EVT_HOOK_EMBMS_GET_SIB16_COVERAGE        = QCRIL_EVT_HOOK_EMBMS_BASE + 24,
  QCRIL_EVT_HOOK_EMBMS_UNSOL_SIB16_COVERAGE      = QCRIL_EVT_HOOK_EMBMS_BASE + 25,
  QCRIL_EVT_HOOK_EMBMS_GET_UTC_TIME              = QCRIL_EVT_HOOK_EMBMS_BASE + 26,
  QCRIL_EVT_HOOK_EMBMS_GET_ACTIVE_LOG_PACKET_IDS = QCRIL_EVT_HOOK_EMBMS_BASE + 27,
  QCRIL_EVT_HOOK_EMBMS_DELIVER_LOG_PACKET        = QCRIL_EVT_HOOK_EMBMS_BASE + 28,


  QCRIL_EVT_HOOK_RFPE_BASE                          = QCRIL_EVT_HOOK_BASE + 6200,
  QCRIL_EVT_HOOK_SET_RFM_SCENARIO_REQ               = QCRIL_EVT_HOOK_RFPE_BASE + 1,
  QCRIL_EVT_HOOK_GET_RFM_SCENARIO_REQ               = QCRIL_EVT_HOOK_RFPE_BASE + 2,
  QCRIL_EVT_HOOK_GET_PROVISIONED_TABLE_REVISION_REQ = QCRIL_EVT_HOOK_RFPE_BASE + 3,

  QCRIL_EVT_HOOK_MAX,

  QCRIL_EVT_OEM_BASE                           = 0x90000,
  QCRIL_EVT_OEM_MAX                            = 0x9ffff,

  /* QMI */
  QCRIL_EVT_QMI_VOICE_BASE                     = 0xa0000,
  QCRIL_EVT_QMI_NAS_DSDS_SUBS_FOLLOWUP         = QCRIL_EVT_QMI_VOICE_BASE + 9,
  QCRIL_EVT_QMI_NAS_SIG_STRENGTH_UPDATE        = QCRIL_EVT_QMI_VOICE_BASE + 10,
  QCRIL_EVT_QMI_VOICE_BURST_START_CONT_DTMF    = QCRIL_EVT_QMI_VOICE_BASE + 11,
  QCRIL_EVT_QMI_VOICE_BURST_STOP_CONT_DTMF     = QCRIL_EVT_QMI_VOICE_BASE + 12,
  QCRIL_EVT_QMI_VOICE_PENDING_MNG_CALLS_REQ_FINISHED = QCRIL_EVT_QMI_VOICE_BASE + 13,
  QCRIL_EVT_QMI_VOICE_GET_WAITING_CALL         = QCRIL_EVT_QMI_VOICE_BASE + 14,
  QCRIL_EVT_QMI_VOICE_EMERGENCY_CALL_PENDING   = QCRIL_EVT_QMI_VOICE_BASE + 15,
  QCRIL_EVT_QMI_VOICE_HANDLE_INDICATIONS       = QCRIL_EVT_QMI_VOICE_BASE + 16,
  QCRIL_EVT_QMI_VOICE_HANDLE_COMM_CALLBACKS    = QCRIL_EVT_QMI_VOICE_BASE + 17,
  QCRIL_EVT_QMI_IMSS_HANDLE_COMM_CALLBACKS     = QCRIL_EVT_QMI_VOICE_BASE + 18,
  QCRIL_EVT_QMI_IMSA_HANDLE_INDICATIONS        = QCRIL_EVT_QMI_VOICE_BASE + 19,
  QCRIL_EVT_QMI_IMSA_HANDLE_COMM_CALLBACKS     = QCRIL_EVT_QMI_VOICE_BASE + 20,
  QCRIL_EVT_QMI_NAS_DSDS_SUBS_DEACTIVATE_FOLLOWUP         = QCRIL_EVT_QMI_VOICE_BASE + 21,

  QCRIL_EVT_QMI_NAS_CLEANUP_NW_SEL,
  QCRIL_EVT_QMI_NAS_PASSOVER_NW_SEL_IND,
  QCRIL_EVT_QMI_NAS_HANDLE_INDICATIONS,

  QCRIL_EVT_QMI_DMS_HANDLE_INDICATIONS,

  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_SUSPEND_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_SUSPEND_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_SUSPEND_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_SUSPEND_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_RESUME_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_RESUME_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_RESUME_REQ,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_RESUME_REQ,

  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_SUSPEND_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_SUSPEND_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_SUSPEND_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_SUSPEND_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_PRE_RESUME_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_DATA_RESUME_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_UIM_RESUME_CON,
  QCRIL_EVT_QMI_RIL_MODEM_RESTART_RIL_CORE_FINAL_RESUME_CON,


  QCRIL_EVT_QMI_RIL_SEND_UNSOL_RADIO_STATE_CHANGED,
  QCRIL_EVT_QMI_RIL_CONNECTED_EMEGENCY_CALL_END,
  QCRIL_EVT_QMI_RIL_NAS_EMERGENCY_MODE_ON,
  QCRIL_EVT_QMI_RIL_NAS_EMERGENCY_MODE_OFF,

  QCRIL_EVT_QMI_RIL_CHECK_PRL_VER_CHANGE,
  QCRIL_EVT_QMI_RIL_PRL_VER_FETCH_ATTEMPT,

  QCRIL_EVT_QMI_RIL_EMBMS_ENABLE_DATA_REQ,
  QCRIL_EVT_QMI_RIL_EMBMS_ENABLE_DATA_CON,
  QCRIL_EVT_QMI_RIL_EMBMS_DISABLE_DATA_IND,
  QCRIL_EVT_QMI_RIL_EMBMS_SEND_UNSOL_RADIO_STATE,

  QCRIL_EVT_QMI_RIL_POST_VOICE_RTE_CHANGE_IND,

  QCRIL_EVT_QMI_RIL_ASSESS_EMRGENCY_NUMBER_LIST_DESIGNATED_COUNTRY,

  QCRIL_EVT_QMI_RIL_COMMON_IND_SUBSCRIBE_CONSIDER_ACTION,

  QCRIL_EVT_QMI_RIL_POST_OPRT_ONLINE_ACTION,

  QCRIL_EVT_QMI_RIL_POLL_CELL_INFO_LIST,
  QCRIL_EVT_QMI_RIL_CELL_INFO_LIST_CHANGED_IND,
  QCRIL_EVT_QMI_RIL_ENFORCE_DEFERRED_MODE_PREF_SET,

  QCRIL_EVT_QMI_RIL_PDC_LOAD_CONFIGURATION,
  QCRIL_EVT_QMI_RIL_PDC_SELECT_CONFIGURATION,
  QCRIL_EVT_QMI_RIL_PDC_ACTIVATE_CONFIGURATION,
  QCRIL_EVT_QMI_RIL_PDC_DELETE_CONFIGURATION,
  QCRIL_EVT_QMI_RIL_PDC_LIST_CONFIGURATION,

  QCRIL_EVT_QMI_REQUEST_BASE                  = 0xc0000,
  QCRIL_EVT_QMI_REQUEST_NW_SCAN               = QCRIL_EVT_QMI_REQUEST_BASE + 1,
  QCRIL_EVT_QMI_REQUEST_NW_SELECT             = QCRIL_EVT_QMI_REQUEST_BASE + 2,
  QCRIL_EVT_QMI_REQUEST_POWER_RADIO           = QCRIL_EVT_QMI_REQUEST_BASE + 3,
  QCRIL_EVT_QMI_REQUEST_NEIGHBOR_CELL_INFO    = QCRIL_EVT_QMI_REQUEST_BASE + 4,
  QCRIL_EVT_QMI_REQUEST_SET_SUBS_MODE         = QCRIL_EVT_QMI_REQUEST_BASE + 5,
  QCRIL_EVT_QMI_REQUEST_3GPP2_SUB             = QCRIL_EVT_QMI_REQUEST_BASE + 6,
  QCRIL_EVT_QMI_REQUEST_POWER_WAIT_FOR_CARD_STATUS           = QCRIL_EVT_QMI_REQUEST_BASE + 7,
  QCRIL_EVT_QMI_REQUEST_3GPP2_OP_NAME         = QCRIL_EVT_QMI_REQUEST_BASE + 8,
  QCRIL_EVT_QMI_REQUEST_EMBMS_ENABLE          = QCRIL_EVT_QMI_REQUEST_BASE + 9,
  QCRIL_EVT_QMI_REQUEST_EMBMS_DISABLE         = QCRIL_EVT_QMI_REQUEST_BASE + 10,
  QCRIL_EVT_QMI_REQUEST_MODIFY_INITIATE       = QCRIL_EVT_QMI_REQUEST_BASE + 11,
  QCRIL_EVT_QMI_REQUEST_MODIFY_CONFIRM        = QCRIL_EVT_QMI_REQUEST_BASE + 12,
  QCRIL_EVT_QMI_REQUEST_INIT_ATTACH_APN       = QCRIL_EVT_QMI_REQUEST_BASE + 13,
  QCRIL_EVT_QMI_REQUEST_SET_SYS_SEL_PREF      = QCRIL_EVT_QMI_REQUEST_BASE + 14,

  QCRIL_EVT_IMS_SOCKET_REQ_BASE               = 0xd0000,
  QCRIL_EVT_IMS_SOCKET_IMS_REGISTRATION_STATE = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 1,
  QCRIL_EVT_IMS_SOCKET_REQ_DIAL               = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 2,
  QCRIL_EVT_IMS_SOCKET_REQ_ANSWER             = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 3,
  QCRIL_EVT_IMS_SOCKET_REQ_HANGUP             = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 4,
  QCRIL_EVT_IMS_SOCKET_REQ_LAST_CALL_FAIL_CAUSE = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 5,
  QCRIL_EVT_IMS_SOCKET_REQ_GET_CURRENT_CALLS  = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 6,
  QCRIL_EVT_IMS_SOCKET_REQ_HANGUP_WAITING_OR_BACKGROUND = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 7,
  QCRIL_EVT_IMS_SOCKET_REQ_HANGUP_FOREGROUND_RESUME_BACKGROUND  = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 8,
  QCRIL_EVT_IMS_SOCKET_REQ_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 9,
  QCRIL_EVT_IMS_SOCKET_REQ_CONFERENCE         = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 10,
  QCRIL_EVT_IMS_SOCKET_REQ_EXIT_ECBM          = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 11,
  QCRIL_EVT_IMS_SOCKET_REQ_DTMF               = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 13,
  QCRIL_EVT_IMS_SOCKET_REQ_DTMF_START         = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 14,
  QCRIL_EVT_IMS_SOCKET_REQ_DTMF_STOP          = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 15,
  QCRIL_EVT_IMS_SOCKET_REQ_MODIFY_CALL_INITIATE = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 17,
  QCRIL_EVT_IMS_SOCKET_REQ_MODIFY_CALL_CONFIRM  = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 18,
  QCRIL_EVT_IMS_SOCKET_REQ_QUERY_CLIP           = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 19,
  QCRIL_EVT_IMS_SOCKET_REQ_GET_CLIR             = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 20,
  QCRIL_EVT_IMS_SOCKET_REQ_SET_CLIR             = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 21,
  QCRIL_EVT_IMS_SOCKET_REQ_QUERY_CALL_FORWARD_STATUS = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 22,
  QCRIL_EVT_IMS_SOCKET_REQ_SET_CALL_FORWARD_STATUS = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 23,
  QCRIL_EVT_IMS_SOCKET_REQ_QUERY_CALL_WAITING   = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 24,
  QCRIL_EVT_IMS_SOCKET_REQ_SET_CALL_WAITING     = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 25,
  QCRIL_EVT_IMS_SOCKET_REQ_IMS_REG_STATE_CHANGE = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 26,
  QCRIL_EVT_IMS_SOCKET_REQ_SET_SUPP_SVC_NOTIFICATION = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 27,
  QCRIL_EVT_IMS_SOCKET_REQ_ADD_PARTICIPANT      = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 28,
  QCRIL_EVT_IMS_SOCKET_REQ_QUERY_SERVICE_STATUS = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 29,
  QCRIL_EVT_IMS_SOCKET_REQ_SET_SERVICE_STATUS   = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 30,
  QCRIL_EVT_IMS_SOCKET_REQ_STOP_CONT_DTMF   = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 31,
  QCRIL_EVT_IMS_SOCKET_REQ_SUPP_SVC_STATUS   = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 32,
  QCRIL_EVT_IMS_SOCKET_REQ_CALL_DEFLECTION   = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 33,
  QCRIL_EVT_IMS_SOCKET_REQ_QUERY_VT_CALL_QUALITY = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 34,
  QCRIL_EVT_IMS_SOCKET_REQ_SET_VT_CALL_QUALITY   = QCRIL_EVT_IMS_SOCKET_REQ_BASE + 35,
  QCRIL_EVT_IMS_SOCKET_REQ_MAX,

  QCRIL_EVT_NONE                               = 0xfffff /* Internal use only */
} qcril_evt_e_type;

/* Payload of RIL request or AMSS event */
typedef struct
{
  qcril_instance_id_e_type instance_id;
  qcril_modem_id_e_type modem_id;
  int event_id;
  void *data;
  size_t datalen;
  RIL_Token t;
} qcril_request_params_type;

/* Payload of RIL Request Response */
typedef struct
{
  qcril_instance_id_e_type instance_id;
  RIL_Token t;
  int request_id;
  int android_request_id;
  RIL_Errno ril_err_no;
  void *resp_pkt;
  size_t resp_len;
  const char *logstr;
} qcril_request_resp_params_type;

/* Payload of RIL Unsolicited Response */
typedef struct
{
  int response_id;
  void *resp_pkt;
  size_t resp_len;
  const char *logstr;
  uint8 instance_id;
} qcril_unsol_resp_params_type;

typedef enum {
  QCRIL_SUBS_MODE_1X = 0,
  QCRIL_SUBS_MODE_GW  = 1
} qcril_subs_mode_pref;

#ifndef RIL_REQUEST_SET_SUBSCRIPTION_MODE
#define RIL_REQUEST_SET_SUBSCRIPTION_MODE  10113
#endif

#ifndef RIL_REQUEST_GET_UICC_SUBSCRIPTION
#define RIL_REQUEST_GET_UICC_SUBSCRIPTION  10111
#endif

#ifndef RIL_REQUEST_GET_DATA_SUBSCRIPTION
#define RIL_REQUEST_GET_DATA_SUBSCRIPTION  10112
#endif

#ifdef RIL_REQUEST_SET_UICC_SUBSCRIPTION
// Remove RIL_SUBSCRIPTION_3 when telephony TSTS changes are merged.
#ifndef RIL_SUBSCRIPTION_3
#define RIL_SUBSCRIPTION_3 2
// end ifndef RIL_SUBSCRIPTION_3
#endif
#define FEATURE_QCRIL_DSDS
#else
#define RIL_REQUEST_SET_UICC_SUBSCRIPTION  10109
#define RIL_REQUEST_SET_DATA_SUBSCRIPTION  10110
#define RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED 11041

typedef enum {
  RIL_UICC_SUBSCRIPTION_DEACTIVATE = 0,
  RIL_UICC_SUBSCRIPTION_ACTIVATE = 1
} RIL_UiccSubActStatus;

typedef enum {
  RIL_SUBSCRIPTION_1 = 0,
  RIL_SUBSCRIPTION_2 = 1,
  RIL_SUBSCRIPTION_3 = 2
} RIL_SubscriptionType;

typedef struct {
  int   slot;                        /* 0, 1, ... etc. */
  int   app_index;                   /* array subscriptor from applications[RIL_CARD_MAX_APPS] in
                                        RIL_REQUEST_GET_SIM_STATUS */
  RIL_SubscriptionType  sub_type;    /* Indicates subscription 0 or subscription 1 */
  RIL_UiccSubActStatus  act_status;
} RIL_SelectUiccSub;

#endif /* FEATURE_QCRIL_DSDS */


typedef struct {
  RIL_SelectUiccSub    uicc_subs_info;
  qcril_subs_mode_pref subs_mode_pref;
}qcril_uicc_subs_info_type;

/* Timed Callback information */
struct qcril_timed_callback_info;
typedef struct qcril_timed_callback_info qcril_timed_callback_info;
struct qcril_timed_callback_info
{
  uint32 timer_id;
  void*                     callback;
  void*                     extra_params;
  boolean                   need_free; // indicates if needs to free extra_params when canceling the callback
  qcril_timed_callback_info *next;
  qcril_timed_callback_info *prev;
};

typedef struct qcril_heap_list_info qcril_heap_list_info;
struct qcril_heap_list_info
{
  uint32               mem_id;
  char                 func_name[32];
  void*                addr;
  uint32               line_number;
  size_t               size;
  qcril_heap_list_info *next;
  qcril_heap_list_info *prev;
};

typedef struct
{
    qcril_evt_e_type evt_originator;
    RIL_Errno        result;
} qcril_modem_restart_con_type;


typedef struct
{
    int32 dbg_trace_id;
} qcril_embms_enable_request_payload_type;

#define QCRIL_EMBMS_INTERFACE_NAME_LENGTH_MAX (256)

typedef struct
{
    int     cause;
    uint8   call_id;
    char    interafce_name[ QCRIL_EMBMS_INTERFACE_NAME_LENGTH_MAX ];
    unsigned int if_index;
    int32  dbg_trace_id;
    int32  resp_code;
} qcril_embms_enable_response_payload_type;

typedef struct
{
    int     cause;
    uint8   call_id;
    char    interafce_name[ QCRIL_EMBMS_INTERFACE_NAME_LENGTH_MAX ];
    int32  dbg_trace_id;
    int32  resp_code;
} qcril_embms_disable_indication_payload_type;

typedef struct
{
    RIL_Token t;
    qcril_instance_id_e_type instance_id;
    qcril_request_resp_params_type *data;
} qcril_free_req_and_dispatch_follower_req_payload_type;

typedef struct
{
    uint32      timer_id;
    void        *custom_param;
} qcril_timed_callback_handler_params_type;

typedef void qcril_req_handler_type
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr /*!< Output parameter */
);

/*! Hold general information about RIL_REQUESTs */
typedef struct qcril_dispatch_tag
{
  /*! field to link entries together in hash table */
  struct qcril_dispatch_tag *next_ptr;
  uint32 event_id;
  qcril_req_handler_type *handler;
  uint16 allowed_radio_states_mask;
} qcril_dispatch_table_entry_type;

#ifndef RIL_UNSOL_ON_SS

#define RIL_UNSOL_ON_SS 11038
#define RIL_UNSOL_STK_CC_ALPHA_NOTIFY 11039

#define CALL_FAIL_DIAL_MODIFIED_TO_USSD  244
#define CALL_FAIL_DIAL_MODIFIED_TO_SS  245
#define CALL_FAIL_DIAL_MODIFIED_TO_DIAL 246

typedef enum {
  SS_CFU,
  SS_CF_BUSY,
  SS_CF_NO_REPLY,
  SS_CF_NOT_REACHABLE,
  SS_CF_ALL,
  SS_CF_ALL_CONDITIONAL,
  SS_CLIP,
  SS_CLIR,
  SS_COLP,
  SS_COLR,
  SS_WAIT,
  SS_BAOC,
  SS_BAOIC,
  SS_BAOIC_EXC_HOME,
  SS_BAIC,
  SS_BAIC_ROAMING,
  SS_ALL_BARRING,
  SS_OUTGOING_BARRING,
  SS_INCOMING_BARRING
} RIL_SsServiceType;

typedef enum {
  SS_ACTIVATION,
  SS_DEACTIVATION,
  SS_INTERROGATION,
  SS_REGISTRATION,
  SS_ERASURE
} RIL_SsRequestType;

typedef enum {
  SS_ALL_TELE_AND_BEARER_SERVICES,
  SS_ALL_TELESEVICES,
  SS_TELEPHONY,
  SS_ALL_DATA_TELESERVICES,
  SS_SMS_SERVICES,
  SS_ALL_TELESERVICES_EXCEPT_SMS
} RIL_SsTeleserviceType;

#define SS_INFO_MAX 4
#define NUM_SERVICE_CLASSES 7

typedef struct {
  int numValidIndexes; /* This gives the number of valid values in cfInfo.
                       For example if voice is forwarded to one number and data
                       is forwarded to a different one then numValidIndexes will be
                       2 indicating total number of valid values in cfInfo.
                       Similarly if all the services are forwarded to the same
                       number then the value of numValidIndexes will be 1. */

  RIL_CallForwardInfo cfInfo[NUM_SERVICE_CLASSES]; /* This is the response data
                                                      for SS request to query call
                                                      forward status. see
                                                      RIL_REQUEST_QUERY_CALL_FORWARD_STATUS */
} RIL_CfData;

typedef struct {
  RIL_SsServiceType serviceType;
  RIL_SsRequestType requestType;
  RIL_SsTeleserviceType teleserviceType;
  int serviceClass;
  RIL_Errno result;

  union {
    int ssInfo[SS_INFO_MAX]; /* This is the response data for most of the SS GET/SET
                                RIL requests. E.g. RIL_REQUSET_GET_CLIR returns
                                two ints, so first two values of ssInfo[] will be
                                used for response if serviceType is SS_CLIR and
                                requestType is SS_INTERROGATION */

    RIL_CfData cfData;
  };
} RIL_StkCcUnsolSsResponse;

#define RIL_E_UNUSED                     16
#define RIL_E_DIAL_MODIFIED_TO_USSD      17 /* DIAL request modified to USSD */
#define RIL_E_DIAL_MODIFIED_TO_SS        18 /* DIAL request modified to SS */
#define RIL_E_DIAL_MODIFIED_TO_DIAL      19 /* DIAL request modified to DIAL with different data */
#define RIL_E_USSD_MODIFIED_TO_DIAL      20 /* USSD request modified to DIAL */
#define RIL_E_USSD_MODIFIED_TO_SS        21 /* USSD request modified to SS */
#define RIL_E_USSD_MODIFIED_TO_USSD      22 /* USSD request modified to different USSD request */
#define RIL_E_SS_MODIFIED_TO_DIAL        23 /* SS request modified to DIAL */
#define RIL_E_SS_MODIFIED_TO_USSD        24 /* SS request modified to USSD */
#define RIL_E_SS_MODIFIED_TO_SS          25 /* SS request modified to different SS request */
#define RIL_E_SUBSCRIPTION_NOT_SUPPORTED 26 /* Subscription not supported by RIL */

#endif //RIL_UNSOL_ON_SS

#ifndef RIL_REQUEST_SET_INITIAL_ATTACH_APN
#define RIL_REQUEST_SET_INITIAL_ATTACH_APN 123
typedef struct {
    char *apn;
    char *protocol;
    int authtype;
    char *username;
    char *password;
} RIL_InitialAttachApn;
#endif

#ifndef RIL_E_UNUSED
#define RIL_E_UNUSED 16
#endif

#define RIL_E_SKIP_LTE_REATTACH RIL_E_UNUSED

typedef void (*qcril_timed_callback_type) (qcril_timed_callback_handler_params_type* handler_params);

/*===========================================================================

                    EXTERNAL FUNCTION PROTOTYPES

===========================================================================*/

errno_enum_type qcril_cm_init( void );
boolean qcril_cm_screen_is_off( qcril_instance_id_e_type instance_id );
void qcril_cm_get_modem_capability( void );
void qcril_cm_process_network_info( qcril_instance_id_e_type instance_id, qcril_modem_id_e_type modem_id,
                                    qcril_ssic_notification_status_e_type ssic_notification_status );

void qcril_uim_init( void );
void qcril_uim_release( void );

void qcril_gstk_qmi_init( void );

void qcril_data_init();
void qcril_data_qmi_wds_init(boolean from_ssr);
void qcril_data_qmi_wds_release(void);
void qcril_other_init( void );
void qcril_other_mute( qcril_instance_id_e_type instance_id, boolean mic_mute, boolean ear_mute );
void qcril_event_init( void );
void qcril_event_start( void );
void qcril_event_suspend();
void qcril_event_resume();
IxErrnoType qcril_event_queue( qcril_instance_id_e_type instance_id, qcril_modem_id_e_type modem_id, qcril_data_src_e_type data_src,
                        qcril_evt_e_type event_id, void *data, size_t datalen, RIL_Token t );

errno_enum_type qcril_process_event( qcril_instance_id_e_type instance_id, qcril_modem_id_e_type modem_id,
                                     qcril_evt_e_type event_id, void *data, size_t datalen, RIL_Token t );

int qcril_setup_timed_callback( qcril_instance_id_e_type instance_id, qcril_modem_id_e_type modem_id, RIL_TimedCallback callback,
                                   const struct timeval *relativeTime, uint32 *timer_id );
int qcril_setup_timed_callback_ex_params(     qcril_instance_id_e_type      instance_id,
                                              qcril_modem_id_e_type         modem_id,
                                              qcril_timed_callback_type     callback,
                                              void*                         extra_params,
                                              const struct timeval *        relativeTime,
                                              uint32 *                      timer_id
                                         );

int qcril_setup_timed_callback_ex_params_adv( qcril_instance_id_e_type      instance_id,
                                              qcril_modem_id_e_type         modem_id,
                                              qcril_timed_callback_type     callback,
                                              void*                         extra_params,
                                              boolean                       need_free,
                                              const struct timeval *        relativeTime,
                                              uint32 *                      timer_id
                                            );

int qcril_cancel_timed_callback( void *param );

void qcril_default_request_resp_params( qcril_instance_id_e_type instance_id, RIL_Token t, int request_id, RIL_Errno err_no,
                                       qcril_request_resp_params_type *param_ptr );

void qcril_send_empty_payload_request_response(qcril_instance_id_e_type instance_id, RIL_Token t, int request_id, RIL_Errno ril_err_no);
void qcril_send_request_response( qcril_request_resp_params_type *param_ptr );
void qmi_ril_fw_send_request_response_epilog( qcril_instance_id_e_type instance_id,
                                              RIL_Token token,
                                              int android_request_id,
                                              RIL_Errno resp_cause,
                                              void* resp_data,
                                              uint32 resp_data_len,
                                              int is_abnormal_drop,
                                              qcril_request_resp_params_type *param_ptr_ref );
void qmi_ril_fw_android_request_flow_control_abandon_requests_family_main_thrd( int android_request_id, RIL_Errno cause );
void qmi_ril_fw_android_request_flow_control_abandon_all_requests_main_thrd( RIL_Errno cause );
void qmi_ril_fw_android_request_flow_control_drop_legacy_book_records( int voice_calls_related_only  );

pthread_t qmi_ril_fw_get_main_thread_id();

void qcril_default_unsol_resp_params( qcril_instance_id_e_type instance_id, int response_id, qcril_unsol_resp_params_type *param );
void qcril_send_unsol_response( qcril_unsol_resp_params_type *param_ptr );
void qcril_hook_unsol_response( qcril_instance_id_e_type instance_id, uint32 unsol_event, char *data, uint32 data_len );
#define qcril_malloc(size) qcril_malloc_adv(size, __func__, __LINE__)
void *qcril_malloc_adv( size_t size, const char* func_name, int line_num );
#define qcril_free(mem_ptr) qcril_free_adv(mem_ptr, __func__, __LINE__)
void qcril_free_adv( void *mem_ptr, const char* func_name, int line_num );
void qcril_release( void );
#define QCRIL_EXTERN( xxx_request ) \
    void qcril_##xxx_request ( const qcril_request_params_type *const params_ptr, \
                             qcril_request_return_type *const ret_ptr)
#define QCRIL_QMI_EXTERN( xxx_request ) \
    void qcril_##xxx_request ( const qcril_request_params_type *const params_ptr)

void qcril_qmi_mgr_modem_state_updated(qcril_instance_id_e_type instance_id, qcril_modem_state_e_type new_modem_state);
void qcril_qmi_mgr_voice_technology_updated(qcril_radio_tech_e_type new_voice_technology);
qcril_radio_tech_e_type qcril_qmi_mgr_get_known_voice_technology(void);

qmi_ril_gen_operational_status_type qmi_ril_get_operational_status(void);
void qmi_ril_set_operational_status( qmi_ril_gen_operational_status_type new_status );

/// returns ril instance
qcril_instance_id_e_type qmi_ril_get_process_instance_id(void);

/// get ecclist property name by instance_id
extern void get_ecc_property_name(char *ecc_prop_name);

/// returns sim card slot index for current RIL instance
uint32_t qmi_ril_get_sim_slot(void);

extern int ril_to_uim_is_dsds_enabled(void);
extern int ril_to_uim_is_tsts_enabled(void);

void qcril_modem_restart_confirm_suspend_resume_step( qcril_evt_e_type confirmation_evt, qcril_modem_restart_con_type * details );
void qmi_ril_core_init_enter_warp(void);

void qmi_ril_set_thread_name(pthread_t thread_id, const char *thread_name);
int qmi_ril_get_thread_name(pthread_t thread_id, char *thread_name);
void qmi_ril_clear_thread_name(pthread_t thread_id);

void qmi_ril_enter_critical_section(void);
void qmi_ril_leave_critical_section(void);

void qmi_ril_clear_timed_callback_list(void);

char* qmi_ril_util_str_clone( char * org_str );

void qmi_ril_reset_android_unsol_resp_dispatchable_table();
void qmi_ril_reset_unsol_resp_pending_list();

void qmi_ril_suppress_android_unsol_resp(int resp_id);
void qmi_ril_unsuppress_android_unsol_resp(int resp_id);

void qcril_common_update_current_imsi( char * imsi_str, int is_gwl );

void qcril_qmi_print_hex(unsigned char *msg, int msg_len);

int qmi_ril_retrieve_number_of_rilds();
char* qmi_ril_client_get_master_port(void);

//Stub for the Data Ril Function - to be removed when Data Ril Function is available
RIL_Errno qcril_data_request_set_lte_attach_profile(RIL_InitialAttachApn*);

void qmi_ril_reset_multi_sim_ftr_info( void );

/* Data Services */
QCRIL_EXTERN (data_request_setup_data_call);
QCRIL_EXTERN (data_request_deactivate_data_call);
QCRIL_EXTERN (data_request_last_data_call_fail_cause);
QCRIL_EXTERN (data_request_data_call_list);
QCRIL_EXTERN (data_request_omh_profile_info);
#if (RIL_QCOM_VERSION >= 2)
  QCRIL_EXTERN (data_request_setup_qos);
  QCRIL_EXTERN (data_request_release_qos);
  QCRIL_EXTERN (data_request_modify_qos);
  QCRIL_EXTERN (data_request_suspend_qos);
  QCRIL_EXTERN (data_request_resume_qos);
  QCRIL_EXTERN (data_request_get_qos_status);
#endif /* RIL_QCOM_VERSION >= 2 */
QCRIL_EXTERN (data_command_hdlr);
QCRIL_EXTERN (data_event_hdlr);
#ifdef FEATURE_QCRIL_USE_NETCTRL
  QCRIL_EXTERN (data_wds_event_hdlr);
  QCRIL_EXTERN (data_dsd_event_hdlr);
#endif /*FEATURE_QCRIL_USE_NETCTRL*/
QCRIL_EXTERN (data_process_qcrilhook_go_dormant);
QCRIL_EXTERN (data_embms_enable_data_req);
QCRIL_EXTERN (data_embms_activate_tmgi);
QCRIL_EXTERN (data_embms_deactivate_tmgi);
QCRIL_EXTERN (data_embms_activate_deactivate_tmgi);
QCRIL_EXTERN (data_embms_get_available_tmgi);
QCRIL_EXTERN (data_embms_get_active_tmgi);

/* PBM events internal, external handling*/
QCRIL_EXTERN (pbm_event_handler);
QCRIL_EXTERN (pbm_event_card_state_changed);
QCRIL_EXTERN (pbm_update_ota_ecc_list);

/* CM Services */
QCRIL_EXTERN (cm_phonesvc_request_set_preferred_network_type);
QCRIL_EXTERN (cm_phonesvc_request_get_preferred_network_type);
QCRIL_EXTERN (cm_phonesvc_request_set_network_selection_manual);
QCRIL_EXTERN (cm_phonesvc_request_set_network_selection_automatic);
QCRIL_EXTERN (cm_phonesvc_request_query_network_selection_mode);
QCRIL_EXTERN (cm_phonesvc_request_set_band_mode);
QCRIL_EXTERN (cm_phonesvc_request_query_available_band_mode);
QCRIL_EXTERN (cm_phonesvc_request_query_available_networks);
QCRIL_EXTERN (cm_phonesvc_request_radio_power);
QCRIL_EXTERN (cm_phonesvc_request_reset_radio);
QCRIL_EXTERN (cm_phonesvc_request_cdma_set_subscription);
QCRIL_EXTERN (cm_phonesvc_request_cdma_set_roaming_preference);
QCRIL_EXTERN (cm_phonesvc_request_cdma_query_roaming_preference);
QCRIL_EXTERN (cm_phonesvc_request_exit_emergency_callback_mode);
QCRIL_EXTERN (cm_phonesvc_request_cdma_get_subscription);
QCRIL_EXTERN (cm_phonesvc_request_cdma_prl_version);
#ifdef FEATURE_QCRIL_DSDS
QCRIL_EXTERN (cm_phonesvc_request_set_uicc_subscription_source);
QCRIL_EXTERN (cm_phonesvc_request_set_data_subscription_source);
QCRIL_EXTERN (cm_phonesvc_request_get_uicc_subscription_source);
QCRIL_EXTERN (cm_phonesvc_request_get_data_subscription_source);
#endif /* FEATURE_QCRIL_DSDS */
QCRIL_EXTERN (cm_srvsys_request_voice_radio_tech);
QCRIL_EXTERN (cm_srvsys_request_operator);
QCRIL_EXTERN (cm_srvsys_request_signal_strength);
QCRIL_EXTERN (cm_srvsys_request_registration_state);
QCRIL_EXTERN (cm_srvsys_request_data_registration_state);
QCRIL_EXTERN (cm_srvsys_request_get_neighboring_cell_ids);
QCRIL_EXTERN (cm_srvsys_request_screen_state);
QCRIL_EXTERN (cm_srvsys_request_set_location_updates);
QCRIL_EXTERN (cm_callsvc_request_dial);
QCRIL_EXTERN (cm_callsvc_request_answer);
QCRIL_EXTERN (cm_callsvc_request_hangup);
QCRIL_EXTERN (cm_callsvc_request_get_current_calls );
QCRIL_EXTERN (cm_callsvc_request_last_call_fail_cause);
QCRIL_EXTERN (cm_callsvc_request_cdma_set_preferred_voice_privacy_mode);
QCRIL_EXTERN (cm_callsvc_request_cdma_query_preferred_voice_privacy_mode);
QCRIL_EXTERN (cm_callsvc_request_cdma_flash);
QCRIL_EXTERN (cm_inbandsvc_request_dtmf);
QCRIL_EXTERN (cm_inbandsvc_request_dtmf_start);
QCRIL_EXTERN (cm_inbandsvc_request_dtmf_stop);
QCRIL_EXTERN (cm_inbandsvc_request_cdma_burst_dtmf);
QCRIL_EXTERN (cm_inbandsvc_event_rev_burst_dtmf);
QCRIL_EXTERN (cm_inbandsvc_event_rev_start_cont_dtmf_conf);
QCRIL_EXTERN (cm_inbandsvc_event_rev_stop_cont_dtmf_conf);
QCRIL_EXTERN (cm_inbandsvc_event_fwd_burst_dtmf);
QCRIL_EXTERN (cm_inbandsvc_event_fwd_start_cont_dtmf);
QCRIL_EXTERN (cm_inbandsvc_event_fwd_stop_cont_dtmf);
QCRIL_EXTERN (cm_supsvc_request_udub);
QCRIL_EXTERN (cm_supsvc_request_conference);
QCRIL_EXTERN (cm_supsvc_request_separate_connection);
QCRIL_EXTERN (cm_supsvc_request_explicit_call_transfer);
QCRIL_EXTERN (cm_supsvc_request_switch_waiting_or_holding_and_active);
QCRIL_EXTERN (cm_supsvc_request_hangup_waiting_or_background);
QCRIL_EXTERN (cm_supsvc_request_hangup_foreground_resume_background);
QCRIL_EXTERN (cm_supsvc_request_query_clip);
QCRIL_EXTERN (cm_supsvc_request_set_clir);
QCRIL_EXTERN (cm_supsvc_request_get_clir);
QCRIL_EXTERN (cm_supsvc_request_set_call_forward);
QCRIL_EXTERN (cm_supsvc_request_query_call_waiting);
QCRIL_EXTERN (cm_supsvc_request_set_call_waiting);
QCRIL_EXTERN (cm_supsvc_request_query_call_forward_status);
QCRIL_EXTERN (cm_supsvc_request_change_barring_password);
QCRIL_EXTERN (cm_supsvc_request_set_facility_lock);
QCRIL_EXTERN (cm_supsvc_request_query_facility_lock);
QCRIL_EXTERN (cm_supsvc_request_send_ussd);
QCRIL_EXTERN (cm_supsvc_request_cancel_ussd);
QCRIL_EXTERN (cm_supsvc_request_set_supp_svc_notification);
QCRIL_EXTERN (cm_event_command_callback);
QCRIL_EXTERN (cm_phonesvc_event_oprt_mode);
QCRIL_EXTERN (cm_phonesvc_event_info);
QCRIL_EXTERN (cm_phonesvc_event_sys_sel_pref);
#ifdef FEATURE_QCRIL_DSDS
QCRIL_EXTERN (cm_phonesvc_event_dual_standby_pref);
QCRIL_EXTERN (cm_phonesvc_event_subscription_pref_info);
#endif /* FEATURE_QCRIL_DSDS */
QCRIL_EXTERN (cm_phonesvc_event_prl_init);
QCRIL_EXTERN (cm_phonesvc_event_available_networks_conf);
QCRIL_EXTERN (cm_phonesvc_event_terminate_get_networks);
QCRIL_EXTERN (cm_phonesvc_event_nvruim_config_changed);
QCRIL_EXTERN (cm_srvsys_event_reg_reject);
QCRIL_EXTERN (cm_srvsys_event_info);
QCRIL_EXTERN (cm_srvsys_event_rssi);
QCRIL_EXTERN (cm_srvsys_event_srv_changed);
QCRIL_EXTERN (cm_srvsys_event_emerg_num_list);
#ifdef FEATURE_QCRIL_DSAC
QCRIL_EXTERN (cm_srvsys_event_cell_access_ind);
#endif /* FEATURE_QCRIL_DSAC */
QCRIL_EXTERN (cm_callsvc_event_orig);
QCRIL_EXTERN (cm_callsvc_event_setup_ind);
QCRIL_EXTERN (cm_callsvc_event_user_data_ind);
QCRIL_EXTERN (cm_callsvc_event_progress_info_ind);
QCRIL_EXTERN (cm_callsvc_event_incom);
QCRIL_EXTERN (cm_callsvc_event_answer);
QCRIL_EXTERN (cm_callsvc_event_end);
QCRIL_EXTERN (cm_callsvc_event_connect);
QCRIL_EXTERN (cm_callsvc_event_sups);
QCRIL_EXTERN (cm_callsvc_event_privacy);
QCRIL_EXTERN (cm_callsvc_event_privacy_pref);
QCRIL_EXTERN (cm_callsvc_event_caller_id);
QCRIL_EXTERN (cm_callsvc_event_signal);
QCRIL_EXTERN (cm_callsvc_event_display);
QCRIL_EXTERN (cm_callsvc_event_called_party);
QCRIL_EXTERN (cm_callsvc_event_connected_num);
QCRIL_EXTERN (cm_callsvc_event_ext_display);
QCRIL_EXTERN (cm_callsvc_event_nss_clir);
QCRIL_EXTERN (cm_callsvc_event_nss_rel);
QCRIL_EXTERN (cm_callsvc_event_nss_aud_ctrl);
QCRIL_EXTERN (cm_callsvc_event_redirecting_number);
QCRIL_EXTERN (cm_callsvc_event_line_ctrl);
QCRIL_EXTERN (cm_callsvc_event_mng_calls_conf);
QCRIL_EXTERN (cm_callsvc_event_orig_fwd_status);
QCRIL_EXTERN (cm_callsvc_event_call_being_forwarded);
QCRIL_EXTERN (cm_callsvc_event_call_is_waiting);
QCRIL_EXTERN (cm_callsvc_event_call_barred);
QCRIL_EXTERN (cm_callsvc_event_call_restricted);
QCRIL_EXTERN (cm_callsvc_event_incom_fwd_call);
QCRIL_EXTERN (cm_callsvc_event_cug_info_received);
QCRIL_EXTERN (cm_callsvc_event_call_on_hold);
QCRIL_EXTERN (cm_callsvc_event_call_retrieved);
QCRIL_EXTERN (cm_callsvc_event_call_forwarded);
QCRIL_EXTERN (cm_callsvc_event_transferred_call);
QCRIL_EXTERN (cm_callsvc_event_call_deflection);
QCRIL_EXTERN (cm_callsvc_event_cnap_info_received);
QCRIL_EXTERN (cm_callsvc_event_otasp_status);
QCRIL_EXTERN (cm_supsvc_event_activate_conf);
QCRIL_EXTERN (cm_supsvc_event_deactivate_conf);
QCRIL_EXTERN (cm_supsvc_event_erase_conf);
QCRIL_EXTERN (cm_supsvc_event_interrogate_conf);
QCRIL_EXTERN (cm_supsvc_event_register_conf);
QCRIL_EXTERN (cm_supsvc_event_reg_password_conf);
QCRIL_EXTERN (cm_supsvc_event_process_uss_conf);
QCRIL_EXTERN (cm_supsvc_event_fwd_check_ind);
QCRIL_EXTERN (cm_supsvc_event_uss_notify_ind);
QCRIL_EXTERN (cm_supsvc_event_uss_ind);
QCRIL_EXTERN (cm_supsvc_event_release_uss_ind);
QCRIL_EXTERN (cm_supsvc_event_get_password_ind);
QCRIL_EXTERN (cm_callsvc_event_ext_brst_intl);
QCRIL_EXTERN (cm_stats_event_modem_info);
QCRIL_EXTERN (cm_event_card_status_updated);
#ifdef FEATURE_QCRIL_DSDS
QCRIL_EXTERN (cm_event_activate_provision_status);
QCRIL_EXTERN (cm_event_deactivate_provision_status);
#endif /* FEATURE_QCRIL_DSDS */
QCRIL_EXTERN (cm_event_update_fdn_status);

/* UIM/SIM Toolkit */
QCRIL_EXTERN (gstk_qmi_process_qmi_indication);
QCRIL_EXTERN (gstk_qmi_process_notify_ril_is_ready);
QCRIL_EXTERN (gstk_qmi_request_stk_get_profile);
QCRIL_EXTERN (gstk_qmi_request_stk_set_profile);
QCRIL_EXTERN (gstk_qmi_request_stk_send_envelope_command);
QCRIL_EXTERN (gstk_qmi_request_stk_send_terminal_response);
QCRIL_EXTERN (gstk_qmi_request_stk_handle_call_setup_requested_from_sim);
QCRIL_EXTERN (gstk_qmi_request_stk_service_is_running);
QCRIL_EXTERN (gstk_qmi_process_raw_command_callback);
QCRIL_EXTERN (gstk_qmi_process_qmi_response);
QCRIL_EXTERN (gstk_qmi_request_stk_send_envelope_with_status);
QCRIL_EXTERN (gstk_qmi_process_card_error);

QCRIL_EXTERN (uim_process_qmi_callback);
QCRIL_EXTERN (uim_process_qmi_indication);
QCRIL_EXTERN (uim_process_internal_command);
QCRIL_EXTERN (uim_process_imsi_callback);
QCRIL_EXTERN (uim_request_get_sim_status);
QCRIL_EXTERN (uim_request_enter_pin);
QCRIL_EXTERN (uim_request_enter_puk);
QCRIL_EXTERN (uim_request_change_pin);
QCRIL_EXTERN (uim_request_enter_perso_key);
QCRIL_EXTERN (uim_request_get_imsi);
QCRIL_EXTERN (uim_request_sim_io);
QCRIL_EXTERN (uim_request_isim_authenticate);
#ifdef FEATURE_QCRIL_UIM_QMI_APDU_ACCESS
QCRIL_EXTERN (uim_request_send_apdu);
QCRIL_EXTERN (uim_request_logical_channel);
#endif /* FEATURE_QCRIL_UIM_QMI_APDU_ACCESS */
#ifdef FEATURE_QCRIL_UIM_QMI_GET_ATR
QCRIL_EXTERN (uim_request_get_atr);
#endif /* FEATURE_QCRIL_UIM_QMI_GET_ATR */

/* SMS (WMS) */
QCRIL_EXTERN (sms_request_send_sms);
QCRIL_EXTERN (sms_request_send_sms_expect_more);
QCRIL_EXTERN (sms_request_sms_acknowledge);
QCRIL_EXTERN (sms_request_write_sms_to_sim);
QCRIL_EXTERN (sms_request_delete_sms_on_sim);
QCRIL_EXTERN (sms_request_get_smsc_address);
QCRIL_EXTERN (sms_request_set_smsc_address);
QCRIL_EXTERN (sms_request_report_sms_memory_status);
QCRIL_EXTERN (sms_request_gsm_get_broadcast_sms_config);
QCRIL_EXTERN (sms_request_gsm_set_broadcast_sms_config);
QCRIL_EXTERN (sms_request_gsm_sms_broadcast_activation);
QCRIL_EXTERN (sms_request_cdma_send_sms);
QCRIL_EXTERN (sms_request_cdma_sms_acknowledge);
QCRIL_EXTERN (sms_request_cdma_write_sms_to_ruim);
QCRIL_EXTERN (sms_request_cdma_delete_sms_on_ruim);
QCRIL_EXTERN (sms_request_cdma_get_broadcast_sms_config);
QCRIL_EXTERN (sms_request_cdma_set_broadcast_sms_config);
QCRIL_EXTERN (sms_request_cdma_sms_broadcast_activation);
QCRIL_EXTERN (sms_request_ims_registration_state);
QCRIL_EXTERN (sms_request_ims_send_sms);
QCRIL_EXTERN (sms_command_event_callback);
QCRIL_EXTERN (sms_msg_event_send);
QCRIL_EXTERN (sms_msg_event_submit_report);
QCRIL_EXTERN (sms_msg_event_write);
QCRIL_EXTERN (sms_msg_event_delete);
QCRIL_EXTERN (sms_msg_event_read_template);
QCRIL_EXTERN (sms_msg_event_write_template);
QCRIL_EXTERN (sms_msg_event_received_message);
QCRIL_EXTERN (sms_msg_event_status_report);
QCRIL_EXTERN (sms_msg_event_transport_reg);
QCRIL_EXTERN (sms_cfg_event_message_list);
QCRIL_EXTERN (sms_cfg_event_memory_full);
QCRIL_EXTERN (sms_cfg_event_mem_status_set);
QCRIL_EXTERN (sms_bc_mm_event_table);
QCRIL_EXTERN (sms_bc_mm_event_add_services);
QCRIL_EXTERN (sms_bc_mm_event_pref);
QCRIL_EXTERN (sms_subscription_pref_info);
QCRIL_EXTERN (sms_request_raw_read);
QCRIL_EXTERN (sms_perform_initial_configuration_evt_handler);


/* Other RIL services */
QCRIL_EXTERN (other_request_get_imei);
QCRIL_EXTERN (other_request_get_imeisv);
QCRIL_EXTERN (other_request_set_mute);
QCRIL_EXTERN (other_request_get_mute);
QCRIL_EXTERN (other_request_baseband_version);
QCRIL_EXTERN (other_request_oem_hook_strings);
QCRIL_EXTERN (other_request_oem_hook_nv_read);
QCRIL_EXTERN (other_request_oem_hook_nv_write);
QCRIL_EXTERN (other_request_device_identity);
QCRIL_EXTERN (other_request_cdma_validate_and_write_akey);
QCRIL_EXTERN (other_request_cdma_subscription);
QCRIL_EXTERN (other_request_oem_hook_neighboring_cells_info);
QCRIL_EXTERN (qmi_ims_vt_dial_call);
QCRIL_EXTERN (qmi_ims_vt_end_call);
QCRIL_EXTERN (qmi_ims_vt_answer_call);
QCRIL_EXTERN (qmi_ims_vt_get_call_info);
QCRIL_EXTERN (qmi_ims_presence_enabler_state_req);
QCRIL_EXTERN (qmi_ims_presence_send_publish_req);
QCRIL_EXTERN (qmi_ims_presence_send_publish_xml_req);
QCRIL_EXTERN (qmi_ims_presence_send_unpublish_req);
QCRIL_EXTERN (qmi_ims_presence_send_subscribe_req);
QCRIL_EXTERN (qmi_ims_presence_send_subscribe_xml_req);
QCRIL_EXTERN (qmi_ims_presence_set_notify_fmt_req);
QCRIL_EXTERN (qmi_ims_presence_get_notify_fmt_req);
QCRIL_EXTERN (qmi_ims_presence_send_unsubscribe_req);
QCRIL_EXTERN (qmi_ims_presence_get_event_report_req);
QCRIL_EXTERN (qmi_ims_presence_set_event_report_req);

/* PBM */
QCRIL_EXTERN (pbm_event_handler);
QCRIL_EXTERN (pbm_event_card_state_changed);
QCRIL_EXTERN (pbm_event_update_ota_ecc_list);

/* QMI_VOICE */
QCRIL_EXTERN (qmi_voice_request_dial);
QCRIL_EXTERN (qmi_voice_request_get_current_atel_calls);
QCRIL_EXTERN (qmi_voice_request_get_current_ims_calls);
QCRIL_EXTERN (qmi_voice_request_answer);
QCRIL_EXTERN (qmi_voice_request_hangup);
QCRIL_EXTERN (qmi_voice_request_last_call_fail_cause);
QCRIL_EXTERN (qmi_voice_request_dtmf);
QCRIL_EXTERN (qmi_voice_request_dtmf_start);
QCRIL_EXTERN (qmi_voice_request_dtmf_stop);
QCRIL_EXTERN (qmi_voice_request_cdma_burst_dtmf);
QCRIL_EXTERN (qmi_voice_request_modify_call_initiate);
QCRIL_EXTERN (qmi_voice_request_modify_call_confirm);
QCRIL_EXTERN (qmi_voice_request_cdma_flash);
QCRIL_EXTERN (qmi_voice_request_set_preferred_voice_privacy_mode);
QCRIL_EXTERN (qmi_voice_request_query_preferred_voice_privacy_mode);
QCRIL_EXTERN (qmi_nas_request_set_preferred_network_type);
QCRIL_EXTERN (qmi_nas_request_get_preferred_network_type);
QCRIL_EXTERN (qmi_nas_request_set_roaming_preference);
QCRIL_EXTERN (qmi_nas_request_query_roaming_preference);
QCRIL_EXTERN(qmi_voice_request_set_supp_svc_notification);
QCRIL_EXTERN (qmi_voice_request_manage_calls_hangup_waiting_or_background);
QCRIL_EXTERN (qmi_voice_request_manage_calls_hangup_foreground_resume_background);
QCRIL_EXTERN(qmi_voice_request_manage_calls_switch_waiting_or_holding_and_active);
QCRIL_EXTERN (qmi_voice_request_manage_calls_conference);
QCRIL_EXTERN (qmi_voice_request_manage_calls_explicit_call_transfer);
QCRIL_EXTERN (qmi_voice_request_manage_calls_seperate_connection);
QCRIL_EXTERN (qmi_voice_request_manage_calls_udub);
QCRIL_EXTERN (qmi_voice_request_set_call_forward);
QCRIL_EXTERN (qmi_voice_request_set_facility_lock);
QCRIL_EXTERN (qmi_voice_request_set_call_waiting);
QCRIL_EXTERN (qmi_voice_request_query_clip);
QCRIL_EXTERN (qmi_voice_request_get_clir);
QCRIL_EXTERN (qmi_voice_request_change_barring_password);
QCRIL_EXTERN (qmi_voice_request_query_call_waiting);
QCRIL_EXTERN (qmi_voice_request_query_facility_lock);
QCRIL_EXTERN (qmi_voice_request_set_clir);
QCRIL_EXTERN (qmi_voice_request_query_call_forward_status);
QCRIL_EXTERN(qmi_voice_supsvc_request_send_ussd);
QCRIL_EXTERN(qmi_voice_supsvc_request_cancel_ussd);
QCRIL_EXTERN (qmi_voice_request_ims_set_supp_srv_status);
QCRIL_EXTERN(qmi_voice_request_call_deflection);

#if (RIL_VERSION >= 9)
#define QMI_RIL_IS_KK
#endif

#if (RIL_VERSION >= 7)
#define QMI_RIL_IS_JB
#endif

#if (RIL_VERSION > 6)
//Google added this struct with version 7 whereas we introduced it in v6
#define RIL_SimRefreshResponse_v6 RIL_SimRefreshResponse_v7

#endif

#if (RIL_VERSION >= 6)

// ICS support
#undef RIL_CDMA_CallWaiting
#define RIL_CDMA_CallWaiting    RIL_CDMA_CallWaiting_v6

#define QMI_RIL_IS_ICS

// These messages have been renamed on ICS to VOICE_REGISTRATION_STATE and
// VOICE_NETWORK_STATE_CHANGED. But RIL can just use the old names
#define RIL_REQUEST_REGISTRATION_STATE              RIL_REQUEST_VOICE_REGISTRATION_STATE
#define RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED    RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED

// Added by Qualcomm to identify illegal SIM apps.
#define RIL_APPSTATE_ILLEGAL    RIL_APPSTATE_READY

#if !defined(RIL_QCOM_VERSION) || RIL_QCOM_VERSION < 3
#define RADIO_TECH_TD_SCDMA     117
#else /* !defined(RIL_QCOM_VERSION) || RIL_QCOM_VERSION < 3 */

#if (RIL_VERSION >= 9)
#define RIL_TD_SCDMA_SignalStrength RIL_TD_SCDMA_SignalStrength_CAF
#endif /* (RIL_VERSION >= 9) */

#define QCRIL_TDSCDMA_UI

#endif /* !defined(RIL_QCOM_VERSION) || RIL_QCOM_VERSION < 3 */

#if (RIL_VERSION >= 9)
#define RIL_LTE_SignalStrength_qc RIL_LTE_SignalStrength_v8
#else /* (RIL_VERSION >= 9) */
#define RIL_LTE_SignalStrength_qc RIL_LTE_SignalStrength
#endif /* (RIL_VERSION >= 9) */

typedef struct {
  RIL_GW_SignalStrength GW_SignalStrength;
  RIL_CDMA_SignalStrength CDMA_SignalStrength;
  RIL_EVDO_SignalStrength EVDO_SignalStrength;
  RIL_LTE_SignalStrength_qc LTE_SignalStrength;
  RIL_TD_SCDMA_SignalStrength TD_SCDMA_SignalStrength;
} RIL_SignalStrength_qc_internal;

#endif /* (RIL_VERSION >= 6) */

#undef RIL_SignalStrength
#define RIL_SignalStrength      RIL_SignalStrength_qc_internal

// SMS over IMS support using IMS stack in the modem is added by QCOM
// So if RIL_REQUEST_IMS_REGISTRATION_STATE is not defined, then these messages will
// not be defined in ril.h. Add these in here for compilation
// and to avoid conditionally compiled code in QMI RIL
#ifdef RIL_REQUEST_IMS_REGISTRATION_STATE
#define FEATURE_QCRIL_IMS
#else
#define RIL_REQUEST_IMS_REGISTRATION_STATE 10009
#define RIL_REQUEST_IMS_SEND_SMS 10010
#define RIL_UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED 11009

typedef enum {
    RADIO_TECH_3GPP = 1, /* 3GPP Technologies - GSM, WCDMA */
    RADIO_TECH_3GPP2 = 2 /* 3GPP2 Technologies - CDMA */
} RIL_RadioTechnologyFamily;

typedef struct {
    RIL_RadioTechnologyFamily tech;
    unsigned char             retry;       /* 0 == not retry, nonzero == retry */
    int                       messageRef;  /* Valid field if retry is set to nonzero.
                                              Contains messageRef from RIL_SMS_Response
                                              corresponding to failed MO SMS.
                                            */

    union {
        /* Valid field if tech is RADIO_TECH_3GPP2. See RIL_REQUEST_CDMA_SEND_SMS */
        RIL_CDMA_SMS_Message* cdmaMessage;

        /* Valid field if tech is RADIO_TECH_3GPP. See RIL_REQUEST_SEND_SMS */
        char**                gsmMessage;
    } message;
} RIL_IMS_SMS_Message;

#endif /* RIL_REQUEST_IMS_REGISTRATION_STATE */


#ifndef RIL_REQUEST_SETUP_QOS

#define RIL_REQUEST_SETUP_QOS      10012
#define RIL_REQUEST_RELEASE_QOS    10013
#define RIL_REQUEST_GET_QOS_STATUS 10014
#define RIL_REQUEST_MODIFY_QOS     10015
#define RIL_REQUEST_SUSPEND_QOS    10016
#define RIL_REQUEST_RESUME_QOS     10017
#define RIL_UNSOL_QOS_STATE_CHANGED_IND 11010

typedef enum {
    RIL_QOS_CONVERSATIONAL,
    RIL_QOS_STREAMING,
    RIL_QOS_INTERACTIVE,
    RIL_QOS_BACKGROUND
} RIL_QosClass;

typedef enum {
    RIL_QOS_TX,
    RIL_QOS_RX
} RIL_QosDirection;

/* QoS status */
typedef enum {
    RIL_QOS_STATUS_NONE,        /* Qos not active */
    RIL_QOS_STATUS_ACTIVATED,   /* Qos currently active */
    RIL_QOS_STATUS_SUSPENDED    /* Qos Suspended */
} RIL_QosStatus;

/* Enum for status of the QoS flows */
typedef enum {
    RIL_QOS_ACTIVATED,           /* QoS activation completed or QoS Resumed) */
    RIL_QOS_ACTIVATED_NETWORK,   /* QoS activation (from network) complete */
    RIL_QOS_USER_RELEASE,        /* QoS released by the user */
    RIL_QOS_NETWORK_RELEASE,     /* QoS released by the network */
    RIL_QOS_SUSPENDED,           /* QoS was suspended */
    RIL_QOS_MODIFIED,            /* QoS modified */
    RIL_QOS_ERROR_UNKNOWN        /* Any other error */
} RIL_QosIndStates;

/* Keys the QoS spec along with the description of their values.
 *
 * Each QoS Spec will begin with a unique SPEC_INDEX. Within each spec there can
 * be multiple filter sets, each of which will start with a unique FILTER_INDEX
 */
typedef enum {
    RIL_QOS_SPEC_INDEX,                         /* Positive numerical value */

    RIL_QOS_FLOW_DIRECTION,                     /* RIL_QosDirection */
    RIL_QOS_FLOW_TRAFFIC_CLASS,                 /* RIL_QosClass */
    RIL_QOS_FLOW_DATA_RATE_MIN,                 /* Positive number in bps */
    RIL_QOS_FLOW_DATA_RATE_MAX,                 /* Positive number in bps */
    RIL_QOS_FLOW_LATENCY,                       /* Positive number in milliseconds */

    RIL_QOS_FLOW_3GPP2_PROFILE_ID,              /* Positive numerical value */
    RIL_QOS_FLOW_3GPP2_PRIORITY,                /* Positive numerical value */

    RIL_QOS_FILTER_INDEX,                       /* Mandatory. Positive numerical value */
    RIL_QOS_FILTER_IPVERSION,                   /* Mandatory. Values must be "IP" or "IPV6" */
    RIL_QOS_FILTER_DIRECTION,                   /* RIL_QosDirection */
    RIL_QOS_FILTER_IPV4_SOURCE_ADDR,            /* Format: xxx.xxx.xxx.xxx/yy */
    RIL_QOS_FILTER_IPV4_DESTINATION_ADDR,       /* Format: xxx.xxx.xxx.xxx/yy */
    RIL_QOS_FILTER_IPV4_TOS,                    /* Positive numerical Value (max 6-bit number) */
    RIL_QOS_FILTER_IPV4_TOS_MASK,               /* Mask for the 6 bit TOS value */

    /**
     * *PORT_START is the starting port number,
     * *PORT_RANGE is the number of continuous ports from *PORT_START key
     */
    RIL_QOS_FILTER_TCP_SOURCE_PORT_START,
    RIL_QOS_FILTER_TCP_SOURCE_PORT_RANGE,
    RIL_QOS_FILTER_TCP_DESTINATION_PORT_START,
    RIL_QOS_FILTER_TCP_DESTINATION_PORT_RANGE,
    RIL_QOS_FILTER_UDP_SOURCE_PORT_START,
    RIL_QOS_FILTER_UDP_SOURCE_PORT_RANGE,
    RIL_QOS_FILTER_UDP_DESTINATION_PORT_START,
    RIL_QOS_FILTER_UDP_DESTINATION_PORT_RANGE,

    RIL_QOS_FILTER_IPV6_SOURCE_ADDR,        /* Format: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/yyy */
    RIL_QOS_FILTER_IPV6_DESTINATION_ADDR,   /* Format: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/yyy */
    RIL_QOS_FILTER_IPV6_TRAFFIC_CLASS,
    RIL_QOS_FILTER_IPV6_FLOW_LABEL
} RIL_QosSpecKeys;

#endif /* RIL_REQUEST_SETUP_QOS */

#ifndef RIL_QCOM_VERSION

#define QMI_RIL_UNDER_PLAIN_ANDROID

#ifndef RADIO_TECH_GSM
#define RADIO_TECH_GSM     16
#endif

#define RADIO_TECH_IWLAN 18

#endif // #ifndef RIL_QCOM_VERSION

#ifndef RIL_UNSOL_RESPONSE_TETHERED_MODE_STATE_CHANGED
#define RIL_UNSOL_RESPONSE_TETHERED_MODE_STATE_CHANGED 11037
#endif

/* QCRIL definition for RIL_RadioTechnology for get/setPreferredVoice/Data */

/* GSM/WCDMA (WCDMA preferred) */
#define QCRIL_PREF_NET_TYPE_GSM_WCDMA                            PREF_NET_TYPE_GSM_WCDMA

/* GSM only */
#define QCRIL_PREF_NET_TYPE_GSM_ONLY                             PREF_NET_TYPE_GSM_ONLY

/* WCDMA  */
#define QCRIL_PREF_NET_TYPE_WCDMA                                PREF_NET_TYPE_WCDMA

/* GSM/WCDMA (auto mode, according to PRL) */
#define QCRIL_PREF_NET_TYPE_GSM_WCDMA_AUTO                       PREF_NET_TYPE_GSM_WCDMA_AUTO

/* CDMA and EvDo (auto mode, according to PRL) */
#define QCRIL_PREF_NET_TYPE_CDMA_EVDO_AUTO                       PREF_NET_TYPE_CDMA_EVDO_AUTO

/* CDMA only */
#define QCRIL_PREF_NET_TYPE_CDMA_ONLY                            PREF_NET_TYPE_CDMA_ONLY

/* EvDo only */
#define QCRIL_PREF_NET_TYPE_EVDO_ONLY                            PREF_NET_TYPE_EVDO_ONLY

/* GSM/WCDMA, CDMA, and EvDo (auto mode, according to PRL) */
#define QCRIL_PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO             PREF_NET_TYPE_GSM_WCDMA_CDMA_EVDO_AUTO

/* LTE, CDMA and EvDo */
#define QCRIL_PREF_NET_TYPE_LTE_CDMA_EVDO                        PREF_NET_TYPE_LTE_CDMA_EVDO

/* LTE, GSM/WCDMA */
#define QCRIL_PREF_NET_TYPE_LTE_GSM_WCDMA                        PREF_NET_TYPE_LTE_GSM_WCDMA

/* LTE, CDMA, EvDo, GSM/WCDMA */
#define QCRIL_PREF_NET_TYPE_LTE_CMDA_EVDO_GSM_WCDMA              PREF_NET_TYPE_LTE_CMDA_EVDO_GSM_WCDMA

/* LTE only */
#define QCRIL_PREF_NET_TYPE_LTE_ONLY                             PREF_NET_TYPE_LTE_ONLY

#ifdef QCRIL_TDSCDMA_UI
#define FEATURE_TDSCDMA_UI

/* LTE/WCDMA */
#define QCRIL_PREF_NET_TYPE_LTE_WCDMA                            PREF_NET_TYPE_LTE_WCDMA

/* TD-SCDMA only */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_ONLY                        PREF_NET_TYPE_TD_SCDMA_ONLY

/* TD-SCDMA and WCDMA */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_WCDMA                       PREF_NET_TYPE_TD_SCDMA_WCDMA

/* TD-SCDMA and LTE */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_LTE                         PREF_NET_TYPE_TD_SCDMA_LTE

/* TD-SCDMA and GSM */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM                         PREF_NET_TYPE_TD_SCDMA_GSM

/* TD-SCDMA,GSM and LTE */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_LTE                     PREF_NET_TYPE_TD_SCDMA_GSM_LTE

/* TD-SCDMA, GSM/WCDMA */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA                   PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA

/* TD-SCDMA,WCDMA and LTE */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_WCDMA_LTE                   PREF_NET_TYPE_TD_SCDMA_WCDMA_LTE

/* TD-SCDMA, GSM/WCDMA and LTE */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_LTE               PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_LTE

/* TD-SCDMA, GSM/WCDMA, CDMA, and EvDo */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO    PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO

/* TD-SCDMA,LTE, CDMA, EvDo, GSM/WCDMA */
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA     PREF_NET_TYPE_TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA

#else

#define QCRIL_PREF_NET_TYPE_LTE_WCDMA                            (12)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_ONLY                        (13)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_WCDMA                       (14)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_LTE                         (15)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM                         (16)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_LTE                     (17)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA                   (18)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_WCDMA_LTE                   (19)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_LTE               (20)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO    (21)
#define QCRIL_PREF_NET_TYPE_TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA     (22)

#endif /* QCRIL_TDSCDMA_UI */


#define RIL_REQUEST_UNKOWN  106
#define RIL_UNSOL_UNKOWN 1101

#ifndef RIL_REQUEST_MODIFY_CALL_INITIATE
// VT/VoLTE emulation
#define RIL_REQUEST_MODIFY_CALL_INITIATE 10002
#define RIL_REQUEST_MODIFY_CALL_CONFIRM  10003
#define RIL_UNSOL_MODIFY_CALL 11042

typedef enum {
    RIL_CALL_TYPE_VOICE = 0,
    RIL_CALL_TYPE_VS_TX = 1,
    RIL_CALL_TYPE_VS_RX = 2,
    RIL_CALL_TYPE_VT = 3,
    RIL_CALL_TYPE_VT_NODIR = 4,
} RIL_Call_Type;

typedef enum {
    RIL_CALL_DOMAIN_UNKNOWN = 0,
    RIL_CALL_DOMAIN_CS = 1,
    RIL_CALL_DOMAIN_PS = 2,
    RIL_CALL_DOMAIN_AUTOMATIC = 3
} RIL_Call_Domain;

typedef struct {
    RIL_Call_Type   callType;
    RIL_Call_Domain callDomain;
    int extrasLength;
    int n_extras;
    const char **extras;
} RIL_Call_Details;

typedef struct {
    int             callIndex;
    RIL_Call_Details *callDetails;
} RIL_Call_Modify;

#define QMI_RIL__RIL_VT_VOLTE_EMULATED

extern RIL_Call_Details* __vt_volte_emulation_call_details_dummy_ptr;
#define VT_VOLTE_CALL_DETAILS( x )      __vt_volte_emulation_call_details_dummy_ptr

#else

#define VT_VOLTE_CALL_DETAILS( x )      x

#endif // RIL_REQUEST_MODIFY_CALL_INITIATE

#ifndef RIL_REQUEST_GET_DC_RT_INFO
#define RIL_REQUEST_GET_DC_RT_INFO 10117

typedef enum {
    RIL_DC_POWER_STATE_UNKNOWN            = 0,    // Unknown state
    RIL_DC_POWER_STATE_INACTIVE           = 1,    // Data connection is in a inactive state
    RIL_DC_POWER_STATE_DORMANT            = 2,    // Data connection is in a low power state
    RIL_DC_POWER_STATE_NOT_DORMANT        = 3,    // Data connection is in a high power state
} RIL_DcPowerStates;

/**
 * Data connection real time info
 */
typedef struct {
    int                         call_id;    //Call identifier
    RIL_RadioTechnology         radioTech;  // Radio technology
    RIL_DcPowerStates           powerState; // Current power state
} RIL_DcRtInfo;

#endif

#ifndef RIL_UNSOL_DC_RT_INFO_CHANGED
#define RIL_UNSOL_DC_RT_INFO_CHANGED 11036
#endif

#ifndef RIL_REQUEST_GET_CELL_INFO_LIST

#  define RIL_REQUEST_GET_CELL_INFO_LIST 10123
#  define RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE 10124
#  define RIL_UNSOL_CELL_INFO_LIST 11042

typedef struct {
    int signalStrength;  /* Valid values are (0-31, 99) as defined in TS 27.007 8.5 */
    int bitErrorRate;    /* bit error rate (0-7, 99) as defined in TS 27.007 8.5 */
} RIL_SignalStrengthWcdma;

typedef struct {
    int signalStrength;  /* Valid values are (0-31, 99) as defined in TS 27.007 8.5 */
    int rsrp;            /* The current Reference Signal Receive Power in dBm multipled by -1.
                          * Range: 44 to 140 dBm
                          * INT_MAX: 0x7FFFFFFF denotes invalid value.
                          * Reference: 3GPP TS 36.133 9.1.4 */
    int rsrq;            /* The current Reference Signal Receive Quality in dB multiplied by -1.
                          * Range: 20 to 3 dB.
                          * INT_MAX: 0x7FFFFFFF denotes invalid value.
                          * Reference: 3GPP TS 36.133 9.1.7 */
    int rssnr;           /* The current reference signal signal-to-noise ratio in 0.1 dB units.
                          * Range: -200 to +300 (-200 = -20.0 dB, +300 = 30dB).
                          * INT_MAX : 0x7FFFFFFF denotes invalid value.
                          * Reference: 3GPP TS 36.101 8.1.1 */
    int cqi;             /* The current Channel Quality Indicator.
                          * Range: 0 to 15.
                          * INT_MAX : 0x7FFFFFFF denotes invalid value.
                          * Reference: 3GPP TS 36.101 9.2, 9.3, A.4 */
    int timingAdvance;   /* timing advance in micro seconds for a one way trip from cell to device.
                          * Approximate distance can be calculated using 300m/us * timingAdvance.
                          * Range: 0 to 0x7FFFFFFE
                          * INT_MAX : 0x7FFFFFFF denotes invalid value.
                          * Reference: 3GPP 36.321 section 6.1.3.5
                          * also: http://www.cellular-planningoptimization.com/2010/02/timing-advance-with-calculation.html */
} RIL_LTE_SignalStrength_v8;

typedef RIL_LTE_SignalStrength RIL_LTE_SignalStrength_v8;

typedef struct {
    RIL_GW_SignalStrength       GW_SignalStrength;
    RIL_CDMA_SignalStrength     CDMA_SignalStrength;
    RIL_EVDO_SignalStrength     EVDO_SignalStrength;
    RIL_LTE_SignalStrength_v8   LTE_SignalStrength;
} RIL_SignalStrength_v8;

/** RIL_CellIdentityGsm */
typedef struct {
    int mcc;    /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown */
    int mnc;    /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown */
    int lac;    /* 16-bit Location Area Code, 0..65535, INT_MAX if unknown  */
    int cid;    /* 16-bit GSM Cell Identity described in TS 27.007, 0..65535, INT_MAX if unknown  */
} RIL_CellIdentityGsm;

/** RIL_CellIdentityWcdma */
typedef struct {
    int mcc;    /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown  */
    int mnc;    /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown  */
    int lac;    /* 16-bit Location Area Code, 0..65535, INT_MAX if unknown  */
    int cid;    /* 28-bit UMTS Cell Identity described in TS 25.331, 0..268435455, INT_MAX if unknow
n  */
    int psc;    /* 9-bit UMTS Primary Scrambling Code described in TS 25.331, 0..511, INT_MAX if unk
nown */
} RIL_CellIdentityWcdma;

/** RIL_CellIdentityCdma */
typedef struct {
    int networkId;      /* Network Id 0..65535, INT_MAX if unknown */
    int systemId;       /* CDMA System Id 0..32767, INT_MAX if unknown  */
    int basestationId;  /* Base Station Id 0..65535, INT_MAX if unknown  */
    int longitude;      /* Longitude is a decimal number as specified in 3GPP2 C.S0005-A v6.0.
                         * It is represented in units of 0.25 seconds and ranges from -2592000
                         * to 2592000, both values inclusive (corresponding to a range of -180
                         * to +180 degrees). INT_MAX if unknown */

    int latitude;       /* Latitude is a decimal number as specified in 3GPP2 C.S0005-A v6.0.
                         * It is represented in units of 0.25 seconds and ranges from -1296000
                         * to 1296000, both values inclusive (corresponding to a range of -90
                         * to +90 degrees). INT_MAX if unknown */
} RIL_CellIdentityCdma;

/** RIL_CellIdentityLte */
typedef struct {
    int mcc;    /* 3-digit Mobile Country Code, 0..999, INT_MAX if unknown  */
    int mnc;    /* 2 or 3-digit Mobile Network Code, 0..999, INT_MAX if unknown  */
    int ci;     /* 28-bit Cell Identity described in TS ???, INT_MAX if unknown */
    int pci;    /* physical cell id 0..503, INT_MAX if unknown  */
    int tac;    /* 16-bit tracking area code, INT_MAX if unknown  */
} RIL_CellIdentityLte;

/** RIL_CellInfoGsm */
typedef struct {
  RIL_CellIdentityGsm   cellIdentityGsm;
  RIL_GW_SignalStrength signalStrengthGsm;
} RIL_CellInfoGsm;

/** RIL_CellInfoWcdma */
typedef struct {
  RIL_CellIdentityWcdma cellIdentityWcdma;
  RIL_SignalStrengthWcdma signalStrengthWcdma;
} RIL_CellInfoWcdma;

/** RIL_CellInfoCdma */
typedef struct {
  RIL_CellIdentityCdma      cellIdentityCdma;
  RIL_CDMA_SignalStrength   signalStrengthCdma;
  RIL_EVDO_SignalStrength   signalStrengthEvdo;
} RIL_CellInfoCdma;

/** RIL_CellInfoLte */
typedef struct {
  RIL_CellIdentityLte        cellIdentityLte;
  RIL_LTE_SignalStrength_v8  signalStrengthLte;
} RIL_CellInfoLte;

// Must be the same as CellInfo.TYPE_XXX
typedef enum {
  RIL_CELL_INFO_TYPE_GSM    = 1,
  RIL_CELL_INFO_TYPE_CDMA   = 2,
  RIL_CELL_INFO_TYPE_LTE    = 3,
  RIL_CELL_INFO_TYPE_WCDMA  = 4,
} RIL_CellInfoType;

// Must be the same as CellInfo.TIMESTAMP_TYPE_XXX
typedef enum {
    RIL_TIMESTAMP_TYPE_UNKNOWN = 0,
    RIL_TIMESTAMP_TYPE_ANTENNA = 1,
    RIL_TIMESTAMP_TYPE_MODEM = 2,
    RIL_TIMESTAMP_TYPE_OEM_RIL = 3,
    RIL_TIMESTAMP_TYPE_JAVA_RIL = 4,
} RIL_TimeStampType;

typedef struct {
  RIL_CellInfoType  cellInfoType;   /* cell type for selecting from union CellInfo */
  int               registered;     /* !0 if this cell is registered 0 if not registered */
  RIL_TimeStampType timeStampType;  /* type of time stamp represented by timeStamp */
  uint64_t          timeStamp;      /* Time in nanos as returned by ril_nano_time */
  union {
    RIL_CellInfoGsm     gsm;
    RIL_CellInfoCdma    cdma;
    RIL_CellInfoLte     lte;
    RIL_CellInfoWcdma   wcdma;
  } CellInfo;
} RIL_CellInfo;

static inline uint64_t ril_nano_time()
{
    return 0;
}

#endif // RIL_REQUEST_GET_CELL_INFO_LIST

#ifndef RIL_REQUEST_GET_CELL_INFO_LIST

#  define RIL_REQUEST_GET_CELL_INFO_LIST 10123
#  define RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE 10124
#  define RIL_UNSOL_CELL_INFO_LIST 11042

/** RIL_CellIdentityGsm */
typedef struct {
    int mcc;    /* 3-digit Mobile Country Code, 0..999 */
    int mnc;    /* 2 or 3-digit Mobile Network Code, 0..999 */
    int lac;    /* 16-bit Location Area Code, 0..65535 */
    int cid;    /* 16-bit GSM Cell Identity described in TS 27.007, 0..65535
                 * 28-bit UMTS Cell Identity described in TS 25.331, 0..268435455 */
    int psc;    /* 9-bit UMTS Primary Scrambling Code described in TS 25.331, 0..511 */
} RIL_CellIdentityGsm;

/** RIL_CellIdentityCdma */
typedef struct {
    int networkId;      /* Network Id 0..65535 */
    int systemId;       /* CDMA System Id 0..32767 */
    int basestationId;  /* Base Station Id 0..65535 */
    int longitude;      /* Longitude is a decimal number as specified in 3GPP2 C.S0005-A v6.0.
                         * It is represented in units of 0.25 seconds and ranges from -2592000
                         * to 2592000, both values inclusive (corresponding to a range of -180
                         * to +180 degrees). */

    int latitude;       /* Latitude is a decimal number as specified in 3GPP2 C.S0005-A v6.0.
                         * It is represented in units of 0.25 seconds and ranges from -1296000
                         * to 1296000, both values inclusive (corresponding to a range of -90
                         * to +90 degrees). */
} RIL_CellIdentityCdma;

/** RIL_CellIdentityLte */
typedef struct {
    int mcc;    /* 3-digit Mobile Country Code, 0..999 */
    int mnc;    /* 2 or 3-digit Mobile Network Code, 0..999 */
    int ci;     /* 28-bit Cell Identity described in TS ??? */
    int pci;    /* physical cell id 0..503 */
    int tac;    /* 16-bit tracking area code */
} RIL_CellIdentityLte;

/** RIL_CellInfoGsm */
typedef struct {
  RIL_CellIdentityGsm   cellIdentityGsm;
  RIL_GW_SignalStrength signalStrengthGsm;
} RIL_CellInfoGsm;

/** RIL_CellInfoCdma */
typedef struct {
  RIL_CellIdentityCdma      cellIdentityCdma;
  RIL_CDMA_SignalStrength   signalStrengthCdma;
  RIL_EVDO_SignalStrength   signalStrengthEvdo;
} RIL_CellInfoCdma;

/** RIL_CellInfoLte */
typedef struct {
  RIL_CellIdentityLte        cellIdentityLte;
  RIL_LTE_SignalStrength_v7  signalStrengthLte;
} RIL_CellInfoLte;

typedef enum {
  RIL_CELL_INFO_TYPE_GSM    = 1,
  RIL_CELL_INFO_TYPE_CDMA   = 2,
  RIL_CELL_INFO_TYPE_LTE    = 3,
} RIL_CellInfoType;

typedef struct {
  RIL_CellInfoType  cellInfoType;   /* cell type for selecting from union CellInfo */
  int               registered;     /* !0 if this cell is registered 0 if not registered */
  uint64_t          time;           /* Time in nanos as returned by ril_nano_time */
  union {
    RIL_CellInfoGsm     gsm;
    RIL_CellInfoCdma    cdma;
    RIL_CellInfoLte     lte;
  } CellInfo;
} RIL_CellInfo;

#endif // RIL_REQUEST_GET_CELL_INFO_LIST

#ifndef RIL_REQUEST_GET_DATA_CALL_PROFILE

/* Data Call Profile: Simple IP User Profile Parameters*/
typedef struct {
  int  profileId;
  int  priority;       /* priority. [0..255], 0 - highest */
} RIL_DataCallProfileInfo;

#define RIL_REQUEST_GET_DATA_CALL_PROFILE 10111

#endif // RIL_REQUEST_GET_DATA_CALL_PROFILE

#endif /* QCRILI_H */

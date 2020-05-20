/*===========================================================================
  FILE: NetPlatformLib.cpp

  OVERVIEW: This file provides implementation for the network platform
  library.

  DEPENDENCIES: None

  Copyright (c) 2008-2012 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are signaled in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/NetPlatformLib.cpp#2 $
  $DateTime: 2011/06/29 08:15:52 $$Author: smudired $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-12-23 ss  Cleaning up of Globals for thread safety in Q6 Free
                 Floating environment.
  2008-07-02 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "AEEstd.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_Conversion.h"
#include "AEEStdErr.h"
#include "ds_Net_Platform.h"

#include "ps_iface_ioctl.h"
#include "ps_flow_ioctl.h"
#include "ps_phys_link_ioctl.h"
#include "ps_iface.h"
#include "ps_flow.h"
#include "ps_iface_flow.h"
#include "ps_flowi_event.h"
#include "ps_flowi_utils.h"
#include "ps_route.h"
#include "ps_routei.h"
#include "ps_in.h"
#include "ps_iface_ipfltr.h"
#include "dserrno.h"
#include "ps_acl.h"
#include "ps_route_scope.h"
#include "ps_ip6_addr.h"
#include "ds_Utils_Conversion.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_IPolicyPriv.h"
#include "ps_metai_info.h"
#include "ds_Utils_DebugMsg.h"

#ifdef FEATURE_DATA_PS_NAT_IFACE
#include "ps_nat_iface_hdlr.h"
#include "ps_softap_roaming_config.h"
#endif

#if defined (FEATURE_DATA_PS_IPV6)
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ps_utils.h"
#include "ps_crit_sect.h"
#include "ps_stat_iface.h"
#include "ps_policy_mgr.h"
#include "ps_policyi_mgr.h"


#ifdef FEATURE_DSS_LINUX
const struct ps_in6_addr ps_in6addr_any = PS_IN6ADDR_ANY_INIT;            /* :: */
const struct ps_in6_addr ps_in6addr_loopback = PS_IN6ADDR_LOOPBACK_INIT ; /* ::1 */

/* TODO: these are defined in inet/src, which is not ported to Linux. 
 * Using local definitions for now. */
uint32 v6_lo_rscope_idx;
uint32  v4_lo_rscope_idx;

#define ps_ip_addr_ipv6_priv_addr_inc_ref_cnt(a) (0)
#define ps_ip_addr_ipv6_priv_addr_dec_ref_cnt(a) (0)
#endif /* FEATURE_DSS_LINUX */

ps_iface_type * ps_route_look_up_iface_by_policy
(
  acl_policy_info_type    * aclPolInfoptr,
  boolean                 * arbCandidate,
  uint32                  * ifacePriorityMask
);

#ifdef __cplusplus
}
#endif

using namespace NetPlatform;
using namespace ds::Error;
using namespace ds::Net;
using namespace ds::Utils::Conversion;

static IfaceEventCbackType     ifaceCbackFcnPtr = NULL;
static FlowEventCbackType      flowCbackFcnPtr = NULL;
static PhysLinkEventCbackType  physLinkCbackFcnPtr = NULL;


void NetPlatform::Init
(
  void
)
{
  int32 result;
  int   dummy = 1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("ds Net Platform Init()", 0, 0, 0);

  result = IfaceRegAllEvents ((void*) &dummy);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = FlowRegAllEvents ((void*) &dummy);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = PhysLinkRegAllEvents ((void*) &dummy);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return;

bail:
  LOG_MSG_FATAL_ERROR ("Err registering for events.", 0, 0, 0);
  ASSERT (0);
  return;

} /* Init() */


int32 NetPlatform::GenerateAclPolicy
(
  IPolicy               *netPolicyPtr,
  ACLPolicyInfoType     *aclPolicyPtr
)
{
  int32 result;
  ds::AddrFamilyType        addrFamily;
  int                       appType;
  ds::Net::IfaceNameType    ifaceName;
  IfaceGroupType            ifaceGroup;
  PolicyFlagType            policyFlag;
  int                       umtsProfileNum;
  int                       cdmaProfileNum;
  boolean                   isRouteable;
  ps_iface_id_type          ifaceId;
  IPolicyPriv               *netPolicyPrivPtr = NULL;
  static char               apnName[255];
  int                       apnLen;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == aclPolicyPtr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-----------------------------------------------------------------------
    If input Policy object is NULL, then return default inited ACL policy.
  -----------------------------------------------------------------------*/
  memset (aclPolicyPtr, 0, sizeof(ACLPolicyInfoType));
  acl_init_policy_info (aclPolicyPtr);
  aclPolicyPtr->is_sock_orig = TRUE;

  if (NULL == netPolicyPtr)
  {
    return AEE_SUCCESS;
  }

  /*-----------------------------------------------------------------------
    Get the parameters set in the Policy object.
  -----------------------------------------------------------------------*/
  result = netPolicyPtr->QueryInterface(AEEIID_IPolicyPriv,
                                        (void **)&netPolicyPrivPtr);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetAddressFamily (&addrFamily);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetAppType (&appType);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetIfaceName (&ifaceName);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetIfaceGroup (&ifaceGroup);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetPolicyFlag (&policyFlag);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetCDMAProfileNumber(&cdmaProfileNum);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetUMTSProfileNumber(&umtsProfileNum);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetRoutable (&isRouteable);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetIfaceId((::ds::Net::IfaceIdType *)&ifaceId);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = netPolicyPrivPtr->GetUMTSAPNName(apnName, 255, &apnLen);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Set the parameters obtained from Policy object into ACL policy.
  -----------------------------------------------------------------------*/
  aclPolicyPtr->app_identifier  = (unsigned int)appType;
  aclPolicyPtr->pdp_info        = (void *) umtsProfileNum;
  aclPolicyPtr->data_session_profile_id = cdmaProfileNum;
  aclPolicyPtr->policy_flag     = (dss_iface_policy_flags_enum_type)
                                  policyFlag;
  aclPolicyPtr->is_routeable    = isRouteable;
  aclPolicyPtr->apn.length      = apnLen;
  (void) std_strlcpy ((char *) aclPolicyPtr->apn.name,
                      (const char *) apnName,
                      255);

  /*-----------------------------------------------------------------------
    If iface ID is set, set the same into ACL policy.
    Else if iface name is set (non-zero), use the given iface name.
    Else if iface group is set (non-zero), use the iface group.
    Else use IFACE_ANY_DEFAULT iface name.
  -----------------------------------------------------------------------*/
  if (PS_IFACE_INVALID_ID != ifaceId)
  {
    aclPolicyPtr->iface.kind = DSS_IFACE_ID;
    aclPolicyPtr->iface.info.id = ifaceId;
  }
  else if (0 != ifaceName)
  {
    aclPolicyPtr->iface.kind = DSS_IFACE_NAME;
    aclPolicyPtr->iface.info.name = (dss_iface_name_enum_type) ifaceName;
  }
  else if (0 != ifaceGroup)
  {
    aclPolicyPtr->iface.kind = DSS_IFACE_NAME;
    aclPolicyPtr->iface.info.name = (dss_iface_name_enum_type) ifaceGroup;
  }
  else
  {
    aclPolicyPtr->iface.kind = DSS_IFACE_NAME;
    aclPolicyPtr->iface.info.name = DSS_IFACE_ANY_DEFAULT;
  }

  /*-----------------------------------------------------------------------
    The values of QDS_AF_INET, QDS_AF_INET6, QDS_AF_UNSPEC and DS_AF_ANY are
    not the same as IFACE_IPV4_ADDR_FAMILY etc used by ACL addr family
  -----------------------------------------------------------------------*/
  switch (addrFamily)
  {
    case ds::AddrFamily::QDS_AF_INET:
      aclPolicyPtr->ip_family = IFACE_IPV4_ADDR_FAMILY;
      break;
    case ds::AddrFamily::QDS_AF_INET6:
      aclPolicyPtr->ip_family = IFACE_IPV6_ADDR_FAMILY;
      break;
    case ds::AddrFamily::QDS_AF_UNSPEC:
      aclPolicyPtr->ip_family = IFACE_UNSPEC_ADDR_FAMILY;
      break;
    default:
      aclPolicyPtr->ip_family = IFACE_ANY_ADDR_FAMILY;
      break;
  }

  (void) netPolicyPrivPtr->Release();
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Conversion to ACL policy failed, err %d", result, 0, 0);
  if (NULL != netPolicyPrivPtr)
  {
    (void) netPolicyPrivPtr->Release();
  }
  return result;

} /* GenerateAclPolicy() */

int32 NetPlatform::RegIfaceCbackFcn
(
  IfaceEventCbackType cbackFcnPtr,
  void *              userDataPtr
)
{
  if (NULL == cbackFcnPtr)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  ifaceCbackFcnPtr = cbackFcnPtr;
  return AEE_SUCCESS;
}

int32 NetPlatform::RegFlowCbackFcn
(
  FlowEventCbackType  cbackFcnPtr,
  void *              userDataPtr
)
{
  if (NULL == cbackFcnPtr)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  flowCbackFcnPtr = cbackFcnPtr;
  return AEE_SUCCESS;
}

int32 NetPlatform::RegPhysLinkCbackFcn
(
  PhysLinkEventCbackType cbackFcnPtr,
  void *                 userDataPtr
)
{
  if (NULL == cbackFcnPtr)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  physLinkCbackFcnPtr = cbackFcnPtr;
  return AEE_SUCCESS;
}

/*---------------------------------------------------------------------------
  Local structure that hold information about events being registered for.
  TODO: Change this to CPP style naming conventions.
---------------------------------------------------------------------------*/
static struct
{
  void                 *iface_flow_enabled_cback;
  void                 *phys_link_flow_enabled_cback;
  void                 *flow_tx_enabled_cback;
  void                 *ip_addr_changed_cback;
  void                 *iface_up_cback;
  void                 *iface_down_cback;
  void                 *iface_coming_up_cback;
  void                 *iface_going_down_cback;
  void                 *iface_lingering_cback;
  void                 *iface_configuring_cback;
  void                 *iface_routeable_cback;
  void                 *iface_extended_ip_config_cback;
  void                 *phys_link_up_cback;
  void                 *phys_link_down_cback;
  void                 *phys_link_gone_cback;
  void                 *phys_link_coming_up_cback;
  void                 *phys_link_going_down_cback;
  void                 *phys_link_going_null_cback;
  void                 *phys_link_resuming_cback;
  void                 *iface_ipfltr_updated_cback;
  void                 *addr_family_changed_cback;
  void                 *iface_mt_request_cback;
  void                 *iface_mcast_reg_success_cback;
  void                 *iface_mcast_reg_failure_cback;
  void                 *iface_idle_event_cback;
#ifdef FEATURE_HDR_BCMCS_2p0
  void                 *iface_mcast_status_cback;
#endif /* FEATURE_HDR_BCMCS_2p0 */
  void                 *iface_mcast_dereg_cback;
  void                 *iface_bearer_tech_change_cback;
  void                 *iface_hdr_rev0_rate_inertia_success_cback;
  void                 *iface_hdr_rev0_rate_inertia_failure_cback;
  void                 *iface_outage_notification_cback;
  void                 *iface_rf_conditions_change_cback;
  void                 *iface_app_preempted_cback;

  void                 *iface_hdr_slotted_mode_success_cback;
  void                 *iface_hdr_slotted_mode_failure_cback;
  void                 *iface_hdr_slotted_mode_session_changed_cback;

#ifdef FEATURE_DATA_PS_IPV6
  void                 *prefix_updated_cback;
  void                 *iface_ipv6_priv_addr_generated_cback;
  void                 *iface_ipv6_priv_addr_deprecated_cback;
  void                 *iface_ipv6_priv_addr_deleted_cback;
#endif /* FEATURE_DATA_PS_IPV6 */

#ifdef FEATURE_DATA_PS_QOS
  void                 *flow_activate_cback;
  void                 *flow_suspended_cback;
  void                 *flow_null_cback;
  void                 *flow_modify_accepted_cback;
  void                 *flow_modify_rejected_cback;
  void                 *flow_primary_modify_result_cback;
  void                 *flow_info_code_updated_cback;
  void                 *iface_qos_aware_cback;
  void                 *iface_qos_unaware_cback;
  void                 *iface_flow_added_cback;
  void                 *iface_flow_deleted_cback;
  void                 *iface_network_supported_qos_profiles_changed_cback;
  void                 *iface_pri_phys_link_changed_cback;
#endif /* FEATURE_DATA_PS_QOS */

} EventCbackInfo;

/*---------------------------------------------------------------------------
  Map all the PS errno values to return values for ds
---------------------------------------------------------------------------*/
//TODO: Return value would be ds::ErrorType
int NetPlatform::MapErrorCode (int16 psErrno)
throw()
{
  return PSErrnoGetDSErrno( psErrno);
} /* MapErrorCode() */

typedef iface_event_info_cache_type IfaceEventInfoCacheType;
typedef flow_event_info_cache_type FlowEventInfoCacheType;
typedef phys_link_event_info_cache_type PhysLinkEventInfoCacheType;

/*---------------------------------------------------------------------------
  Functions to cache event info.
---------------------------------------------------------------------------*/
static void CacheEventInfo
(
  int32                        handle,
  ps_iface_event_enum_type     psEvent,
  ps_iface_event_info_u_type   eventInfo
)
{
  ps_iface_type*              ifacePtr;
  ps_flow_type*               flowPtr;
  ps_phys_link_type*          physLinkPtr;
  IfaceEventInfoCacheType*    ifaceEventInfoCache;
  FlowEventInfoCacheType*     flowEventInfoCache;
  PhysLinkEventInfoCacheType* physLinkEventInfoCache;

  if (0 == handle)
  {
    LOG_MSG_ERROR ("NULL handle", 0, 0, 0);
    return;
  }

  ifacePtr    = (ps_iface_type *) handle;
  flowPtr     = (ps_flow_type *) handle;
  physLinkPtr = (ps_phys_link_type *) handle;

  ifaceEventInfoCache    = &ifacePtr->event_info_cache;
  flowEventInfoCache     = &flowPtr->event_info_cache;
  physLinkEventInfoCache = &physLinkPtr->event_info_cache;


  //TODO: What is n_filters event info used for

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  switch (psEvent)
  {

    /* Phys link state events */
    case PHYS_LINK_DOWN_EV:
    case PHYS_LINK_COMING_UP_EV:
    case PHYS_LINK_UP_EV:
    case PHYS_LINK_GOING_DOWN_EV:
    case PHYS_LINK_GONE_EV:
    case PHYS_LINK_RESUMING_EV:
    case PHYS_LINK_GOING_NULL_EV:
      memcpy (&physLinkEventInfoCache->phys_link_event_info,
              &eventInfo.phys_link_event_info,
              sizeof (physLinkEventInfoCache->phys_link_event_info));
      break;

    case PHYS_LINK_FLOW_ENABLED_EV:
    case PHYS_LINK_FLOW_DISABLED_EV:
      physLinkEventInfoCache->flow_mask = eventInfo.flow_mask;
      break;

    case IFACE_UP_EV:
    case IFACE_COMING_UP_EV:
    case IFACE_CONFIGURING_EV:
    case IFACE_ROUTEABLE_EV:
    case IFACE_GOING_DOWN_EV:
      ifaceEventInfoCache->state = eventInfo.state;
      break;

    case IFACE_DOWN_EV:
      /* For Iface down event, also update the state so that
         GetPreviousState() can return correct state info */
      ifaceEventInfoCache->state = eventInfo.state;
      memcpy (&ifaceEventInfoCache->iface_down_info,
              &eventInfo.iface_down_info,
              sizeof (ifaceEventInfoCache->iface_down_info));
      break;

    case IFACE_ENABLED_EV:
    case IFACE_DISABLED_EV:
      //TODO
      break;

    case IFACE_FLOW_ENABLED_EV:
    case IFACE_FLOW_DISABLED_EV:
      ifaceEventInfoCache->flow_mask = eventInfo.flow_mask;
      break;

    case IFACE_ADDR_CHANGED_EV:
      ifaceEventInfoCache->ip_addr = eventInfo.ip_addr;
      break;

    case IFACE_ADDR_FAMILY_CHANGED_EV:
      memcpy (&ifaceEventInfoCache->addr_family_info,
              &eventInfo.addr_family_info,
              sizeof (ifaceEventInfoCache->addr_family_info));
      break;

    case IFACE_MTU_CHANGED_EV:
      //TODO
      break;

    case IFACE_DELETE_EV:
      //TODO
      break;

    case IFACE_IPFLTR_UPDATED_EV:
      memcpy (&ifaceEventInfoCache->ipfltr_info,
              &eventInfo.ipfltr_info,
              sizeof (ifaceEventInfoCache->ipfltr_info));
      break;

    case IFACE_PRI_PHYS_LINK_CHANGED_EV:
      memcpy (&physLinkEventInfoCache->pri_changed_info,
              &eventInfo.pri_changed_info,
              sizeof (physLinkEventInfoCache->pri_changed_info));
      break;

    case IFACE_VALID_RA_EV:
      ifaceEventInfoCache->ra_lifetime = eventInfo.ra_lifetime;
      break;

    case IFACE_MT_REQUEST_EV:
      ifaceEventInfoCache->mt_handle = eventInfo.mt_handle;
      break;

    case IFACE_MCAST_REGISTER_SUCCESS_EV:
    case IFACE_MCAST_REGISTER_FAILURE_EV:
    case IFACE_MCAST_DEREGISTERED_EV:
      ifaceEventInfoCache->mcast_info = eventInfo.mcast_info;
      break;


    case IFACE_BEARER_TECH_CHANGED_EV:
      memcpy (&ifaceEventInfoCache->bearer_tech_changed_info,
              &eventInfo.bearer_tech_changed_info,
              sizeof (ifaceEventInfoCache->bearer_tech_changed_info));
      break;

    case IFACE_QOS_AWARE_SYSTEM_EV:
    case IFACE_QOS_UNAWARE_SYSTEM_EV:
      ifaceEventInfoCache->qos_aware_info_code =
        eventInfo.qos_aware_info_code;
      break;

    case IFACE_FLOW_ADDED_EV:
    case IFACE_FLOW_DELETED_EV:
      //TODO
      break;

    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV:
    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV:
      ifaceEventInfoCache->hdr_rev0_rate_inertia_failure_code =
        eventInfo.hdr_rev0_rate_inertia_failure_code;
      break;


    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV:
      //TODO
      break;

    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV:
    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV:
      memcpy (&ifaceEventInfoCache->slotted_mode_info,
              &eventInfo.slotted_mode_info,
              sizeof (ifaceEventInfoCache->slotted_mode_info));
      break;


    case IFACE_PREFIX_UPDATE_EV:
      ifaceEventInfoCache->prefix_info = eventInfo.prefix_info;
      break;

    case IFACE_IPV6_PRIV_ADDR_GENERATED_EV:
    case IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV:
    case IFACE_IPV6_PRIV_ADDR_EXPIRED_EV:
    case IFACE_IPV6_PRIV_ADDR_DELETED_EV:
      ifaceEventInfoCache->priv_ipv6_addr = eventInfo.priv_ipv6_addr;
      break;

    case IFACE_OUTAGE_NOTIFICATION_EV:
      ifaceEventInfoCache->outage_notification_info =
        eventInfo.outage_notification_info;
      break;


    case IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV:
      //TODO
      break;

    case IFACE_RF_CONDITIONS_CHANGED_EV:
      ifaceEventInfoCache->rf_conditions_change_info =
        eventInfo.rf_conditions_change_info;
      break;

    case IFACE_EXTENDED_IP_CONFIG_EV:
      ifaceEventInfoCache->extended_ip_config_succeeded =
        eventInfo.extended_ip_config_succeeded;
      break;

    case IFACE_LINGERING_EV:
      //TODO
      break;

    case IFACE_MBMS_CONTEXT_ACT_SUCCESS_EV:
    case IFACE_MBMS_CONTEXT_ACT_FAILURE_EV:
    case IFACE_MBMS_CONTEXT_DEACT_SUCCESS_EV:
    case IFACE_MBMS_CONTEXT_DEACT_FAILURE_EV:
      //TODO
      break;

    case IFACE_AUTHENTICATING_EV:
    case IFACE_APP_PREEMPTED_EV:
      //TODO
      break;

    case IFACE_MCAST_STATUS_EV:
      break;

    case FLOW_NULL_EV:
    case FLOW_ACTIVATING_EV:
    case FLOW_ACTIVATED_EV:
    case FLOW_SUSPENDING_EV:
    case FLOW_SUSPENDED_EV:
    case FLOW_RESUMING_EV:
    case FLOW_GOING_NULL_EV:
    case FLOW_CONFIGURING_EV:
    case FLOW_INFO_CODE_UPDATED_EV:
      memcpy (&flowEventInfoCache->flow_event_info,
              &eventInfo.flow_event_info,
              sizeof (flowEventInfoCache->flow_event_info));
      break;

    case FLOW_TX_ENABLED_EV:
    case FLOW_TX_DISABLED_EV:
      flowEventInfoCache->flow_mask = eventInfo.flow_mask;
      break;

    case FLOW_RX_FLTR_UPDATED_EV:
      memcpy (&ifaceEventInfoCache->rx_fltr_info,
              &eventInfo.rx_fltr_info,
              sizeof (ifaceEventInfoCache->rx_fltr_info));
      break;

    case FLOW_MODIFY_ACCEPTED_EV:
      flowEventInfoCache->modify_result = PS_FLOW_MODIFY_RESULT_ACCEPTED;
      break;

    case FLOW_MODIFY_REJECTED_EV:
      flowEventInfoCache->modify_result = PS_FLOW_MODIFY_RESULT_REJECTED;
      break;

    case FLOW_PRIMARY_MODIFY_RESULT_EV:
      memcpy (&flowEventInfoCache->primary_qos_modify_result_info,
              &eventInfo.primary_qos_modify_result_info,
              sizeof (flowEventInfoCache->primary_qos_modify_result_info));
      break;

    default:
      break;

  } /* switch (psEvent) */
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* CacheEventInfo() */

/*---------------------------------------------------------------------------
  Callback function for iface, flow and phys link events.
---------------------------------------------------------------------------*/
static void IfaceEventCback
(
  ps_iface_type             *ifacePtr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type eventInfo,
  void                      *userDataPtr
)
{
  int32                      ifaceHandle;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("IfaceEventCback(), handle=%d, ev=%d, ifaceCbackFcnPtr = %p",
    (int32)ifacePtr, event, ifaceCbackFcnPtr);

  if ( NULL == ifacePtr )
  {
    LOG_MSG_ERROR ( "IfaceEventCback(), NULL iface_ptr", 0,0,0);
    return;
  }

  ifaceHandle = (int32) ifacePtr;

  /* Store event info */
  CacheEventInfo (ifaceHandle, event, eventInfo);

  if (0 != ifaceCbackFcnPtr)
  {
    ifaceCbackFcnPtr(ifaceHandle, (int32) event, eventInfo, userDataPtr);
  }
} /* IfaceEventCback() */

static void FlowEventCback
(
  ps_flow_type                 *flowPtr,
  ps_iface_event_enum_type      event,
  ps_iface_event_info_u_type    eventInfo,
  void                         *userDataPtr
)
{
  int32                         flowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#if 0
  LOG_MSG_INFO2 ("FlowEventCback(): handle 0x%p, ev %d", flowPtr, event, 0);
#endif

  if (NULL == flowPtr)
  {
    LOG_MSG_ERROR("NULL flow handle", 0, 0, 0);
    return;
  }

  flowHandle = (int32) flowPtr;

  /* Store event info */
  CacheEventInfo (flowHandle, event, eventInfo);
  if (0 != flowCbackFcnPtr)
  {
    flowCbackFcnPtr(flowHandle, (int32) event, eventInfo, userDataPtr);
  }
} /* FlowEventCback() */

static void PhysLinkEventCback
(
  ps_phys_link_type           *physLinkPtr,
  ps_iface_event_enum_type     event,
  ps_iface_event_info_u_type   eventInfo,
  void                        *userDataPtr
)
{
  int32                        physLinkHandle;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("PhysLinkEventCback() handle=%d, ev=%d",
    (int32)physLinkPtr, event, 0);

  if (NULL == physLinkPtr)
  {
    LOG_MSG_ERROR("NULL phys link handle", 0, 0, 0);
    return;
  }

  physLinkHandle = (int32) physLinkPtr;

  /* Store event info */
  CacheEventInfo (physLinkHandle, event, eventInfo);
  if (0 != physLinkCbackFcnPtr)
  {
    physLinkCbackFcnPtr(physLinkHandle, (int32) event, eventInfo, userDataPtr);
  }
} /* PhysLinkEventCback() */


/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/**
  @brief      Helper IOCTL fcn which also verified if NULL arg is passed.

  @param[in]  ifaceHandle - Interface Handle
  @param[in]  ioctlName - Name of the IOCTL
  @param      argValPtr - Pointer to arg val. This is an in/out param.

  @see        NetPlatform::IfaceIoctlEnumType

  @return     AEE_SUCCESS on successful IOCTL call
              Error code on error.
*/
int32 NetPlatform::IfaceIoctlNonNullArg
(
  int32 ifaceHandle,
  NetPlatform::IfaceIoctlEnumType ioctlName,
  void *argValPtr
)
throw()
{
  if (NULL == argValPtr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  return IfaceIoctl(ifaceHandle, ioctlName, argValPtr);

} /* IfaceIoctlNonNullArg() */

/**
  @brief      This function calls the ps_iface_ioctl to perform interface
              specific operations.

  @param[in]  ifaceHandle - Interface Handle
  @param[in]  ioctlName - Name of the IOCTL
  @param      argValPtr - Pointer to arg val. This is an in/out param.

  @see        NetPlatform::IfaceIoctlEnumType

  @return     AEE_SUCCESS on successful IOCTL call
              Error code on error.
*/
int32 NetPlatform::IfaceIoctl
(
  int32 ifaceHandle,
  NetPlatform::IfaceIoctlEnumType ioctlName,
  void *argValPtr
)
throw()
{
  ps_iface_type*   ifacePtr;
  int32            result;
  int16            psErrno;
  boolean          isEventInfoCached;
  int64            timeToOutage;
  int64            outageDuration;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("IfaceIoctl() iface handle %d, ioctl name %d",
    ifaceHandle, ioctlName, 0);

  ifacePtr = (ps_iface_type *) ifaceHandle;

  if (0 == ifacePtr)
  {
    LOG_MSG_ERROR ("NULL iface pointer", 0, 0, 0);
    return QDS_EBADF;
  }

  /*-------------------------------------------------------------------------
    If net handle associated with this object is NULL, return QDS_EBADF
    GET_ALL_IFACES is a special case, iface pointer need not be valid here.
  -------------------------------------------------------------------------*/
  if (0 == ifacePtr && IFACE_IOCTL_GET_ALL_IFACES != ioctlName)
  {
    LOG_MSG_ERROR ("NULL iface pointer", 0, 0, 0);
    return QDS_EBADF;
  }

  /*-------------------------------------------------------------------------
    For the following IOCTLs, get the cached info from iface layer.
  -------------------------------------------------------------------------*/
  isEventInfoCached = FALSE;

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  switch (ioctlName)
  {
    case IFACE_IOCTL_GET_PREVIOUS_STATE:
      if (NULL == argValPtr)
      {
        result = QDS_EFAULT;
        break;
      }

      *((ps_iface_state_enum_type *)argValPtr) =
        ifacePtr->event_info_cache.state;
      isEventInfoCached = TRUE;
      break;

    case IFACE_IOCTL_GET_PREVIOUS_IP_ADDR:
      if (NULL == argValPtr)
      {
        result = QDS_EFAULT;
        break;
      }

      memcpy (argValPtr,
              &ifacePtr->event_info_cache.ip_addr,
              sizeof (ps_ip_addr_type));
      isEventInfoCached = TRUE;
      break;

    case IFACE_IOCTL_GET_PREVIOUS_BEARER_TECHNOLOGY:
      if (NULL == argValPtr)
      {
        result = QDS_EFAULT;
        break;
      }
      memcpy (argValPtr,
              &ifacePtr->event_info_cache.bearer_tech_changed_info.old_bearer_tech,
              sizeof (ps_iface_bearer_technology_type));
      isEventInfoCached = TRUE;
      break;

    case IFACE_IOCTL_GET_QOS_AWARE_INFO_CODE:
      if (NULL == argValPtr)
      {
        result = QDS_EFAULT;
        break;
      }
      *((ps_extended_info_code_enum_type *)argValPtr) =
        ifacePtr->event_info_cache.qos_aware_info_code;
      isEventInfoCached = TRUE;
      break;

    case IFACE_IOCTL_GET_HDR_REV0_RATE_INERTIA_RESULT_INFO_CODE:
      if (NULL == argValPtr)
      {
        result = QDS_EFAULT;
        break;
      }
      *((ps_hdr_rev0_rate_inertia_failure_code_enum_type *)argValPtr) =
        ifacePtr->event_info_cache.hdr_rev0_rate_inertia_failure_code;
      isEventInfoCached = TRUE;
      break;

    case IFACE_IOCTL_GET_OUTAGE_NOTIFICATION_INFO:
      if (NULL == argValPtr)
      {
        result = QDS_EFAULT;
        break;
      }

      memcpy (argValPtr,
              &ifacePtr->event_info_cache.outage_notification_info,
              sizeof (ps_iface_outage_notification_event_info_type));

      timeToOutage = (int64)((OutageNotificationInfoType*)argValPtr)->time_to_outage - (int64)msclock();
      outageDuration = (int64)((OutageNotificationInfoType*)argValPtr)->duration;
      if (timeToOutage > 0)
      {
        ((OutageNotificationInfoType*)argValPtr)->time_to_outage = (uint32)timeToOutage;
      }
      else if (outageDuration + timeToOutage > 0)
      {
        ((OutageNotificationInfoType*)argValPtr)->time_to_outage = 0;
        ((OutageNotificationInfoType*)argValPtr)->duration = (uint32)(outageDuration + timeToOutage);
      }
      else
      {
        ((OutageNotificationInfoType*)argValPtr)->time_to_outage = 0;
        ((OutageNotificationInfoType*)argValPtr)->duration = 0;
      }
      isEventInfoCached = TRUE;
      break;

    default:
      break;
  } /* switch */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  //TODO: Not checking for EFAULT for result

  if (TRUE == isEventInfoCached)
  {
    return AEE_SUCCESS;
  }

  result = ps_iface_ioctl (ifacePtr,
                           (ps_iface_ioctl_type)ioctlName,
                           argValPtr,
                           &psErrno);
  if (DSS_ERROR == result)
  {
    LOG_MSG_ERROR ("ps iface ioctl returned -1, err %d", psErrno, 0, 0);
    result = MapErrorCode(psErrno);
  }

  return result;
}

/**
  @brief      This function calls the ps_iface_ioctl to perform interface
              specific operations.

  @param[in]  ifaceHandle - Interface Handle
  @param[in]  ioctlName - Name of the IOCTL
  @param      argValPtr - Pointer to arg val. This is an in/out param.

  @see        NetPlatform::IfaceIoctlEnumType

  @return     AEE_SUCCESS on successful IOCTL call
              Error code on error.
*/
int32 NetPlatform::IfaceIoctlByIfaceId
(
  int32                              ifaceId,
  NetPlatform::IfaceIoctlEnumType    ioctlName,
  void                             * argValPtr
)
{
  ps_iface_type *  ifacePtr;
  int32            result;
  int16            psErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Determine the specific ps_iface_ptr based on the iface ID
  -------------------------------------------------------------------------*/
  ifacePtr = ps_iface_get_handle( (ps_iface_id_type) ifaceId );
  if (ifacePtr == NULL)
  {
    LOG_MSG_ERROR("Invalid iface id: 0x%x", ifaceId, 0, 0);
    return QDS_EBADF;
  }

  result = ps_iface_ioctl (ifacePtr,
                           (ps_iface_ioctl_type)ioctlName,
                           argValPtr,
                           &psErrno);
  if (DSS_ERROR == result)
  {
    LOG_MSG_ERROR ("ps iface ioctl returned -1, err %d", psErrno, 0, 0);
    result = MapErrorCode(psErrno);
  }

  return result;
}

int32 NetPlatform::FlowIoctlNonNullArg
(
  int32                          flowHandle,
  FlowIoctlEnumType              ioctlName,
  void                          *argValPtr
)
{
  if (NULL == argValPtr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  return FlowIoctl (flowHandle, ioctlName, argValPtr);
}

/**
  @brief      This function calls the ps_flow_ioctl to perform flow
              specific operations.

  @param[in]  flowHandle - Interface Handle
  @param[in]  ioctlName - Name of the IOCTL
  @param      argValPtr - Pointer to arg val. This is an in/out param.

  @see        NetPlatform::FlowIoctlEnumType

  @return     AEE_SUCCESS on successful IOCTL call
              Error code on error.
*/
int32 NetPlatform::FlowIoctl
(
 int32 flowHandle,
 NetPlatform::FlowIoctlEnumType ioctlName,
 void *argValPtr
)
{
  ps_flow_type*    flowPtr;
  int32            result;
  int16            psErrno;
  boolean          isEventInfoCached;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("FlowIoctl() flow handle %d, ioctl name %d",
    flowHandle, ioctlName, 0);

  flowPtr = (ps_flow_type *) flowHandle;

  if (NULL == flowPtr)
  {
    LOG_MSG_ERROR ("NULL flow handle", 0, 0, 0);
    return AEE_SUCCESS;
  }

  isEventInfoCached = FALSE;

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  switch (ioctlName)
  {
    case FLOW_IOCTL_GET_MODIFY_RESULT:
    isEventInfoCached = TRUE;

    *((QoSResultType *)argValPtr) =
      (QoSResultType)
      flowPtr->event_info_cache.modify_result;
    break;


    case FLOW_IOCTL_GET_PRIMARY_QOS_MODIFY_RESULT:
    isEventInfoCached = TRUE;

    *((boolean *)argValPtr) =
      flowPtr->event_info_cache.primary_qos_modify_result_info. \
      is_modify_succeeded;
    break;


    case FLOW_IOCTL_GET_FLOW_UPDATED_INFO_CODE:
    isEventInfoCached = TRUE;

    *((QoSInfoCodeType*)argValPtr) =
      (QoSInfoCodeType)
      flowPtr->event_info_cache.flow_event_info.info_code;
    break;

    default:
    /* Do nothing */
    break;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  if (TRUE == isEventInfoCached)
  {
    return AEE_SUCCESS;
  }

  result = ps_flow_ioctl (flowPtr,
                          (ps_flow_ioctl_type)ioctlName,
                          argValPtr,
                          &psErrno);
  if (-1 == result)
  {
    LOG_MSG_ERROR ("ps flow ioctl failed, err %d", psErrno, 0, 0);
    result = MapErrorCode(psErrno);
  }

  return result;
}

/**
  @brief      This function calls the ps_phys_link_ioctl to perform phys
              link specific operations.

  @param[in]  physLinkHandle - Interface Handle
  @param[in]  ioctlName - Name of the IOCTL
  @param      argValPtr - Pointer to arg val. This is an in/out param.

  @see        NetPlatform::PhysLinkIoctlEnumType

  @return     AEE_SUCCESS on successful IOCTL call
              Error code on error.
*/
int32 NetPlatform::PhysLinkIoctl
(
 int32 physLinkHandle,
 NetPlatform::PhysLinkIoctlEnumType ioctlName,
 void *argValPtr
)
{
  ps_phys_link_type*   physLinkPtr;
  int32                result;
  int16                psErrno;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("phys_link_ioctl() handle %d, ioctl name %d",
    physLinkHandle, ioctlName, 0);

  physLinkPtr = (ps_phys_link_type *) physLinkHandle;

  if (NULL == physLinkPtr)
  {
    LOG_MSG_ERROR ("NULL phys link handle", 0, 0, 0);
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    For the GET_PREVIOUS_STATE IOCTL get the cached event info.
  -------------------------------------------------------------------------*/
  if (PHYS_LINK_IOCTL_GET_PREVIOUS_STATE == ioctlName)
  {
    if (NULL == argValPtr)
    {
      return QDS_EFAULT;
    }

    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    (*(phys_link_state_type*)argValPtr) =
      physLinkPtr->event_info_cache.phys_link_event_info.state;

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    return AEE_SUCCESS;
  }

  if (PHYS_LINK_IOCTL_GET_DORMANCY_INFO_CODE == ioctlName)
  {
    if (NULL == argValPtr)
    {
      return QDS_EFAULT;
    }

    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    (*(ps_extended_info_code_enum_type *)argValPtr) =
      physLinkPtr->dormancy_info_code;

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    return AEE_SUCCESS;
  }

  result = ps_phys_link_ioctl (physLinkPtr,
                               (ps_phys_link_ioctl_type)ioctlName,
                               argValPtr,
                               &psErrno);
  if (-1 == result)
  {
    LOG_MSG_ERROR ("ps phys link ioctl failed, err %d", psErrno, 0, 0);
    result = MapErrorCode(psErrno);
  }

  return result;
}


int32 NetPlatform::IfaceRegAllEvents
(
  void*  userDataPtr
)
{
  int32 result;
  /*-------------------------------------------------------------------------
    The event registration functions need to be called only once. This static
    variable keep track of if the registration is happening more than once.
  -------------------------------------------------------------------------*/
  static boolean ifaceEventsRegistered = FALSE;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("IfaceRegAllEvents() called", 0, 0, 0);

  if (TRUE == ifaceEventsRegistered)
  {
    LOG_MSG_ERROR ("IfaceRegAllEvents() called more than once!", 0, 0, 0);
    ASSERT (0);
    return AEE_EFAILED;
  }

  /*-----------------------------------------------------------------------
     Register cbacks for events with psiface for FLOW ENABLED.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_flow_enabled_cback =
                 ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                                userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_iface_event_cback_reg( NULL,
                               IFACE_FLOW_ENABLED_EV,
                               EventCbackInfo.iface_flow_enabled_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IP_ADDR_CHANGED event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.ip_addr_changed_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_ADDR_CHANGED_EV,
                               EventCbackInfo.ip_addr_changed_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

#ifdef FEATURE_DATA_PS_IPV6
  /*-----------------------------------------------------------------------
    Register cbacks for the prefix updated event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.prefix_updated_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_PREFIX_UPDATE_EV,
                               EventCbackInfo.prefix_updated_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }
#endif /* FEATURE_DATA_PS_IPV6 */

  /*-----------------------------------------------------------------------
    Register cbacks for the Address family changed event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.addr_family_changed_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_ADDR_FAMILY_CHANGED_EV,
                               EventCbackInfo.addr_family_changed_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for  IP filter updated event
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_ipfltr_updated_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_IPFLTR_UPDATED_EV,
                               EventCbackInfo.iface_ipfltr_updated_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_UP event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_up_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_UP_EV,
                               EventCbackInfo.iface_up_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_CONFIGURING event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_configuring_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_CONFIGURING_EV,
                               EventCbackInfo.iface_configuring_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_ROUTEABLE event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_routeable_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_ROUTEABLE_EV,
                               EventCbackInfo.iface_routeable_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_DOWN event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_down_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_DOWN_EV,
                               EventCbackInfo.iface_down_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_COMING_UP event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_coming_up_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_COMING_UP_EV,
                               EventCbackInfo.iface_coming_up_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_GOING_DOWN event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_going_down_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_GOING_DOWN_EV,
                               EventCbackInfo.iface_going_down_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the IFACE_GOING_DOWN event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_lingering_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_LINGERING_EV,
                               EventCbackInfo.iface_lingering_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }


  /*-----------------------------------------------------------------------
    Register cbacks for the MT event with ps_iface
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_mt_request_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_MT_REQUEST_EV,
                               EventCbackInfo.iface_mt_request_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the Multicast events with ps_iface
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_mcast_reg_success_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_MCAST_REGISTER_SUCCESS_EV,
                               EventCbackInfo.iface_mcast_reg_success_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_mcast_reg_failure_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_MCAST_REGISTER_FAILURE_EV,
                               EventCbackInfo.iface_mcast_reg_failure_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_mcast_dereg_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_MCAST_DEREGISTERED_EV,
                               EventCbackInfo.iface_mcast_dereg_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_idle_event_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                                IFACE_IDLE_EV,
                                EventCbackInfo.iface_idle_event_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /* BCMCS 2.0 Support */
#ifdef FEATURE_HDR_BCMCS_2p0
  if ((EventCbackInfo.iface_mcast_status_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_MCAST_STATUS_EV,
                               EventCbackInfo.iface_mcast_status_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }
#endif /* FEATURE_HDR_BCMCS_2p0 */

  /*-----------------------------------------------------------------------
    Register cbacks for the bearer technology change event with ps_iface
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_bearer_tech_change_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                         IFACE_BEARER_TECH_CHANGED_EV,
                         EventCbackInfo.iface_bearer_tech_change_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cback for the extended IP config event with ps_iface
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_extended_ip_config_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                         IFACE_EXTENDED_IP_CONFIG_EV,
                         EventCbackInfo.iface_extended_ip_config_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

#ifdef FEATURE_DATA_PS_QOS
  /*-----------------------------------------------------------------------
    Register cbacks for the primary phys link changed event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_pri_phys_link_changed_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(NULL,
                      IFACE_PRI_PHYS_LINK_CHANGED_EV,
                      EventCbackInfo.iface_pri_phys_link_changed_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for QOS_AWARE and QOS_UNAWARE events.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_qos_aware_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_QOS_AWARE_SYSTEM_EV,
                               EventCbackInfo.iface_qos_aware_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_qos_unaware_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg( NULL,
                               IFACE_QOS_UNAWARE_SYSTEM_EV,
                               EventCbackInfo.iface_qos_unaware_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }


  /*-----------------------------------------------------------------------
    Register cbacks for NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_network_supported_qos_profiles_changed_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg
      (
        NULL,
        IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV,
        EventCbackInfo.iface_network_supported_qos_profiles_changed_cback
      ) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }
#endif /* FEATURE_DATA_PS_QOS */

  /*-----------------------------------------------------------------------
    Register cbacks for OUTAGE_NOTIFICATION_EV
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_outage_notification_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg
      (
        NULL,
        IFACE_OUTAGE_NOTIFICATION_EV,
        EventCbackInfo.iface_outage_notification_cback
      ) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for RF_CONDITIONS_CHANGED_EV
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_rf_conditions_change_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg
      (
        NULL,
        IFACE_RF_CONDITIONS_CHANGED_EV,
        EventCbackInfo.iface_rf_conditions_change_cback
      ) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for APP_PREEMPTED_EV
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_app_preempted_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg
      (
        NULL,
        IFACE_APP_PREEMPTED_EV,
        EventCbackInfo.iface_app_preempted_cback
      ) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for HDR mode success/failure  events.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_hdr_rev0_rate_inertia_success_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV,
              EventCbackInfo.iface_hdr_rev0_rate_inertia_success_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_hdr_rev0_rate_inertia_failure_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
             NULL,
             IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV,
             EventCbackInfo.iface_hdr_rev0_rate_inertia_failure_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for eidle slotted mode success/failure events.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_hdr_slotted_mode_success_cback =
               ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                              userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV,
              EventCbackInfo.iface_hdr_slotted_mode_success_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_hdr_slotted_mode_failure_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV,
              EventCbackInfo.iface_hdr_slotted_mode_failure_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_hdr_slotted_mode_session_changed_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_iface_event_cback_reg(
    NULL,
    IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV,
    EventCbackInfo.iface_hdr_slotted_mode_session_changed_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

#ifdef FEATURE_DATA_PS_IPV6
  /*-----------------------------------------------------------------------
    Register cbacks for iface IPv6 Privacy Extensions events.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_ipv6_priv_addr_generated_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_IPV6_PRIV_ADDR_GENERATED_EV,
              EventCbackInfo.iface_ipv6_priv_addr_generated_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_ipv6_priv_addr_deprecated_cback =
          ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                         userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV,
              EventCbackInfo.iface_ipv6_priv_addr_deprecated_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_ipv6_priv_addr_deleted_cback =
          ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                         userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_IPV6_PRIV_ADDR_DELETED_EV,
              EventCbackInfo.iface_ipv6_priv_addr_deleted_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

#endif
  /*-----------------------------------------------------------------------
    Register cbacks for iface flow added/deleted events.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.iface_flow_added_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
              NULL,
              IFACE_FLOW_ADDED_EV,
              EventCbackInfo.iface_flow_added_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  if ((EventCbackInfo.iface_flow_deleted_cback =
              ps_iface_alloc_event_cback_buf(IfaceEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_iface_event_cback_reg(
             NULL,
             IFACE_FLOW_DELETED_EV,
             EventCbackInfo.iface_flow_deleted_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }


  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Ev reg failed, err %d", result, 0, 0);
  ASSERT (0);
  return result;

}


int32 NetPlatform::FlowRegAllEvents
(
  void*  userDataPtr
)
{
  int32 result;
  /*-------------------------------------------------------------------------
    The event registration functions need to be called only once. This static
    variable keep track of if the registration is happening more than once.
  -------------------------------------------------------------------------*/
  static boolean flowEventsRegistered = FALSE;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("FlowRegAllEvents() called", 0, 0, 0);
  if (TRUE == flowEventsRegistered)
  {
    LOG_MSG_ERROR ("FlowRegAllEvents() called more than once!", 0, 0, 0);
    ASSERT (0);
    return AEE_EFAILED;
  }

#ifdef FEATURE_DATA_PS_QOS
  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_ACTIVATED event with ps_flow.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_activate_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_flow_event_cback_reg(NULL,
                              FLOW_ACTIVATED_EV,
                              EventCbackInfo.flow_activate_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_SUSPENDED event with ps_flow.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_suspended_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_flow_event_cback_reg(NULL,
                              FLOW_SUSPENDED_EV,
                              EventCbackInfo.flow_suspended_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_NULL event with ps_flow.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_null_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_flow_event_cback_reg(NULL,
                              FLOW_NULL_EV,
                              EventCbackInfo.flow_null_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_MODIFY_ACCEPTED event with ps_flow.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_modify_accepted_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_flow_event_cback_reg(NULL,
                              FLOW_MODIFY_ACCEPTED_EV,
                              EventCbackInfo.flow_modify_accepted_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_MODIFY_REJECTED event with ps_flow.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_modify_rejected_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_flow_event_cback_reg(NULL,
                              FLOW_MODIFY_REJECTED_EV,
                              EventCbackInfo.flow_modify_rejected_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_PRIMARY_MODIFY_RESULT event with ps_flow
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_primary_modify_result_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  result =
    ps_flow_event_cback_reg(NULL,
                            FLOW_PRIMARY_MODIFY_RESULT_EV,
                            EventCbackInfo.flow_primary_modify_result_cback);
  if (result != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_INFO_CODE_UPDATED event with ps_flow
  -----------------------------------------------------------------------*/
  if ( ( EventCbackInfo.flow_info_code_updated_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  result =
    ps_flow_event_cback_reg(NULL,
                            FLOW_INFO_CODE_UPDATED_EV,
                            EventCbackInfo.flow_info_code_updated_cback);
  if (result != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }
#endif /* FEATURE_DATA_PS_QOS */

  /*-----------------------------------------------------------------------
    Register cbacks for the FLOW_TX_ENABLED event with ps_flow.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.flow_tx_enabled_cback =
              ps_flow_alloc_event_cback_buf(FlowEventCback,
                                            userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (ps_flow_event_cback_reg(NULL,
                              FLOW_TX_ENABLED_EV,
                              EventCbackInfo.flow_tx_enabled_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }


  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Ev reg failed, err %d", result, 0, 0);
  ASSERT (0);
  return result;

} /* FlowRegAllEvents() */


int32 NetPlatform::PhysLinkRegAllEvents
(
  void*  userDataPtr
)
{
  int32 result;
  /*-------------------------------------------------------------------------
    The event registration functions need to be called only once. This static
    variable keep track of if the registration is happening more than once.
  -------------------------------------------------------------------------*/
  static boolean physLinkEventsRegistered = FALSE;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("PhysLinkRegAllEvents() called", 0, 0, 0);

  if (TRUE == physLinkEventsRegistered)
  {
    LOG_MSG_ERROR ("PhysLinkRegAllEvents() called more than once!", 0, 0, 0);
    ASSERT (0);
    return AEE_EFAILED;
  }

  /*-----------------------------------------------------------------------
     Register cbacks for events with physlink for FLOW ENABLED.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_flow_enabled_cback =
     ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                        userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg(NULL,
                                  PHYS_LINK_FLOW_ENABLED_EV,
                                  EventCbackInfo.phys_link_flow_enabled_cback)
     != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_UP event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_up_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_UP_EV,
                               EventCbackInfo.phys_link_up_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_DOWN event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_down_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_DOWN_EV,
                               EventCbackInfo.phys_link_down_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_GONE event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_gone_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                                userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_GONE_EV,
                                   EventCbackInfo.phys_link_gone_cback) != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_COMING_UP_EV event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_coming_up_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                             userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_COMING_UP_EV,
                                   EventCbackInfo.phys_link_coming_up_cback)
     != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_GOING_DOWN_EV event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_going_down_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                                userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_GOING_DOWN_EV,
                                   EventCbackInfo.phys_link_going_down_cback)
     != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_GOING_NULL_EV event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_going_null_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                                userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_GOING_NULL_EV,
                                   EventCbackInfo.phys_link_going_null_cback)
     != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Register cbacks for the PHYS_LINK_RESUMING_EV event with psiface.
  -----------------------------------------------------------------------*/
  if ((EventCbackInfo.phys_link_resuming_cback =
             ps_phys_link_alloc_event_cback_buf(PhysLinkEventCback,
                                                userDataPtr)) == NULL)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  if (ps_phys_link_event_cback_reg( NULL,
                                   PHYS_LINK_RESUMING_EV,
                                   EventCbackInfo.phys_link_resuming_cback)
     != 0)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Ev reg failed, err %d", result, 0, 0);
  ASSERT (0);
  return result;

}


int32 NetPlatform::IfaceBringUpCmd
(
 int32 ifaceHandle,
 void *userDataPtr
)
{
  ps_iface_type*   ifacePtr;
  int32            result;
  int16            psErrno;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("IfaceBringUpCmd() iface handle %d", ifaceHandle, 0, 0);

  ifacePtr = (ps_iface_type *) ifaceHandle;

  if (NULL == ifacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EBADF;
  }

  result = ps_iface_bring_up_cmd(ifacePtr, &psErrno, userDataPtr);

  if (DSS_ERROR ==  result)
  {
    if (DS_EWOULDBLOCK != psErrno)
    {
      LOG_MSG_ERROR ("IfaceBringUpCmd() returned %d, err %d", result, psErrno, 0);
    }
    result = MapErrorCode (psErrno);
  }

  return result;

} /* IfaceBringUpCmd() */


int32 NetPlatform::IfaceTearDownCmd
(
 int32 ifaceHandle,
 void *userDataPtr
)
{
  ps_iface_type*   ifacePtr;
  int32            result;
  int16            psErrno;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("IfaceTearDownCmd() iface handle %d", ifaceHandle, 0, 0);

  ifacePtr = (ps_iface_type *) ifaceHandle;

  if (NULL == ifacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EBADF;
  }

  result = ps_iface_tear_down_cmd(ifacePtr, &psErrno, userDataPtr);

  if (-1 == result)
  {
    if (DS_EWOULDBLOCK != psErrno)
    {
      LOG_MSG_ERROR ("IfaceTearDownCmd() failed, err %d", result, 0, 0);
    }
    result = MapErrorCode (psErrno);
  }

  return result;

} /* IfaceTearDownCmd() */


int32 NetPlatform::IfaceTearDownCmdEx
(
 int32     ifaceHandle,
 int32     appPriority,
 void *    userDataPtr
)
{
  ps_iface_type*   ifacePtr;
  int32            result;
  int16            psErrno;
/*-------------------------------------------------------------------------*/

  LOG_MSG_INFO2 ("IfaceTearDownCmd() iface handle %d", ifaceHandle, 0, 0);

  ifacePtr = (ps_iface_type *) ifaceHandle;

  if (NULL == ifacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EBADF;
  }

  result = ps_iface_tear_down_cmd_ex (ifacePtr,
                                      appPriority,
                                      &psErrno,
                                      userDataPtr);

  if (-1 == result)
  {
    if (DS_EWOULDBLOCK != psErrno)
    {
      LOG_MSG_ERROR ("IfaceTearDownCmd() failed, err %d", result, 0, 0);
    }
    result = MapErrorCode (psErrno);
  }

  return result;

} /* IfaceTearDownCmd() */

int32 NetPlatform::IfaceLookUpByPolicy
(
  IPolicy *    pIDSNetPolicy,
  int32 *      pIfaceHandle
)
{
  NetPlatform::ACLPolicyInfoType  aclPolicy;
  int32                           result = AEE_SUCCESS;
  ps_iface_type *                 ifacePtr;
  uint32                          ifacePrioMask = 0;
  boolean                         arbCandidate = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pIfaceHandle)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  result = GenerateAclPolicy (pIDSNetPolicy, &aclPolicy);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  aclPolicy.lookup_only = TRUE;
  aclPolicy.bring_up    = FALSE;

  ifacePtr = ps_route_look_up_iface_by_policy (&aclPolicy, &arbCandidate, &ifacePrioMask);
  if (ifacePtr == NULL)
  {
    result = QDS_ENETNONET;
    goto bail;
  }

  *pIfaceHandle = (int32) ifacePtr;
  LOG_MSG_INFO2 ("Lookup succeeded", 0, 0, 0);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Lookup failed, err %d", result, 0, 0);
  return result;
}

int32 NetPlatform::IfaceBringUpByPolicy
(
  IPolicy *    pIDSNetPolicy,
  int32 *      pIfaceHandle,
  int32 *      pAppPriority
)
{
  NetPlatform::ACLPolicyInfoType  aclPolicy;
  int32                           result = AEE_SUCCESS;
  ps_iface_type *                 ifacePtr;
  int16                           psErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pIfaceHandle)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  if (NULL == pAppPriority)
  {
    ASSERT (0);
    return QDS_EINVAL;
  }

  result = GenerateAclPolicy (pIDSNetPolicy, &aclPolicy);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  aclPolicy.lookup_only = FALSE;
  aclPolicy.bring_up    = TRUE;

  result = ps_route_bring_up_iface_by_policy (&aclPolicy, &ifacePtr, &psErrno);
  if (-1 == result)
  {
    result = MapErrorCode (psErrno);
  }

  *pAppPriority = ps_policy_mgr_get_priority
                 (
                   aclPolicy.app_identifier,
                   aclPolicy.data_session_profile_id
                 );

  if (AEE_SUCCESS == result ||
      AEE_EWOULDBLOCK == result ||
      QDS_ENETCLOSEINPROGRESS == result)
  {
    *pIfaceHandle = (int32) ifacePtr;
  }
  else
  {
    goto bail;
  }

  return result;

bail:
  LOG_MSG_ERROR ("Bringup failed, err %d", result, 0, 0);
  return result;
}



int32 NetPlatform::PhysLinkUpCmd
(
 int32 physLinkHandle,
 void *userDataPtr
)
{
  ps_phys_link_type*   physLinkPtr;
  int32                result;
  int16                psErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("PhysLinkUpCmd() phys link handle %d", physLinkHandle, 0, 0);

  physLinkPtr = (ps_phys_link_type *) physLinkHandle;

  if (NULL == physLinkPtr)
  {
    LOG_MSG_ERROR ("NULL phys link handle", 0, 0, 0);
    return QDS_EINVAL;
  }

  result = ps_phys_link_up_cmd(physLinkPtr, &psErrno, userDataPtr);
  if (-1 == result)
  {
    result = MapErrorCode (psErrno);
  }
  else
  {
    result = AEE_SUCCESS;
  }

  return result;

} /* PhysLinkUpCmd() */


int32 NetPlatform::IfaceGetAddr
(
  int32                     ifaceHandle,
  const ds::INAddr6Type *   v6RemoteIPAddrPtr,
  ds::INAddr6Type *         ifaceIPAddr
)
{
  ps_iface_type *       psIfacePtr;
  ps_ip_addr_type       ipAddr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == v6RemoteIPAddrPtr || 0 == ifaceIPAddr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return QDS_EFAULT;
  }

  psIfacePtr = reinterpret_cast <ps_iface_type *> ( ifaceHandle);

  if (NULL == psIfacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EINVAL;
  }

  ps_iface_get_ipv6_addr
  (
    psIfacePtr,
    reinterpret_cast <const struct ps_in6_addr *> ( v6RemoteIPAddrPtr),
    &ipAddr
  );

  if (ipAddr.type != IPV6_ADDR)
  {
    LOG_MSG_ERROR( "Invalid addr type %d expected %d",
                   ipAddr.type, IPV6_ADDR, 0);
    return QDS_EINVAL;
  }

  memcpy( ifaceIPAddr, ipAddr.addr.v6.ps_s6_addr, sizeof( ds::INAddr6Type));
  return AEE_SUCCESS;

} /* NetPlatform::IfaceGetAddr() */


boolean NetPlatform::IsDefaultIPv6AddrStateValid
(
  int32  psIfaceHandle
)
{
#ifdef FEATURE_DATA_PS_IPV6
  ps_iface_type *  psIfacePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  psIfacePtr = reinterpret_cast < ps_iface_type *> ( psIfaceHandle);
  if (NULL == psIfacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return FALSE;
  }

  return PS_IFACE_IPV6_DEFAULT_ADDR_STATE_VALID( psIfacePtr);
#else
  return FALSE;
#endif

} /* NetPlatform::IsDefaultIPv6AddrStateValid() */


int32 NetPlatform::IPv6PrivAddrIncRefCnt
(
  const ds::INAddr6Type *  ipAddrPtr,
  ds::ErrorType *          dsErrnoPtr
)
{
#ifdef FEATURE_DATA_PS_IPV6
  struct ps_in6_addr *  privIPv6AddrPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == ipAddrPtr || 0 == dsErrnoPtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return -1;
  }

  privIPv6AddrPtr = (struct ps_in6_addr *) ipAddrPtr;
  if (-1 == ps_ip_addr_ipv6_priv_addr_inc_ref_cnt( privIPv6AddrPtr))
  {
    *dsErrnoPtr = QDS_EINVAL;
    return -1;
  }

  return DSS_SUCCESS;
#else
  *dsErrnoPtr = DS_EOPNOTSUPP;
  return DSS_ERROR;
#endif
} /* NetPlatform::IPv6PrivAddrIncRefCnt() */


int32 NetPlatform::IPv6PrivAddrDecRefCnt
(
  const ds::INAddr6Type *  ipAddrPtr,
  ds::ErrorType *          dsErrnoPtr
)
throw()
{
#ifdef FEATURE_DATA_PS_IPV6
  struct ps_in6_addr *  privIPv6AddrPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == ipAddrPtr || 0 == dsErrnoPtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return -1;
  }

  privIPv6AddrPtr = (struct ps_in6_addr *) ipAddrPtr;
  if (-1 == ps_ip_addr_ipv6_priv_addr_dec_ref_cnt( privIPv6AddrPtr))
  {
    *dsErrnoPtr = QDS_EINVAL;
    return -1;
  }

  return DSS_SUCCESS;
#else
  *dsErrnoPtr = DS_EOPNOTSUPP;
  return DSS_ERROR;
#endif
} /* NetPlatform::IPv6PrivAddrDecRefCnt() */


int32 NetPlatform::RouteDataPathLookup
(
  ip_pkt_info_type *  ipPktPtr,
  bool                isSystemSocket,
  IPolicy *           policyObjPtr,
  boolean             isAddrBasedRoutingOnly,
  int32 *             ifaceHandlePtr
)
{
  ps_iface_type *                    psIfacePtr;
  int32                              retVal;
  NetPlatform::ACLPolicyInfoType     aclPolicy;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == ipPktPtr || 0 == ifaceHandlePtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return -1;
  }

  retVal = GenerateAclPolicy (policyObjPtr, &aclPolicy);
  if (AEE_SUCCESS != retVal)
  {
    LOG_MSG_ERROR ("Err getting ACL policy, %d", retVal, 0, 0);
    return retVal;
  }

  aclPolicy.bring_up    = FALSE;
  aclPolicy.lookup_only = FALSE;

  /*-------------------------------------------------------------------------
    Set data path policy flag appropriately
      System socket    - POLICY_NOT_DOWN
      Routeable socket - POLICY_UP_OR_ROUTEABLE
      Regular socket   - POLICY_UP_ONLY
  -------------------------------------------------------------------------*/
  if (isSystemSocket)
  {
    aclPolicy.data_path_policy_flag = PS_ACL_POLICY_NOT_DOWN;
  }
  else if (aclPolicy.is_routeable)
  {
    aclPolicy.data_path_policy_flag = PS_ACL_POLICY_UP_OR_ROUTEABLE;
  }
  else
  {
    aclPolicy.data_path_policy_flag = PS_ACL_POLICY_UP_ONLY;
  }

  retVal = ps_route_data_path_lookup( ipPktPtr,
                                    &aclPolicy,
                                    isAddrBasedRoutingOnly,
                                    &psIfacePtr);

  *ifaceHandlePtr = reinterpret_cast <int32> ( psIfacePtr);
  return retVal;

} /* NetPlatform::RouteDataPathLookup() */


uint32 NetPlatform::IPFltrExecute
(
  int32                                ifaceHandle,
  ps_iface_ipfltr_client_id_enum_type  ipFltrClientID,
  ip_pkt_info_type *                   ipPktInfoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == ipPktInfoPtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return 0;
  }

  if (0 == ifaceHandle)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return 0;
  }

  return
    ps_iface_ipfltr_execute( reinterpret_cast <ps_iface_type *> ( ifaceHandle),
                             ipFltrClientID,
                             PS_IFACE_IPFLTR_SUBSET_ID_DEFAULT,
                             ipPktInfoPtr);

} /* NetPlatform::IPFltrExecute() */


int32 NetPlatform::GetDefaultPSFlowFromRtMetaInfo
(
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  void *  routingCachePtr;
  int32   psIfaceHandle;
  int32   defaultPSFlowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == rtMetaInfoPtr)
  {
    LOG_MSG_ERROR("NULL rt meta info", 0, 0, 0);
    return 0;
  }

  routingCachePtr = PS_RT_META_GET_ROUTING_CACHE( rtMetaInfoPtr);
  psIfaceHandle   = reinterpret_cast <int32> ( routingCachePtr);

  (void) PSGetDefaultFlow( psIfaceHandle, &defaultPSFlowHandle);

  return defaultPSFlowHandle;
} /* NetPlatform::GetDefaultPSFlowFromRtMetaInfo() */


int32 NetPlatform::GetPSFlowFromRtMetaInfo
(
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  ps_flow_type *  psFlowPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == rtMetaInfoPtr)
  {
    LOG_MSG_ERROR("NULL rt meta info", 0, 0, 0);
    return 0;
  }

  psFlowPtr = PS_FLOW_GET_FLOW_FROM_RT_META_INFO( rtMetaInfoPtr);
  return reinterpret_cast <int32> ( psFlowPtr);

} /* NetPlatform::GetPSFlowFromRtMetaInfo() */


int32 NetPlatform::GetPhysLinkFromRtMetaInfo
(
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  ps_flow_type *  psFlowPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == rtMetaInfoPtr)
  {
    LOG_MSG_ERROR("NULL rt meta info", 0, 0, 0);
    return 0;
  }

  psFlowPtr = PS_FLOW_GET_FLOW_FROM_RT_META_INFO( rtMetaInfoPtr);
  return reinterpret_cast <int32> ( PS_FLOW_GET_PHYS_LINK( psFlowPtr));

} /* NetPlatform::GetPhysLinkFromRtMetaInfo() */


uint8 NetPlatform::PSGetIfaceIndex
(
  int32 ifaceHandle
)
{
  return (uint8) PS_IFACE_GET_IFACE_INDEX((ps_iface_type *)ifaceHandle);
}

uint8 NetPlatform::PSGetFlowInstance
(
  int32 ifaceHandle
)
{
  //TODO: How do we find out the flow instance from iface pointer?
  return 0;
}


int32 NetPlatform::PSGetPhysLink
(
  int32     ifaceHandle,
  int32    *physLinkHandle
)
{

  if (NULL == physLinkHandle)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *physLinkHandle = 0;

  if (0 == ifaceHandle)
  {
    return AEE_EFAILED;
  }

  *physLinkHandle = (int32) PS_IFACE_GET_PHYS_LINK ((ps_iface_type *)ifaceHandle);
  return AEE_SUCCESS;

} /* GetPhysLink() */

int32 NetPlatform::PSGetDefaultFlow
(
  int32  argIfaceHandle,
  int32* defaultFlowPtr
)
{
  ps_iface_type *ifacePtr = (ps_iface_type *) argIfaceHandle;

  if (NULL == defaultFlowPtr)
  {
    LOG_MSG_ERROR ("Inv. args", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (0 == argIfaceHandle)
  {
    LOG_MSG_ERROR ("NULL iface ptr", 0, 0, 0);
    return QDS_EINVAL;
  }

  *defaultFlowPtr = (int32) PS_IFACE_GET_DEFAULT_FLOW(ifacePtr);
  return AEE_SUCCESS;

} /* PSGetDefaultFlow() */

int32 NetPlatform::PSGetPhysLinkFromFlow
(
  int32   flowHandle,
  int32*  physLinkHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == physLinkHandle)
  {
    LOG_MSG_ERROR ("NULL args to GetPhysLinkFromFlow", 0, 0, 0);
    goto bail;
  }

  *physLinkHandle = (int32) PS_FLOW_GET_PHYS_LINK((ps_flow_type *)flowHandle);
  if (0 == *physLinkHandle)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  return QDS_EINVAL;

} /* PSGetPhysLinkFromFlow() */

int32 NetPlatform::PSGetPhysLinkFromIface
(
  int32  ifaceHandle,
  int32* physLinkHandle
)
{
  if (NULL == physLinkHandle)
  {
    LOG_MSG_ERROR ("NULL args to GetPhysLinkFromIface", 0, 0, 0);
    goto bail;
  }

  if (0 == ifaceHandle)
  {
    *physLinkHandle = 0;
    goto bail;
  }

  *physLinkHandle = (int32) PS_IFACEI_GET_PHYS_LINK((ps_iface_type *)ifaceHandle);
  if (0 == *physLinkHandle)
  {
    goto bail;
  }
  return AEE_SUCCESS;

bail:
  return AEE_EFAILED;

} /* PSGetPhysLinkFromIface() */

int32 NetPlatform::PSGetIfaceId
(
  int32  ifaceHandle,
  int32* ifaceId
)
{
  *ifaceId = PS_IFACE_GET_ID((ps_iface_type *) ifaceHandle);
  return AEE_SUCCESS;
}

int32 NetPlatform::PSIfaceIPFilterAdd
(
  int32                                   ifaceHandle,
  ps_iface_ipfltr_client_id_enum_type     clientID,
  const PSIfaceIPFilterAddParamType     * fltrParams,
  int32                                 * fltrHandle
)
{
  ps_iface_type*        ifacePtr;
  int16                 psErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == fltrHandle)
  {
    return QDS_EFAULT;
  }

  ifacePtr = (ps_iface_type *)ifaceHandle;
  if (!PS_IFACE_IS_VALID(ifacePtr))
  {
    return QDS_EFAULT;
  }

  *fltrHandle = ps_iface_ipfltr_add(ifacePtr, clientID, fltrParams, &psErrno);
  if (PS_IFACE_IPFLTR_INVALID_HANDLE == *fltrHandle)
  {
    return MapErrorCode(psErrno);
  }
  return AEE_SUCCESS;

} /* PSIfaceIPFilterAdd() */

int32 NetPlatform::PSIfaceIPFilterDelete
(
  int32                                  ifaceHandle,
  ps_iface_ipfltr_client_id_enum_type    clientId,
  int32                                  fltrHandle
)
throw()
{
  ps_iface_type*        ifacePtr;
  int16                 psErrno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ifacePtr = (ps_iface_type *)ifaceHandle;
  if (!PS_IFACE_IS_VALID(ifacePtr))
  {
    return QDS_EFAULT;
  }

  if (-1 == ps_iface_ipfltr_delete (ifacePtr, clientId, fltrHandle, &psErrno))
  {
    return MapErrorCode (psErrno);
  }

  return AEE_SUCCESS;

} /* PSIfaceIPFilterDelete() */


bool NetPlatform::IsPSIfaceTxEnabled
(
  int32  psIfaceHandle
)
{
  ps_iface_type *  psIfacePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  psIfacePtr = reinterpret_cast < ps_iface_type *> ( psIfaceHandle);
  if (TRUE == PS_IFACE_FLOW_ENABLED( psIfacePtr))
  {
    return true;
  }

  return false;
} /* NetPlatform::IsPSIfaceTxEnabled() */


bool NetPlatform::IsPSIfaceCommonTxEnabled
(
  int32  psIfaceHandle,
  int32  psFlowHandle
)
{
  ps_iface_type *  psIfacePtr;
  ps_flow_type *   psFlowPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  psIfacePtr = reinterpret_cast < ps_iface_type *> ( psIfaceHandle);
  psFlowPtr  = reinterpret_cast < ps_flow_type *> ( psFlowHandle);

  if (TRUE == PS_IFACE_COMMON_IS_FLOW_ENABLED( psIfacePtr, psFlowPtr))
  {
    return true;
  }

  return false;
} /* NetPlatform::IsPSIfaceCommonTxEnabled() */

bool NetPlatform::IsPSIfaceCommonTxDoSEnabled
(
  int32  psIfaceHandle,
  int32  psFlowHandle
)
{
  ps_iface_type *  psIfacePtr;
  ps_flow_type *   psFlowPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  psIfacePtr = reinterpret_cast < ps_iface_type *> ( psIfaceHandle);
  psFlowPtr  = reinterpret_cast < ps_flow_type *> ( psFlowHandle);

  if (PS_IFACE_FLOW_ENABLED(psIfacePtr) &&
      PS_FLOW_IS_TX_ENABLED(psFlowPtr))
  {
     return true;
  }

  return false;
} /* NetPlatform::IsPSIfaceCommonTxDoSEnabled() */

int32 NetPlatform::GetQoSStatus
(
  int32             flowHandle,
  QoSStatusType *   pQoSStatus
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pQoSStatus)
  {
    return QDS_EFAULT;
  }

  switch (PS_FLOWI_GET_STATE((ps_flow_type *)flowHandle))
  {
    case FLOW_ACTIVATING:
    case FLOW_RESUMING:
      *pQoSStatus = QOS_ACTIVATING;
      break;

    case FLOW_CONFIGURING:
      *pQoSStatus = QOS_CONFIGURING;
      break;

    case FLOW_ACTIVATED:
      *pQoSStatus = QOS_AVAILABLE;
      break;

    case FLOW_SUSPENDING:
      *pQoSStatus = QOS_SUSPENDING;
      break;

    case FLOW_SUSPENDED:
      *pQoSStatus = QOS_DEACTIVATED;
      break;

    case FLOW_GOING_NULL:
      *pQoSStatus = QOS_RELEASING;
      break;

    case FLOW_NULL:
      *pQoSStatus = QOS_UNAVAILABLE;
      break;

    default:
      return QDS_EINVAL;
  }

  return AEE_SUCCESS;
} /* GetQoSStatus */

void NetPlatform::GetLastNetworkDownReason
(
  int32                     ifaceHandle,
  NetworkDownReasonType *   pNetDownReason
)
{
  ps_iface_type*  ifacePtr = (ps_iface_type *)ifaceHandle;;

  if (NULL == pNetDownReason)
  {
    ASSERT (0);
    return;
  }

  if (NULL == ifacePtr)
  {
    *pNetDownReason = PS_NET_DOWN_REASON_NOT_SPECIFIED;
    return;
  }

  *pNetDownReason = ifacePtr->event_info_cache.iface_down_info.netdown_reason;

} /* GetLastNetworkDownReason() */

IfaceStateEnumType NetPlatform::GetIfaceState
(
  int32 ifaceHandle
)
{
  return ps_iface_state((ps_iface_type *)ifaceHandle);
}


PSFlowStateType NetPlatform::GetPSFlowState
(
  int32  psFlowHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return PS_FLOW_GET_STATE(reinterpret_cast <ps_flow_type *> (psFlowHandle));
} /* NetPlatform::GetPSFlowState() */


ps_iface_type * NetPlatform::PSIfaceGetHandle
(
  int32  ifaceId
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return ps_iface_get_handle( (ps_iface_id_type) ifaceId );
} /* NetPlatform::PSIfaceGetHandle() */


bool NetPlatform::IsPSFlowDelaySensitive
(
  int32  psFlowHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*lint -e{506} */
  if (PS_FLOW_GET_CAPABILITY( (ps_flow_type *) psFlowHandle,
                              PS_FLOW_CAPABILITY_DELAY_SENSITIVE))
  {
    return true;
  }

  return false;
} /* NetPlatform::IsPSFlowDelaySensitive() */


ds::ErrorType NetPlatform::GetRouteScopeByPolicy
(
  IPolicy *              netPolicyPtr,
  ps_route_scope_type *  routeScopePtr
)
{
  ds::ErrorType      dsErrno;
  ACLPolicyInfoType  aclPolicy;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == routeScopePtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  dsErrno = GenerateAclPolicy( netPolicyPtr, &aclPolicy);
  if (AEE_SUCCESS != dsErrno)
  {
    LOG_MSG_ERROR( "GenerateAclPolicy failed, err 0x%x", dsErrno, 0, 0);
    return dsErrno;
  }

  aclPolicy.proc_id = PS_IFACE_PROC_ID_ANY;

  *routeScopePtr = ps_route_scope_by_policy( 0, &aclPolicy);

  /* Add V4 and V6 routescope always */
  (void) AddLoopbackToRouteScope (TRUE, routeScopePtr);

  PS_ROUTE_SCOPE_PRINT( routeScopePtr);
  return AEE_SUCCESS;

} /* NetPlatform::GetRouteScopeByPolicy() */


ds::ErrorType NetPlatform::GetRouteScopeByIfaceHandle
(
  int32                  ifaceHandle,
  bool                   addV6LoopbackIface,
  ps_route_scope_type *  routeScopePtr
)
{
  ps_iface_type *  psIfacePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == routeScopePtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (0 == ifaceHandle)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EINVAL;
  }

  /*-------------------------------------------------------------------------
    Fetch ps iface corresponding to ifaceHandle and update route scope
  -------------------------------------------------------------------------*/
  psIfacePtr = reinterpret_cast <ps_iface_type *> ( ifaceHandle);
  if (FALSE == ps_route_scope_by_iface_handle( psIfacePtr, routeScopePtr))
  {
    LOG_MSG_ERROR( "ps_route_scope_by_iface_handle failed", 0, 0, 0);
  }

  /* Add V4 and V6 routescope always */
  (void) AddLoopbackToRouteScope( TRUE, routeScopePtr);

  PS_ROUTE_SCOPE_PRINT( routeScopePtr);
  return AEE_SUCCESS;

} /* NetPlatform::GetRouteScopeByIfaceHandle() */


ds::ErrorType NetPlatform::GetRouteScopeByIfaceId
(
  int32                  ifaceId,
  bool                   addV6LoopbackIface,
  ps_route_scope_type *  routeScopePtr
)
{
  ps_iface_type *  psIfacePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == routeScopePtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Fetch ps iface corresponding to ifaceId and update route scope
  -------------------------------------------------------------------------*/
  psIfacePtr = ps_iface_get_handle( (ps_iface_id_type) ifaceId );

  if (NULL == psIfacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (FALSE == ps_route_scope_by_iface_handle( psIfacePtr, routeScopePtr))
  {
    LOG_MSG_ERROR( "ps_route_scope_by_iface_handle failed", 0, 0, 0);
  }

  /* Add V4 and V6 routescope always */
  (void) AddLoopbackToRouteScope( TRUE, routeScopePtr);

  PS_ROUTE_SCOPE_PRINT( routeScopePtr);
  return AEE_SUCCESS;

} /* NetPlatform::GetRouteScopeByIfaceId() */


ds::ErrorType NetPlatform::AddLoopbackToRouteScope
(
  bool                   addV6LoopbackIface,
  ps_route_scope_type *  routeScopePtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == routeScopePtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (addV6LoopbackIface)
  {
    ps_route_scope_add_lo( routeScopePtr, TRUE);
  }
  else
  {
    ps_route_scope_add_lo( routeScopePtr, FALSE);
  }

  PS_ROUTE_SCOPE_PRINT( routeScopePtr);
  return AEE_SUCCESS;

} /* NetPlatform::AddLoopbackToRouteScope() */


ds::ErrorType NetPlatform::DeleteIfaceHandleFromRouteScope
(
  ps_route_scope_type *  routeScopePtr,
  int32                  ifaceHandle
)
{
  ps_iface_type *  psIfacePtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == routeScopePtr)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  psIfacePtr = reinterpret_cast <ps_iface_type *> ( ifaceHandle);

  if (NULL == psIfacePtr)
  {
    LOG_MSG_ERROR ("NULL iface handle", 0, 0, 0);
    return QDS_EINVAL;
  }

  ps_route_scope_delete( routeScopePtr, PS_IFACE_GET_IFACE_INDEX( psIfacePtr));

  PS_ROUTE_SCOPE_PRINT( routeScopePtr);
  return AEE_SUCCESS;

} /* NetPlatform::DeleteIfaceHandleFromRouteScope() */

boolean NetPlatform::RouteScopeIntersect
(
  ps_route_scope_type *  pRouteScope1,
  ps_route_scope_type *  pRouteScope2
)
{
  ps_route_scope_type    tempRouteScope;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pRouteScope1 || NULL == pRouteScope2)
  {
    return TRUE;
  }

  tempRouteScope = ps_route_scope_intersect(*pRouteScope1, *pRouteScope2);

  ps_route_scope_delete (&tempRouteScope, ps_get_v6_lo_rscope_idx());
  ps_route_scope_delete (&tempRouteScope, ps_get_v4_lo_rscope_idx());

  return !(ps_is_route_scope_empty (tempRouteScope));

} /* NetPlatform::RouteScopeIntersect() */

int32 NetPlatform::GetIfaceStats
(
  int32                          ifaceHandle,
  ds::Net::NetworkStatsType      *stats
)
{
  ds::ErrorType           result  = AEE_SUCCESS;
  ps_stat_iface_i_s_type  psStats;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (0 == stats)
  {
    LOG_MSG_ERROR( "NULL arg", 0, 0, 0);
    result = QDS_EFAULT;
    goto bail;
  }

  memset(stats, 0, sizeof(stats));
  memset(&psStats, 0, sizeof(psStats));

  if(E_SUCCESS != ps_stat_get_iface(
                                     PS_STAT_IFACE_ALL,
                                     (void*)ifaceHandle,
                                     (void*)&psStats,
                                     sizeof(ps_stat_iface_i_s_type)
                                   )
    )
  {
    result = AEE_EFAILED;
    goto bail;
  }
  /*-----------------------------------------------------------------------
    convert to output parameter
  -----------------------------------------------------------------------*/

  stats->bytesRX = psStats.bytes_rx;
  stats->bytesTX = psStats.bytes_tx;
  stats->totalBytesTransferredSincePowerUp = psStats.total_bytes_transferred_since_power_up;
  stats->bytesTransferred = psStats.bytes_transferred;
  stats->mcastPktsRX = psStats.mcast_pkts_rx;
  stats->mcastPktsTX = psStats.mcast_pkts_tx;
  stats->pktsDroppedRX = psStats.pkts_dropped_rx;
  stats->pktsDroppedTX = psStats.pkts_dropped_tx;
  stats->pktsRX = psStats.pkts_rx;
  stats->pktsTX = psStats.pkts_tx;
  stats->totalActiveTimeSincePowerUp = psStats.total_active_time_since_power_up;
  stats->totalOpenTimeSincePowerUp = psStats.total_open_time_since_power_up;
  stats->firstIOTimeStamp = psStats.first_io_timestamp;
  stats->lastUpStateTimeStamp = psStats.last_up_state_timestamp;
  stats->state = psStats.state;

  result = AEE_SUCCESS;

/* fall through */

bail:

  return result;

} /* NetPlatform::GetIfaceStats() */

int32 NetPlatform::ResetIfaceStats
(
  int32                          ifaceHandle
)
{
  ds::ErrorType result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void) ps_stat_reset_iface_stats_by_iface_ptr((void*)ifaceHandle);

  return result;

} /* NetPlatform::GetIfaceStats() */

void * NetPlatform::EnableNat
(
  IPolicy       *pIPolicy,
  ds::ErrorType *errnoPtr
)
{
#ifdef FEATURE_DATA_PS_NAT_IFACE
  NetPlatform::ACLPolicyInfoType  aclPolicy;
  int16                           psErrno;
  int32                           result;
  void                            *handle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  result = GenerateAclPolicy (pIPolicy, &aclPolicy);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x converting IPolicy to ACL policy", result, 0, 0);
    return NULL;
  }

  handle = ps_nat_iface_enable_nat(&aclPolicy, &psErrno);

  if(NULL == handle)
  {
    *errnoPtr = MapErrorCode (psErrno);
    return NULL;
  }

  LOG_MSG_INFO1 ("NAT enabled successfully on handle 0x%x", handle, 0, 0);
  return handle;
#else
  *errnoPtr = QDS_EOPNOTSUPP;
  return NULL;
#endif /* FEATURE_DATA_PS_NAT_IFACE */
}  /* NetPlatform::EnableNat() */

ds::ErrorType NetPlatform::DisableNat
(
  void   *natHandle
)
{
#ifdef FEATURE_DATA_PS_NAT_IFACE
  int16   psErrno;
  int32   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* TODO: This is pending a change in ps_nat_iface API, there is no need to pass
   * pointer to natHandle */
  result = ps_nat_iface_disable_nat(natHandle, &psErrno);

  if (DSS_ERROR == result)
  {
    LOG_MSG_ERROR ("Err %d disabling NAT on handle 0x%x",
                    psErrno, natHandle, 0);
    result = MapErrorCode (psErrno);
    return result;
  }

  LOG_MSG_INFO1 ("NAT disabled successfully on handle 0x%x", natHandle, 0, 0);
  return AEE_SUCCESS;
#else
  return QDS_EOPNOTSUPP;
#endif /* FEATURE_DATA_PS_NAT_IFACE */

} /* NetPlatform::DisableNat() */

ds::ErrorType NetPlatform::EnableRoamingAutoconnect
(
  void
)
{
#ifdef FEATURE_DATA_PS_NAT_IFACE
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) ps_roaming_enable_autoconnect_post_cmd();
  return AEE_SUCCESS;

#else
  return QDS_EOPNOTSUPP;
#endif /* FEATURE_DATA_PS_NAT_IFACE */

} /* NetPlatform::EnableRoamingAutoconnect() */

ds::ErrorType NetPlatform::DisableRoamingAutoconnect
(
  void
)
{
#ifdef FEATURE_DATA_PS_NAT_IFACE
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void) ps_roaming_disable_autoconnect_post_cmd();
  return AEE_SUCCESS;

#else
  return QDS_EOPNOTSUPP;
#endif /* FEATURE_DATA_PS_NAT_IFACE */

} /* NetPlatform::DisableRoamingAutoconnect */

#endif /* FEATURE_DATA_PS */

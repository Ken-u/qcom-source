/*=========================================================================*/
/*!
  @file
  ds_Net_Conversion.cpp

  @brief
  This file provides misc conversion routines commonly used in the 
  DSNET layer.

  Copyright (c) 2009 - 2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_Conversion.cpp#2 $
  $DateTime: 2011/06/30 17:39:13 $$Author: brijeshd $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-01-02 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ps_ip_addr.h"
#include "ps_system_heap.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_QoSFlowSpec.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_INetworkExt.h"
#include "ds_Net_INetwork1x.h"
#include "ds_Net_INetworkUMTS.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
#include "ds_Net_INetworkIPv6.h"
#include "ds_Net_INetworkControl.h"
#include "ds_Net_QoS_Def.h"
#include "ds_Net_IQoSDefault.h"
#include "ds_Net_IQoSSecondary.h"
#include "ds_Net_IQoSManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_IIPv6Address.h"
#include "ds_Net_IIPFilterRegPriv.h"
#include "ds_Net_ITechUMTS.h"
#include "ds_Net_INetwork1xPriv.h"
#include "ds_Addr_Def_Helpers.h"
#include "ds_Sock_AddrUtils.h"
#include "ds_Net_TechUMTSFactory.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;


namespace ds
{
namespace Net
{
namespace Conversion
{


ds::ErrorType PS2DSIfaceState 
(
  NetPlatform::IfaceStateEnumType         ifaceState,
  ds::Net::NetworkStateType*              networkState 
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4 ("PS iface state %d", ifaceState, 0, 0);
  
  switch (ifaceState)
  {
    case IFACE_DOWN:
    case IFACE_DISABLED:
      *networkState = NetworkState::QDS_CLOSED;
      break;
    case IFACE_UP:
    case IFACE_ROUTEABLE:
      *networkState = NetworkState::QDS_OPEN;
      break;
    case IFACE_COMING_UP:
    case IFACE_CONFIGURING:
      *networkState = NetworkState::QDS_OPEN_IN_PROGRESS;
      break;
    case IFACE_GOING_DOWN:
      *networkState = NetworkState::QDS_CLOSE_IN_PROGRESS;
      break;
    case IFACE_LINGERING:
      *networkState = NetworkState::QDS_LINGERING;
      break;
    default:
       return QDS_INTERNAL;
  } /* switch */

  return AEE_SUCCESS;
  
} /* PS2DSIfaceState() */


ds::Net::PhysLinkStateType PS2DSPhysLinkState
(
  NetPlatform::PSPhysLinkStateType   psPhysLinkState
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4 ("PS iface state %d", psPhysLinkState, 0, 0);

  switch (psPhysLinkState)
  {
    case PHYS_LINK_DOWN:
    case PHYS_LINK_NULL:
      return PhysLinkState::QDS_DOWN;

    case PHYS_LINK_COMING_UP:
    case PHYS_LINK_RESUMING:
      return PhysLinkState::QDS_RESUMING;

    case PHYS_LINK_UP:
      return PhysLinkState::QDS_UP;

    case PHYS_LINK_GOING_DOWN:
    case PHYS_LINK_GOING_NULL:
      return PhysLinkState::QDS_GOING_DOWN;

    default:
      ASSERT (0);
      return PhysLinkState::QDS_DOWN;
  } /* switch */
  
} /* PS2DSPhysLinkState() */


ds::ErrorType PS2DSAddrFamily
(
 NetPlatform::PSIPAddrFamilyType ipFamily, 
 ds::AddrFamilyType*  dsFamily
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO4 ("PS Iface addr family %d", ipFamily, 0, 0);

  switch (ipFamily)
  {
    // Internal value, should never be returned to upper layers
    case IP_ANY_ADDR:
      ASSERT (0);
      return QDS_EFAULT;
    case IPV4_ADDR:
      *dsFamily = ds::AddrFamily::QDS_AF_INET;
      break;
    case IPV6_ADDR:
      *dsFamily = ds::AddrFamily::QDS_AF_INET6;
      break;
    case IFACE_UNSPEC_ADDR_FAMILY:
      *dsFamily = ds::AddrFamily::QDS_AF_UNSPEC;
      break;
    default:
       return QDS_EINVAL;
  } /*switch */

  return AEE_SUCCESS;

} /* PS2DSAddrFamily() */

ds::ErrorType PS2DSIPAddr
(
  NetPlatform::PSIPAddrType*              pPSIPAddr,
  ds::IPAddrType*                         pDSIPAddr
)
{
   ds::ErrorType nRet;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
        
  if (NULL == pPSIPAddr || NULL == pDSIPAddr)
  {
    return QDS_EFAULT;
  }

  memset (pDSIPAddr, 0, sizeof(ds::IPAddrType));
  nRet = PS2DSAddrFamily (pPSIPAddr->type, &(pDSIPAddr->family));
  if (AEE_SUCCESS != nRet)
  {
     return nRet;
  }

  if (ds::AddrFamily::QDS_AF_INET == pDSIPAddr->family)
  {
    memcpy (pDSIPAddr->addr, &pPSIPAddr->addr.v4, sizeof(struct ps_in_addr));
    return AEE_SUCCESS;
  }
  
  if (ds::AddrFamily::QDS_AF_INET6 == pDSIPAddr->family)
  {
    memcpy (pDSIPAddr->addr, &pPSIPAddr->addr.v6, sizeof(struct ps_in6_addr));
    return AEE_SUCCESS;
  }

  return QDS_EINVAL;

} /* PS2DSIPAddr() */

ds::ErrorType PS2DSIPAddr
(
  ip_addr_type*                           pPSIPAddr,
  ds::IPAddrType*                         pDSIPAddr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return PS2DSIPAddr ((PSIPAddrType*)pPSIPAddr, pDSIPAddr);
} /* PS2DSIPAddr() */

ds::ErrorType PS2DSIPAddr
(
  NetPlatform::PSIPAddrType*              pPSIPAddr,
  ds::INAddr6Type                         in6Addr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (IPV6_ADDR == pPSIPAddr->type)
  {
    memcpy (in6Addr, &pPSIPAddr->addr.v6, sizeof(ds::INAddr6Type));
    return AEE_SUCCESS;
  }

  return QDS_EINVAL;

} /* PS2DSIPAddr() */

void PS2DSIPStaticNatEntry
(
  const NetPlatform::PSStaticNatEntryType*     pPSIPNatEntry,
  ds::Net::IPNatStaticEntryType*               pDSNetIPNatEntry
)
{
  if (NULL == pPSIPNatEntry || NULL == pDSNetIPNatEntry)
  {
    ASSERT (0);
  }

  pDSNetIPNatEntry->protocol = pPSIPNatEntry->protocol;

  memcpy(&pDSNetIPNatEntry->private_ip_addr,
         &pPSIPNatEntry->private_ip_addr, 4);

  memcpy(&pDSNetIPNatEntry->private_port, &pPSIPNatEntry->private_port,
         2);

  memcpy(&pDSNetIPNatEntry->global_port, &pPSIPNatEntry->global_port,
         2);

  return;
}

void DS2PSIPStaticNatEntry
(
  const ds::Net::IPNatStaticEntryType*   pDSNetIPNatEntry,
  NetPlatform::PSStaticNatEntryType*     pPSIPNatEntry
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pPSIPNatEntry || NULL == pDSNetIPNatEntry)
  {
    ASSERT (0);
  }

  pPSIPNatEntry->protocol = pDSNetIPNatEntry->protocol;

  memcpy (&pPSIPNatEntry->private_ip_addr,
          &pDSNetIPNatEntry->private_ip_addr,
          sizeof(struct ps_in_addr));

  memcpy (&pPSIPNatEntry->private_port,
          &pDSNetIPNatEntry->private_port,
          sizeof (uint16));

  memcpy (&pPSIPNatEntry->global_port,
          &pDSNetIPNatEntry->global_port,
          sizeof(uint16));

  return;
}

void PS2DSDMZEntry
(
  const NetPlatform::PSDMZEntryType*     pPSDMZEntry,
  ds::Net::DMZEntryType*                 pDSNetDMZEntry
)
{
  if (NULL == pPSDMZEntry || NULL == pDSNetDMZEntry)
  {
    ASSERT (0);
    return;
  }

  memcpy(pDSNetDMZEntry->dmz_ip_addr, 
         &pPSDMZEntry->dmz_ip_addr.addr.v4, 
         sizeof(uint32));
  
  return;
}

void DS2PSDMZEntry
(
  const ds::Net::DMZEntryType*   pDSNetDMZEntry,
  NetPlatform::PSDMZEntryType*   pPSDMZEntry
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pPSDMZEntry || NULL == pDSNetDMZEntry)
  {
    ASSERT (0);
  }

  memcpy (&pPSDMZEntry->dmz_ip_addr.addr.v4,
          pDSNetDMZEntry->dmz_ip_addr, 
          sizeof(struct ps_in_addr));
  
  return;
}

ds::ErrorType DS2PSIPFilterSpec
(
  IIPFilterPriv*                          pIDSNetIPFilter,
  NetPlatform::PSFilterSpecType*          pPSFilter
)
{
  IPFilterSpec *                 pDSIPFilterSpec = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (NULL == pIDSNetIPFilter || NULL == pPSFilter)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  pDSIPFilterSpec = reinterpret_cast <IPFilterSpec *> (pIDSNetIPFilter);

  return pDSIPFilterSpec->GetPSIPFilterSpec(pPSFilter);

} /* DS2PSIPFilterSpec() */

ds::ErrorType DS2PSQoSFlowSpec
(
  IQoSFlowPriv*                      pIDSNetQoSFlowSpec,
  NetPlatform::PSFlowSpecType*       pPSFlowSpec
)
{

  QoSFlowSpec *                  pDSFlowSpec = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (NULL == pIDSNetQoSFlowSpec || NULL == pPSFlowSpec)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  pDSFlowSpec = reinterpret_cast <QoSFlowSpec *> (pIDSNetQoSFlowSpec);

  return pDSFlowSpec->GetPSQoSFlowSpec(pPSFlowSpec);

} /* DS2PSQoSFlowSpec() */


void PS2DSQoSSpec
(
  NetPlatform::PSQoSSpecType*                 pPSQoSSpec,
  QoSSpecType*                       pDSQoSSpec
)
{
  int                       index;
  IPFilterSpec *   pFilterSpec;
  QoSFlowSpec *    pFlowSpec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == pPSQoSSpec || NULL == pDSQoSSpec)
  {
    ASSERT (0);
    return;
  }

  /*-------------------------------------------------------------------------
    Update RX Flow spec.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSQoSSpec->rxFlowsLen; index++)
  {
    pFlowSpec = 
      reinterpret_cast <QoSFlowSpec *> (pDSQoSSpec->rxFlows[index]);

    if (0 == index)
    {
      (void) pFlowSpec->UpdateSpec (
        &pPSQoSSpec->rx.flow_template.req_flow);
    }
    else if (pDSQoSSpec->rxFlowsLen - 1 == index && 
             TRUE == pDSQoSSpec->rxMinFlow)
    {
      (void) pFlowSpec->UpdateSpec (
        &pPSQoSSpec->rx.flow_template.min_req_flow);
    }
    else
    {
      (void) pFlowSpec->UpdateSpec (
        &pPSQoSSpec->rx.flow_template.aux_flow_list_ptr[index - 1]);
    }
  }

  /*-------------------------------------------------------------------------
    Update TX Flow spec.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSQoSSpec->txFlowsLen; index++)
  {
    pFlowSpec = 
      reinterpret_cast <QoSFlowSpec *> (pDSQoSSpec->txFlows[index]);

    if (0 == index)
    {
      (void) pFlowSpec->UpdateSpec (
        &pPSQoSSpec->tx.flow_template.req_flow);
    }
    else if (pDSQoSSpec->txFlowsLen - 1 == index && 
             TRUE == pDSQoSSpec->txMinFlow)
    {
      (void) pFlowSpec->UpdateSpec (
        &pPSQoSSpec->tx.flow_template.min_req_flow);
    }
    else
    {
      (void) pFlowSpec->UpdateSpec(
        &pPSQoSSpec->tx.flow_template.aux_flow_list_ptr[index - 1]);
    }
  }

  /*-------------------------------------------------------------------------
    Update RX Filter spec.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSQoSSpec->rxFilterLen; index++)
  {
    pFilterSpec = 
      reinterpret_cast <IPFilterSpec *> (pDSQoSSpec->rxFilter[index]);

    (void) pFilterSpec->UpdateSpec (
      &pPSQoSSpec->rx.fltr_template.list_ptr[index]);
  }

  /*-------------------------------------------------------------------------
    Update TX Filter spec.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSQoSSpec->txFilterLen; index++)
  {
    pFilterSpec = 
      reinterpret_cast <IPFilterSpec *> (pDSQoSSpec->txFilter[index]);
    (void) pFilterSpec->UpdateSpec (
      &pPSQoSSpec->tx.fltr_template.list_ptr[index]);
  }

} /* PS2DSQoSSpec() */

void PS2DSIPFilterSpec
(
  NetPlatform::PSFilterSpecType*     pPSIPFilterSpec,
  IPFilterSpec*                      pIPFilterSpec
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == pPSIPFilterSpec || NULL == pIPFilterSpec)
  {
    ASSERT (0);
    return;
  }

  /*-------------------------------------------------------------------------
    Update Filter spec.
  -------------------------------------------------------------------------*/

  (void) pIPFilterSpec->UpdateSpec (pPSIPFilterSpec);

} /* PS2DSIPFilterSpec() */


ds::ErrorType DS2PSQoSRequestSpec
(
  const QoSSpecType*             pDSNetQoSSpec, 
  NetPlatform::PSQoSSpecType*             pPSQoSSpec
)
{
  int               result;
  int               index;
  int               tx_num_aux_flows;
  int               rx_num_aux_flows;
  IQoSFlowPriv*     pIDSNetQoSFlow = NULL;
  IIPFilterPriv*    pIDSNetIPFilterPriv = NULL;

  PSFilterSpecType* pPSFilterSpec;
  PSFlowSpecType*   pPSFlowSpec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == pDSNetQoSSpec || NULL == pPSQoSSpec)
  {
    ASSERT (0);
    result = QDS_EFAULT;
    goto bail;
  }

  memset (pPSQoSSpec, 0, sizeof(NetPlatform::PSQoSSpecType));

  /*-------------------------------------------------------------------------
    Allocate memories required for holding TX/RX filter specs and
    TX/RX auxillary flow specs.
  -------------------------------------------------------------------------*/
  
  /* Update num_filters in any case 
     i.e if the rxFilterLen is 0 (we want to delete all the filters) */
  pPSQoSSpec->rx.fltr_template.num_filters = 
     (uint8) pDSNetQoSSpec->rxFilterLen;

  if (pDSNetQoSSpec->rxFilterLen > 0)
  {
    pPSQoSSpec->rx.fltr_template.list_ptr = 
      (ip_filter_type *)
      ps_system_heap_mem_alloc 
      ( 
        sizeof (PSFilterSpecType) * pDSNetQoSSpec->rxFilterLen
      );

    if (NULL == pPSQoSSpec->rx.fltr_template.list_ptr)
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  pPSQoSSpec->tx.fltr_template.num_filters = 
     (uint8) pDSNetQoSSpec->txFilterLen;

  if (pDSNetQoSSpec->txFilterLen > 0)
  {
    pPSQoSSpec->tx.fltr_template.list_ptr = 
      (ip_filter_type *)
      ps_system_heap_mem_alloc 
      ( 
        sizeof (PSFilterSpecType) * pDSNetQoSSpec->txFilterLen
      );

    if (NULL == pPSQoSSpec->tx.fltr_template.list_ptr)
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  rx_num_aux_flows = (pDSNetQoSSpec->rxMinFlow) ? pDSNetQoSSpec->rxFlowsLen - 2 : pDSNetQoSSpec->rxFlowsLen - 1;
  
  if (rx_num_aux_flows > 0)
  {
    pPSQoSSpec->rx.flow_template.num_aux_flows = rx_num_aux_flows;
      

    pPSQoSSpec->rx.flow_template.aux_flow_list_ptr = 
      (ip_flow_type *) 
      ps_system_heap_mem_alloc 
      ( 
        sizeof (PSFlowSpecType) * pPSQoSSpec->rx.flow_template.num_aux_flows
      );

    if (NULL == pPSQoSSpec->rx.flow_template.aux_flow_list_ptr)
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  tx_num_aux_flows = (pDSNetQoSSpec->txMinFlow) ? pDSNetQoSSpec->txFlowsLen - 2 : pDSNetQoSSpec->txFlowsLen - 1;
  
  if (tx_num_aux_flows > 0)
  {
    pPSQoSSpec->tx.flow_template.num_aux_flows = tx_num_aux_flows;

    pPSQoSSpec->tx.flow_template.aux_flow_list_ptr = 
      (ip_flow_type *) 
      ps_system_heap_mem_alloc 
      ( 
        sizeof (PSFlowSpecType) * pPSQoSSpec->tx.flow_template.num_aux_flows
      );

    if (NULL == pPSQoSSpec->tx.flow_template.aux_flow_list_ptr)
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Start the conversion to PS layout with a field mask of 0.
  -------------------------------------------------------------------------*/
  pPSQoSSpec->field_mask = 0;

  /*-------------------------------------------------------------------------
    Fill in the RX filter spec parameters.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSNetQoSSpec->rxFilterLen; index++)
  {
    pIDSNetIPFilterPriv = pDSNetQoSSpec->rxFilter[index];
    pPSFilterSpec = &(pPSQoSSpec->rx.fltr_template.list_ptr[index]);
    (void) DS2PSIPFilterSpec(pIDSNetIPFilterPriv, pPSFilterSpec);
  }

  /*-------------------------------------------------------------------------
    Fill in the TX filter spec parameters.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSNetQoSSpec->txFilterLen; index++)
  {
    pIDSNetIPFilterPriv = pDSNetQoSSpec->txFilter[index];
    pPSFilterSpec = &(pPSQoSSpec->tx.fltr_template.list_ptr[index]);
    (void) DS2PSIPFilterSpec(pIDSNetIPFilterPriv, pPSFilterSpec);
  }

  /*-------------------------------------------------------------------------
    RX flow parameters.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSNetQoSSpec->rxFlowsLen; index++)
  {
    pIDSNetQoSFlow = pDSNetQoSSpec->rxFlows[index];

    if (0 == index)
    {
      /*---------------------------------------------------------------------
        Fill in the req_flow paramters.
      ---------------------------------------------------------------------*/
      pPSQoSSpec->field_mask |= (uint32) QOS_MASK_RX_FLOW;
      pPSFlowSpec = &(pPSQoSSpec->rx.flow_template.req_flow);
      (void) DS2PSQoSFlowSpec(pIDSNetQoSFlow, pPSFlowSpec);
    }
    else if (TRUE == pDSNetQoSSpec->rxMinFlow && 
             index == pDSNetQoSSpec->rxFlowsLen - 1)
    {
      /*---------------------------------------------------------------------
        Fill in the min_req_flow paramters.
        Min flow is the last in the flows array.
      ---------------------------------------------------------------------*/
      pPSQoSSpec->field_mask |= (uint32) QOS_MASK_RX_MIN_FLOW;
      pPSFlowSpec = &(pPSQoSSpec->rx.flow_template.min_req_flow);
      (void) DS2PSQoSFlowSpec(pIDSNetQoSFlow, pPSFlowSpec);
    }
    else if (rx_num_aux_flows > 0)
    {
      /*---------------------------------------------------------------------
        Fill in the aux_flow paramters.
      ---------------------------------------------------------------------*/
      pPSQoSSpec->field_mask |= (uint32) QOS_MASK_RX_AUXILIARY_FLOWS;
      pPSFlowSpec = &(pPSQoSSpec->rx.flow_template.aux_flow_list_ptr[index-1]);
      (void) DS2PSQoSFlowSpec(pIDSNetQoSFlow, pPSFlowSpec);
    }
  }

  /*-------------------------------------------------------------------------
    TX flow parameters.
  -------------------------------------------------------------------------*/
  for (index = 0; index < pDSNetQoSSpec->txFlowsLen; index++)
  {
    pIDSNetQoSFlow = pDSNetQoSSpec->txFlows[index];

    if (0 == index)
    {
      /*---------------------------------------------------------------------
        Fill in the req_flow paramters.
      ---------------------------------------------------------------------*/
      pPSQoSSpec->field_mask |= (uint32) QOS_MASK_TX_FLOW;
      pPSFlowSpec = &(pPSQoSSpec->tx.flow_template.req_flow);
      (void) DS2PSQoSFlowSpec(pIDSNetQoSFlow, pPSFlowSpec);
    }
    else if (TRUE == pDSNetQoSSpec->txMinFlow && 
             index == pDSNetQoSSpec->txFlowsLen - 1)
    {
      /*-----------------------------------------------------------------------
        Fill in the min_req_flow paramters.
        Min flow is the last in the flows array.
      -----------------------------------------------------------------------*/
      pPSQoSSpec->field_mask |= (uint32) QOS_MASK_TX_MIN_FLOW;
      pPSFlowSpec = &(pPSQoSSpec->tx.flow_template.min_req_flow);
      (void) DS2PSQoSFlowSpec(pIDSNetQoSFlow, pPSFlowSpec);
    }
    else if (tx_num_aux_flows > 0)
    {
      /*---------------------------------------------------------------------
        Fill in the aux_flow paramters.
      ---------------------------------------------------------------------*/
      pPSQoSSpec->field_mask |= (uint32) QOS_MASK_TX_AUXILIARY_FLOWS;
      pPSFlowSpec = &(pPSQoSSpec->tx.flow_template.aux_flow_list_ptr[index-1]);
      (void) DS2PSQoSFlowSpec(pIDSNetQoSFlow, pPSFlowSpec);
    }
  }
  
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d constr PS QoS spec", result, 0, 0);
  CleanupPSQoSRequestSpec (pPSQoSSpec);
  return result;

} /* DS2PSQoSRequestSpec() */


ds::ErrorType DS2PSQoSModifySpec
(
  IQoSFlowPriv**                          ppRxFlows, 
  int                                     rxFlowsLen, 
  IQoSFlowPriv**                          ppTxFlows, 
  int                                     txFlowsLen, 
  QoSModifyMaskType                       fieldMask,
  NetPlatform::PrimaryQoSModifySpecType * pPSQoSModifySpec
)
{
  NetPlatform::PSQoSSpecType              psQoSRequestSpec;
  QoSSpecType                    dsQoSRequestSpec;
  int                                     result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == pPSQoSModifySpec)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (((0 != rxFlowsLen) && (NULL == ppRxFlows)) ||
      ((0 != txFlowsLen) && (NULL == ppTxFlows)))
  {
    LOG_MSG_ERROR ("Inv. params specified.", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Currently we are not looking at the fieldMask completely. The 
    converstion happens based on the number of number of flows etc.
  -------------------------------------------------------------------------*/
  (void) fieldMask;

  /*-------------------------------------------------------------------------
    We are trying to reuse the existing code to perform the conversion.
    We first construct a QoS Request spec in PS layer format.
  -------------------------------------------------------------------------*/
  memset (&dsQoSRequestSpec, 0, sizeof (dsQoSRequestSpec));
  dsQoSRequestSpec.rxFlows    =  ppRxFlows;
  dsQoSRequestSpec.rxFlowsLen =  rxFlowsLen;
  dsQoSRequestSpec.txFlows    =  ppTxFlows;
  dsQoSRequestSpec.txFlowsLen =  txFlowsLen;

  memset (&psQoSRequestSpec, 0, sizeof (psQoSRequestSpec));

  result = DS2PSQoSRequestSpec (&dsQoSRequestSpec,
                                &psQoSRequestSpec);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    memcpy the conversion results into PS QoS Modify spec.
  -------------------------------------------------------------------------*/
  pPSQoSModifySpec->field_mask = psQoSRequestSpec.field_mask;
  (void) memcpy (&pPSQoSModifySpec->rx_flow_template, 
                 &psQoSRequestSpec.rx.flow_template,
                 sizeof (ip_flow_spec_type));

  (void) memcpy (&pPSQoSModifySpec->tx_flow_template, 
                 &psQoSRequestSpec.tx.flow_template,
                 sizeof (ip_flow_spec_type));

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("QoS Modify conversion failed, err %d", result, 0, 0);
  CleanupPSQoSRequestSpec (&psQoSRequestSpec);
  return result;

} /* DS2PSQoSModifySpec() */

void CleanupPSQoSRequestSpec
(
  NetPlatform::PSQoSSpecType*               pPSQoSSpec
)
{
  
  if (NULL == pPSQoSSpec)
  {
    return;
  }

  if( pPSQoSSpec->rx.fltr_template.num_filters > 0)
  {
    PS_SYSTEM_HEAP_MEM_FREE (pPSQoSSpec->rx.fltr_template.list_ptr);
  }
  
  if( pPSQoSSpec->tx.fltr_template.num_filters > 0)
  {
    PS_SYSTEM_HEAP_MEM_FREE (pPSQoSSpec->tx.fltr_template.list_ptr);
  }

  if ((uint32) QOS_MASK_RX_AUXILIARY_FLOWS & pPSQoSSpec->field_mask)
  {
    PS_SYSTEM_HEAP_MEM_FREE (pPSQoSSpec->rx.flow_template.aux_flow_list_ptr);
  }

  if ((uint32) QOS_MASK_TX_AUXILIARY_FLOWS & pPSQoSSpec->field_mask)
  {
    PS_SYSTEM_HEAP_MEM_FREE (pPSQoSSpec->tx.flow_template.aux_flow_list_ptr);
  }
  
} /* CleanupPSQoSRequestSpec() */

void CleanupPSQoSModifySpec
(
  NetPlatform::PrimaryQoSModifySpecType *   pPSQoSSpec
)
{
  if (NULL == pPSQoSSpec)
  {
    return;
  }

  if ((uint32) QOS_MASK_RX_AUXILIARY_FLOWS & pPSQoSSpec->field_mask)
  {
    PS_SYSTEM_HEAP_MEM_FREE (pPSQoSSpec->rx_flow_template.aux_flow_list_ptr);
  }

  if ((uint32) QOS_MASK_TX_AUXILIARY_FLOWS & pPSQoSSpec->field_mask)
  {
    PS_SYSTEM_HEAP_MEM_FREE (pPSQoSSpec->tx_flow_template.aux_flow_list_ptr);
  }
  
} /* CleanupPSQoSRequestSpec() */


ds::ErrorType DS2PSMCastJoinSpec
(
  const SockAddrStorageType      pSockAddr, 
  NetPlatform::MCastJoinType*    pPSMCastJoinSpec
)
{
  SockAddrINType* v4SockAddr;
  SockAddrIN6Type* v6SockAddr;

  ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
  
  if (NULL == pSockAddr || NULL == pPSMCastJoinSpec)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  (void)ds::Sock::AddrUtils::GetFamily(pSockAddr,&family);

  family = ps_ntohs(family);
  
  if (ds::AddrFamily::QDS_AF_INET == family)
  {
    v4SockAddr = (SockAddrINType *) pSockAddr;
    pPSMCastJoinSpec->ip_addr.type = IPV4_ADDR;
    pPSMCastJoinSpec->ip_addr.addr.v4 = v4SockAddr->addr;
    pPSMCastJoinSpec->port = v4SockAddr->port;
  }
  else if (ds::AddrFamily::QDS_AF_INET6 == family)
  {
    v6SockAddr = (SockAddrIN6Type *) pSockAddr;
    pPSMCastJoinSpec->ip_addr.type = IPV6_ADDR;
    memcpy ((void *) &pPSMCastJoinSpec->ip_addr.addr, 
            v6SockAddr->addr, 
            sizeof(pPSMCastJoinSpec->ip_addr.addr));
    pPSMCastJoinSpec->port = v6SockAddr->port;
  }
  else
  {
    return QDS_EAFNOSUPPORT;
  }
  
  pPSMCastJoinSpec->mcast_param_ptr = NULL;
  pPSMCastJoinSpec->handle = 0;

  return 0;
} /* DS2PSMCastJoinSpec() */

ds::ErrorType DS2PSMCastJoinExSpec
(
  const SockAddrStorageType *     pSockAddrSeq, 
  int                             addrSeqLen, 
  NetPlatform::MCastJoinExType *  pPSMCastJoinExSpec,
  const MCastJoinFlagsType *      regFlags
)
{
  int index;
  SockAddrINType* v4SockAddr;
  SockAddrIN6Type* v6SockAddr;
  ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;

  if (NULL == pSockAddrSeq || 
      0 >= addrSeqLen || 
      NULL == pPSMCastJoinExSpec)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  pPSMCastJoinExSpec->num_flows = addrSeqLen;
  for (index = 0; index < addrSeqLen; index++)
  {
    (void)ds::Sock::AddrUtils::GetFamily(pSockAddrSeq[index],&family);
    family = ps_ntohs(family);

    if (ds::AddrFamily::QDS_AF_INET == family)
    {
      v4SockAddr = (SockAddrINType *) (pSockAddrSeq[index]);
      pPSMCastJoinExSpec->ip_addr[index].type = IPV4_ADDR;
      pPSMCastJoinExSpec->ip_addr[index].addr.v4 = v4SockAddr->addr;
      pPSMCastJoinExSpec->port[index] = v4SockAddr->port;
    }
    else if (ds::AddrFamily::QDS_AF_INET6 == family)
    {
      v6SockAddr = (SockAddrIN6Type *) (pSockAddrSeq[index]);
      pPSMCastJoinExSpec->ip_addr[index].type = IPV6_ADDR;
      memcpy ((void *) &pPSMCastJoinExSpec->ip_addr[index].addr,
              v6SockAddr->addr, 
              sizeof (pPSMCastJoinExSpec->ip_addr[index].addr));
      pPSMCastJoinExSpec->port[index] = v6SockAddr->port;
    }
    else
    {
      return QDS_EAFNOSUPPORT;
    }

    pPSMCastJoinExSpec->mcast_param_ptr[index] = NULL;
    pPSMCastJoinExSpec->mcast_request_flags[index] = 
      (ps_iface_ioctl_mcast_join_ex_req_flags_enum_type)regFlags[index];
    pPSMCastJoinExSpec->handle[index] = 0;
  } /* for */

  return 0;

} /* DS2PSMCastJoinExSpec() */

ds::ErrorType DS2PSMCastAddrInfo
(
  const ::ds::SockAddrStorageType  addrSeq,
  NetPlatform::MCastAddrInfoType*  pMCastAddrInfo
)
{  
  SockAddrINType* v4SockAddr;
  SockAddrIN6Type* v6SockAddr;

  ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
  
  if (NULL == addrSeq || NULL == pMCastAddrInfo)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  (void)ds::Sock::AddrUtils::GetFamily(addrSeq, &family);
  family = ps_ntohs(family);
  
  if (ds::AddrFamily::QDS_AF_INET == family)
  {
    v4SockAddr = (SockAddrINType *) addrSeq;
    pMCastAddrInfo->ip_addr.type = IPV4_ADDR;
    pMCastAddrInfo->ip_addr.addr.v4 = v4SockAddr->addr;
    pMCastAddrInfo->port = v4SockAddr->port;
  }
  else if (ds::AddrFamily::QDS_AF_INET6 == family)
  {
    v6SockAddr = (SockAddrIN6Type *) addrSeq;
    pMCastAddrInfo->ip_addr.type = IPV6_ADDR;
    memcpy ((void *) &pMCastAddrInfo->ip_addr.addr, 
            v6SockAddr->addr, 
            sizeof(pMCastAddrInfo->ip_addr.addr));
    pMCastAddrInfo->port = v6SockAddr->port;
  }
  else
  {
    return QDS_EAFNOSUPPORT;
  }

  return AEE_SUCCESS;
} /* DS2PSMCastAddrInfo() */

QoSSecondaryStateType PS2DSFlowState
(
  PSFlowStateType                         psFlowState
)
{
  switch (psFlowState)
  {
    case FLOW_ACTIVATING:
    case FLOW_RESUMING:
      return QoSSecondaryState::QDS_ACTIVATING;

    case FLOW_CONFIGURING:
      return QoSSecondaryState::QDS_CONFIGURING;

    case FLOW_ACTIVATED:
      return QoSSecondaryState::QDS_AVAILABLE_MODIFIED;

    case FLOW_SUSPENDING:
      return QoSSecondaryState::QDS_SUSPENDING;

    case FLOW_SUSPENDED:
      return QoSSecondaryState::QDS_SUSPENDED;

    case FLOW_GOING_NULL:
      return QoSSecondaryState::QDS_RELEASING;

    case FLOW_NULL:
      return QoSSecondaryState::QDS_UNAVAILABLE;

    default:
      return QoSSecondaryState::QDS_UNAVAILABLE;
  }

} /* PS2DSFlowState() */


void PS2DSIPv6Prefix
(
  NetPlatform::IPv6PrefixInfoType *           pPSIPv6Prefix,
  IPv6PrivPrefixInfoType *   pDSIPv6Prefix 
)
{
  if (NULL == pPSIPv6Prefix || NULL == pDSIPv6Prefix)
  {
    return;
  }

  switch (pPSIPv6Prefix->prefix_state)
  {
    case IPV6_ADDR_STATE_INVALID:
      //TODO: This should be prefixState
      pDSIPv6Prefix->prefixType = IPv6AddrState::PRIV_ADDR_DELETED;
      break;
  
    case IPV6_ADDR_STATE_TENTATIVE:
    case IPV6_ADDR_STATE_UNASSIGNED:
      pDSIPv6Prefix->prefixType = IPv6AddrState::PRIV_ADDR_WAITING;
      break;

    case IPV6_ADDR_STATE_VALID:
      pDSIPv6Prefix->prefixType = IPv6AddrState::PRIV_ADDR_AVAILABLE;
      break;

    case IPV6_ADDR_STATE_DEPRECATED:
      pDSIPv6Prefix->prefixType = IPv6AddrState::PRIV_ADDR_DEPRECATED;
      break;

    default:
      ASSERT (0);
      return;
  }

  pDSIPv6Prefix->prefixLen = pPSIPv6Prefix->prefix_len;

  memcpy (pDSIPv6Prefix->prefix, 
          &pPSIPv6Prefix->prefix, 
          sizeof (ds::INAddr6Type));

} /* PS2DSIPv6Prefix() */



ds::ErrorType PS2DSEventInfo
(
  int32                                  psEventName,
  NetPlatform::IfaceEventInfoUnionType  *pPSEventInfo,
  int32                                 *pDSEventName,
  int32                                 *pDSUserHandle,
  EventGroupType                        *pDSEventGroup
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == pDSEventName || 
      NULL == pDSEventGroup  ||
      NULL == pDSUserHandle  ||
      NULL == pPSEventInfo)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  *pDSEventName  = -1;
  *pDSUserHandle = 0;
  *pDSEventGroup = EVENT_GROUP_INVALID;

  switch (psEventName)
  {

    /* Phys link state events */
    case PHYS_LINK_DOWN_EV:
    case PHYS_LINK_COMING_UP_EV:
    case PHYS_LINK_UP_EV:
    case PHYS_LINK_GOING_DOWN_EV:
    case PHYS_LINK_GONE_EV:
    case PHYS_LINK_RESUMING_EV:
    case PHYS_LINK_GOING_NULL_EV:
      *pDSEventName = PhysLinkEvent::QDS_EV_STATE_CHANGED;
      break;

    case PHYS_LINK_707_DOS_ACK_EV:
      /* TODO */
      break;

    case IFACE_UP_EV:
    case IFACE_GOING_DOWN_EV:
    case IFACE_ENABLED_EV:
    case IFACE_DISABLED_EV:
    case IFACE_DOWN_EV:
    case IFACE_COMING_UP_EV:
    case IFACE_CONFIGURING_EV:
    case IFACE_ROUTEABLE_EV:
      *pDSEventName = NetworkEvent::QDS_EV_STATE_CHANGED;
      break;

    case IFACE_ADDR_CHANGED_EV:
    case IFACE_ADDR_FAMILY_CHANGED_EV:
      *pDSEventName = NetworkEvent::QDS_EV_IP_ADDR_CHANGED;
      break;

    case IFACE_MTU_CHANGED_EV:
      //TODO
      break;

    case IFACE_DELETE_EV:
      //TODO
      break;

    case IFACE_IPFLTR_UPDATED_EV:
      *pDSEventName = IPFilterEvent::QDS_EV_STATE_CHANGED;
      break;

    case IFACE_PRI_PHYS_LINK_CHANGED_EV:
      //TODO
      break;

    case IFACE_VALID_RA_EV:
      //TODO
      break;

    case IFACE_MT_REQUEST_EV:
      *pDSEventName = TechUMTSFactory::QDS_EV_MTPD;
      *pDSUserHandle = pPSEventInfo->mt_handle;
      break;

    case IFACE_MCAST_REGISTER_SUCCESS_EV:
    case IFACE_MCAST_REGISTER_FAILURE_EV:
    case IFACE_MCAST_DEREGISTERED_EV:
      *pDSEventName = MCastSessionEvent::QDS_EV_REGISTRATION_STATE;
      *pDSUserHandle = pPSEventInfo->mcast_info.handle;
      break;


    case IFACE_BEARER_TECH_CHANGED_EV:
      *pDSEventName = NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED;
      break;

    case IFACE_QOS_AWARE_SYSTEM_EV:
    case IFACE_QOS_UNAWARE_SYSTEM_EV:
      *pDSEventName = NetworkExtEvent::QDS_EV_QOS_AWARENESS;
      break;

    case IFACE_FLOW_ADDED_EV:
    case IFACE_FLOW_DELETED_EV:
      //TODO
      break;

    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV:
    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV:
      *pDSEventName = Network1xPrivEvent::HDR_REV0_RATE_INERTIA_RESULT;
      break;

    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV:
        *pDSEventName = Network1xPrivEvent::SLOTTED_MODE_CHANGED; 
      break;

    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV:
    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV:
        *pDSEventName = Network1xPrivEvent::SLOTTED_MODE_RESULT;
      break;

    case IFACE_PREFIX_UPDATE_EV:
      *pDSEventName = IPv6PrivEvent::QDS_EV_PREFIX_CHANGED;
      break;

    case IFACE_IPV6_PRIV_ADDR_GENERATED_EV:
    case IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV:
    case IFACE_IPV6_PRIV_ADDR_EXPIRED_EV:
    case IFACE_IPV6_PRIV_ADDR_DELETED_EV:
      *pDSEventName = IPv6AddrEvent::QDS_EV_STATE_CHANGED;
      break;

    case IFACE_OUTAGE_NOTIFICATION_EV:
      *pDSEventName = NetworkEvent::QDS_EV_OUTAGE;
      break;

    case IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV:
      *pDSEventName = QoSMgrEvent::QDS_EV_PROFILES_CHANGED;
      break;

    case IFACE_RF_CONDITIONS_CHANGED_EV:
      *pDSEventName = NetworkEvent::QDS_EV_RF_CONDITIONS_CHANGED;
      break;

    case IFACE_EXTENDED_IP_CONFIG_EV:
      *pDSEventName = NetworkControlEvent::QDS_EV_EXTENDED_IP_CONFIG;
      break;

    case IFACE_IDLE_EV:
      *pDSEventName = NetworkEvent::QDS_EV_IDLE;
      break;

    case IFACE_LINGERING_EV:
      //TODO
      break;

    case IFACE_MBMS_CONTEXT_ACT_SUCCESS_EV:
    case IFACE_MBMS_CONTEXT_ACT_FAILURE_EV:
    case IFACE_MBMS_CONTEXT_DEACT_SUCCESS_EV:
    case IFACE_MBMS_CONTEXT_DEACT_FAILURE_EV:
      *pDSEventName = MBMSEvent::QDS_EV_ACTIVATE_STATE;
      *pDSUserHandle = pPSEventInfo->mcast_info.handle;
      break;

    case IFACE_AUTHENTICATING_EV:
    case IFACE_APP_PREEMPTED_EV:
      //TODO
      break;

    case IFACE_MCAST_STATUS_EV:
      *pDSEventName = MCastSessionEvent::QDS_EV_TECHNOLOGY_STATUS;
	  *pDSUserHandle = pPSEventInfo->mcast_info.handle;
      break;

    case FLOW_NULL_EV:
    case FLOW_ACTIVATING_EV:
    case FLOW_ACTIVATED_EV:
    case FLOW_SUSPENDING_EV:
    case FLOW_SUSPENDED_EV:
    case FLOW_RESUMING_EV:
    case FLOW_GOING_NULL_EV:
    case FLOW_CONFIGURING_EV:
    case FLOW_RX_FLTR_UPDATED_EV:
    case FLOW_INFO_CODE_UPDATED_EV:
      *pDSEventName = QoSSecondaryEvent::QDS_EV_STATE_CHANGED;
      break;

    case FLOW_MODIFY_ACCEPTED_EV:
    case FLOW_MODIFY_REJECTED_EV:
      *pDSEventName = QoSEvent::QDS_EV_MODIFY_RESULT;
      break;

    case FLOW_PRIMARY_MODIFY_RESULT_EV:
      *pDSEventName = QoSDefaultEvent::QDS_EV_MODIFIED;
      break;

    default:
      *pDSEventName = -1;
      break;

  } /* switch (psEventName) */
  
  /*-------------------------------------------------------------------------
    All the ds events are unique numbers (hopefully), so one switch should 
    work fine.
  -------------------------------------------------------------------------*/
  switch(*pDSEventName)
  {
    case  NetworkExtEvent::QDS_EV_BEARER_TECH_CHANGED:
    case  NetworkEvent::QDS_EV_IP_ADDR_CHANGED:
    case  NetworkEvent::QDS_EV_OUTAGE:
    case  NetworkExtEvent::QDS_EV_QOS_AWARENESS:
    case  NetworkEvent::QDS_EV_RF_CONDITIONS_CHANGED:
    case  NetworkEvent::QDS_EV_STATE_CHANGED:
    case  NetworkControlEvent::QDS_EV_EXTENDED_IP_CONFIG:
    case  NetworkEvent::QDS_EV_IDLE:
      *pDSEventGroup = EVENT_GROUP_NETWORK;
      break;

    case IPFilterEvent::QDS_EV_STATE_CHANGED:
      *pDSEventGroup = EVENT_GROUP_IP_FLTR;
      break;

    case  QoSEvent::QDS_EV_INFO_CODE_UPDATED:
    case  QoSEvent::QDS_EV_MODIFY_RESULT:
    case  QoSDefaultEvent::QDS_EV_MODIFIED:
    case  QoSSecondaryEvent::QDS_EV_STATE_CHANGED:
    case  QoSMgrEvent::QDS_EV_PROFILES_CHANGED:
      *pDSEventGroup = EVENT_GROUP_QOS;
      break;

    case  PhysLinkEvent::QDS_EV_STATE_CHANGED:
      *pDSEventGroup = EVENT_GROUP_PHYS_LINK;
      break;

    case  MCastSessionEvent::QDS_EV_REGISTRATION_STATE:
    case  MCastSessionEvent::QDS_EV_TECHNOLOGY_STATUS:
      *pDSEventGroup = EVENT_GROUP_MCAST;
      break;

    case  MBMSEvent::QDS_EV_ACTIVATE_STATE:
      *pDSEventGroup = EVENT_GROUP_NETWORK_MBMS;
      break;

    //TODO: Change this event name in IDL so that this comes under 
    //IPv6::Event::STATE_CHANGED
    //IPv6::Event::PREFIX_CHANGED
    case  IPv6PrivEvent::QDS_EV_PREFIX_CHANGED:
    case  IPv6AddrEvent::QDS_EV_STATE_CHANGED:
      *pDSEventGroup = EVENT_GROUP_NETWORK_IPV6;
      break;

    //TODO: Network1x should be Network1X (capital X for uniformity)
    case  Network1xPrivEvent::HDR_REV0_RATE_INERTIA_RESULT:
    case  Network1xPrivEvent::SLOTTED_MODE_CHANGED:
    case  Network1xPrivEvent::SLOTTED_MODE_RESULT:
      *pDSEventGroup = EVENT_GROUP_NETWORK_1X;
      break;

    case  TechUMTSFactory::QDS_EV_MTPD:
      *pDSEventGroup = EVENT_GROUP_MTPD;
      break;


    default:
      LOG_MSG_INFO1 ("Ignoring event %d", psEventName, 0, 0);
      /* Ignore invalid event */
      return QDS_EINVAL;
  } /* switch (*pDSEventName) */


  return AEE_SUCCESS;

} /* PS2DSEventInfo() */
/*!
@brief
Converts an BCMCS update parameter from ds to PS format.

@param[in]  pIBCMCSUpdParam - Interface that holds BCMCS update parameter.
@param[out] pPSNetBCMCSUpdParam - BCMCS update parameter in PS format.

@see        Interface IBCMCSDBSpec
@see        NetPlatform::PSBCMCSDbUpdateType
@return     QDS_EFAULT - Invalid input.
@return     AEE_SUCCESS - On success.
*/

ds::ErrorType DS2PSBCMCSSpec
(
  const BCMCSDBSpecType               *pIBCMCSUpdParam,
  NetPlatform::PSBCMCSDbUpdateType    *pPSNetBCMCSUpdParam
)
{
  if (NULL == pIBCMCSUpdParam || NULL == pPSNetBCMCSUpdParam)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }

  memset (&pPSNetBCMCSUpdParam->zone, 0, sizeof(dsbcmcs_zone_type));
  memset (&pPSNetBCMCSUpdParam->multicast_ip, 0, sizeof(ip_addr_type));
  
  switch (pIBCMCSUpdParam->zone.type)
  {
    case BCMCSZone::QDS_CDMA1X:
      pPSNetBCMCSUpdParam->zone.type = DSBCMCS_ZONE_1X;
      break;
    case BCMCSZone::QDS_HDR:
      pPSNetBCMCSUpdParam->zone.type = DSBCMCS_ZONE_HDR;
      break;
    default:
      return QDS_EFAULT;
  }
  
  
  (void)memcpy (pPSNetBCMCSUpdParam->zone.zone.subnet,
                pIBCMCSUpdParam->zone.subnet,
                HDRADDR_LENGTH_IN_BYTES);
  
  switch (pIBCMCSUpdParam->multicastAddr.family)
  {
    case ds::AddrFamily::QDS_AF_UNSPEC:
      pPSNetBCMCSUpdParam->multicast_ip.type = IP_ADDR_INVALID;
      break;
    case ds::AddrFamily::QDS_AF_INET:
      pPSNetBCMCSUpdParam->multicast_ip.type = IPV4_ADDR;
      break;
    case ds::AddrFamily::QDS_AF_INET6:
      pPSNetBCMCSUpdParam->multicast_ip.type = IPV6_ADDR;
      break;
    default:
      ASSERT (0);
      pPSNetBCMCSUpdParam->multicast_ip.type = IP_ADDR_INVALID;
      return QDS_EINVAL;
  }

  (void) memcpy (&pPSNetBCMCSUpdParam->multicast_ip.addr,
                 &pIBCMCSUpdParam->multicastAddr.addr,
                 16);
    
  
  pPSNetBCMCSUpdParam->program_id = pIBCMCSUpdParam->programID;
  pPSNetBCMCSUpdParam->program_id_len = pIBCMCSUpdParam->programIDLength;
  pPSNetBCMCSUpdParam->flow_discrim_len = pIBCMCSUpdParam->flowDiscriminatorLength;
  pPSNetBCMCSUpdParam->flow_discrim = pIBCMCSUpdParam->flowDiscriminator;
  pPSNetBCMCSUpdParam->port = pIBCMCSUpdParam->BCMCSPort;
  
  switch(pIBCMCSUpdParam->framing)
  {
    case BCMCSFraming::FRMV_NOT_SET:
      pPSNetBCMCSUpdParam->framing = DSBCMCS_FRAMING_NOT_SET;
      break;
    
    case BCMCSFraming::FRMV_SEGMENT:
      pPSNetBCMCSUpdParam->framing = DSBCMCS_FRAMING_SEGMENT;
      break;

    case BCMCSFraming::FRMV_HDLC:
      pPSNetBCMCSUpdParam->framing = DSBCMCS_FRAMING_HDLC;
      break;
  }
  
  switch(pIBCMCSUpdParam->protocol)
  {
    case BCMCSProtocol::PRTV_NOT_SET:
      pPSNetBCMCSUpdParam->protocol = DSBCMCS_PROTOCOL_NOT_SET;
      break;

    case BCMCSProtocol::PRTV_PPP:
      pPSNetBCMCSUpdParam->protocol = DSBCMCS_PROTOCOL_PPP;
      break;

    case BCMCSProtocol::PRTV_IPV4:
      pPSNetBCMCSUpdParam->protocol = DSBCMCS_PROTOCOL_IPv4;
      break;

    case BCMCSProtocol::PRTV_IPV6:
      pPSNetBCMCSUpdParam->protocol = DSBCMCS_PROTOCOL_IPv6;
      break;
  }
  
  pPSNetBCMCSUpdParam->crc_len = pIBCMCSUpdParam->crcLength;
  pPSNetBCMCSUpdParam->flow_format = pIBCMCSUpdParam->flowFormat;
  pPSNetBCMCSUpdParam->flow_id = pIBCMCSUpdParam->flowID;
  pPSNetBCMCSUpdParam->flow_id_len = pIBCMCSUpdParam->flowIDLen;
  pPSNetBCMCSUpdParam->overwrite = pIBCMCSUpdParam->overwrite;
  pPSNetBCMCSUpdParam->flow_priority = pIBCMCSUpdParam->flowPriority;  
  
  return AEE_SUCCESS;
}

/*!
@brief
Converts Mcast info code from BCMCS 1.0 version to BCMCS 2.0 version.

@param[in]  oldCode - MCast info code in BCMCS 1.0 format.
@param[out]  newCode - MCast info code in BCMCS 2.0 format. 

@see        ps_iface_mcast_info_code_enum_type

@return     AEE_SUCCESS - On success
@return     QDS_EFAULT - invalid parameters.
@return     QDS_EINVAL - no mapping from 1.0 to 2.0 exists.
*/
ds::ErrorType MCastInfoCodeBCMCS1_0To2_0
(
  ps_iface_mcast_info_code_enum_type   oldCode,
  ps_iface_mcast_info_code_enum_type * newCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(0 == newCode)
  {
    ASSERT (0);
    return QDS_EFAULT;
  }
  switch(oldCode)
  {
  case PS_IFACE_MCAST_IC_NOT_SPECIFIED:
    *newCode = PS_IFACE_MCAST_IC_NOT_SPECIFIED;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_CANCELLED:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_CANCELLED;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNABLE_TO_MONITOR:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_UNABLE_TO_MONITOR;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_REQUESTED:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_REQUESTED;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_TIMEOUT:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_TIMEOUT;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_LOST:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_LOST;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_SYS_UNAVAILABLE:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_SYS_UNAVAILABLE;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNAVAILABLE:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_UNAVAILABLE;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_NO_MAPPING:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_NO_MAPPING;
    break;
  case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_ID_NOT_FOUND_FOR_GIVEN_MULTICAST_IP:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_ID_NOT_FOUND_FOR_GIVEN_MULTICAST_IP;
    break;
  case PS_IFACE_MCAST_BCMCS_MAX_FLOWS_REACHED:
    *newCode = PS_IFACE_MCAST_BCMCS2p0_MAX_FLOWS_REACHED;
    break;
  default:
    return QDS_EINVAL;
  }

  return AEE_SUCCESS;
}

/*!
@brief
Checks if provided Mcast info code is related to BCMCS 1.0 version.

@param[in]  infoCode - MCast info code to check.

@see        ps_iface_mcast_info_code_enum_type

@return     TRUE - info code is related to BCMCS 1_0
@return     FALSE - otherwise.
*/
boolean MCastInfoCodeIsBCMCS1_0
(
  ps_iface_mcast_info_code_enum_type   infoCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  switch(infoCode)
  {
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_CANCELLED:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNABLE_TO_MONITOR:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_REQUESTED:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_TIMEOUT:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_LOST:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_SYS_UNAVAILABLE:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNAVAILABLE:
    case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_NO_MAPPING:
    case PS_IFACE_MCAST_BCMCS_MAX_FLOWS_REACHED:
    case PS_IFACE_MCAST_BCMCS_MAX_DEPRECATED_INFO_CODE:
      return TRUE;
    default:
      return FALSE;
  }

}

/*!
@brief
Checks if provided Mcast info code is related to DVBH or MFLO.

@param[in]  infoCode - MCast info code to check.

@see        ps_iface_mcast_info_code_enum_type

@return     TRUE - info code is related to DVBH or MFLO
@return     FALSE - otherwise.
*/
boolean MCastInfoCodeIsDVBHOrMFLO
(
  ps_iface_mcast_info_code_enum_type   infoCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  switch(infoCode)
  {
    case PS_IFACE_MCAST_FLO_IP_OR_PORT_NOT_SUPPORTED:
    case PS_IFACE_MCAST_FLO_NO_AUTHORIZATION:
    case PS_IFACE_MCAST_FLO_NO_SYSTEM_COVERAGE:
    case PS_IFACE_MCAST_FLO_MAX_FLOW_REACHED:
    case PS_IFACE_MCAST_DVBH_IP_OR_PORT_NOT_FOUND:
    case PS_IFACE_MCAST_DVBH_SYSTEM_UNAVAILABLE:
    case PS_IFACE_MCAST_DVBH_BAD_REQUEST:
    case PS_IFACE_MCAST_DVBH_DUP_REQUEST:
    case PS_IFACE_MCAST_DVBH_MAX_FLOWS_REACHED:
      return TRUE;
    default:
      return FALSE;
  }
}

/*!
@brief
Converts socket address family from ds to PS. 

@param[in]  dsFamily - DS layer socket address family type.
@param[out] ipFamily - PS layer socket address family type.

@return     AEE_SUCCESS - On success
@return     QDS_EFAULT - invalid parameters.
*/
ds::ErrorType DS2PSSockAddrFamily
(
  ds::AddrFamilyType dsFamily,
  uint16             * ipFamily
)
{
  if(0 == ipFamily)
  {
    return QDS_EFAULT;
  }

  switch(dsFamily)
  {
  case ds::AddrFamily::QDS_AF_INET:
    *ipFamily = DSS_AF_INET;
    break;

  case ds::AddrFamily::QDS_AF_INET6:
    *ipFamily = DSS_AF_INET6;
    break;

  case ds::AddrFamily::QDS_AF_UNSPEC:
    *ipFamily = DSS_AF_UNSPEC;
    break;

  default:
    *ipFamily = DSS_AF_UNSPEC;
  }

  return AEE_SUCCESS;
}

} /* namespace Conversion */
} /* namespace Net */
} /* namespace ds */



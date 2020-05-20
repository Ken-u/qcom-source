/*======================================================

FILE:  DSS_Conversion.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_Conversion functions

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_Conversion.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-13 en  History added.

===========================================================================*/

#include "DSS_Conversion.h"
#include "DSS_Common.h"
#include "DSS_Globals.h"
#include "ds_Net_IPolicyPriv.h"
#include "dss_iface_ioctl.h"
#include "ds_Net_IMBMSSpecPriv.h"
#include "DSS_IDSNetPolicyPrivScope.h"
#include "DSS_MemoryManagement.h"
#include "ps_iface_defs.h"
#include "ps_system_heap.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Sock_ISocketLocalPriv.h"
#include "ds_Net_INetworkFactory.h"
#include "DSS_GenScope.h"
#include "ds_Addr_Def_Helpers.h"
#include "ds_Sock_AddrUtils.h"
#include "ds_Net_Def.h"
#include "ds_Net_IMCastManagerPriv.h"
#include "ds_Utils_DebugMsg.h"
#include "ps_in_alias.h"

using namespace ds::Net;
//Temp hack : Added validation code.
//This would not work since ps_iface_ipfltr.h is not a remoted API.
//We need to figure out a method to perform the validation.
//ps_flowi.h is included since MAX_FLTR_PER_REQ and
//MAX_ADDITIONAL_FLOWS_PER_REQ are only defined there.
#include "ps_iface_ipfltr.h"
#include "ps_flowi.h"


static boolean DSSInternalValidateFilterSpec
(
  const ip_filter_spec_type          * fltr_spec_ptr,
  qos_spec_field_mask_enum_type        flow_type
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-----------------------------------------------------------------------
    Validate that number of filters is in between (0, MAX_FLTR_PER_REQ]
  -----------------------------------------------------------------------*/
  if (fltr_spec_ptr->num_filters == 0)
  {
    LOG_MSG_INVALID_INPUT("Zero filters specified for mask 0x%x", flow_type, 0, 0);
    return FALSE;
  }
  else if (fltr_spec_ptr->num_filters > MAX_FLTR_PER_REQ)
  {
    LOG_MSG_INVALID_INPUT("Too many filters specified: %d, max = %d, for mask 0x%x",
             fltr_spec_ptr->num_filters, MAX_FLTR_PER_REQ, flow_type);
    return FALSE;
  }
  else if (fltr_spec_ptr->list_ptr == NULL)
  {
    LOG_MSG_INVALID_INPUT("NULL list_ptr in fltr spec for mask 0x%x", flow_type, 0, 0);
    return FALSE;
  }

  return TRUE;
} /* DSSInternalValidateFilterSpec() */

static boolean DSSInternalValidateFlowParam
(
  ip_flow_type                   * ip_flow_ptr,
  qos_spec_field_mask_enum_type    flow_type
)
{
  boolean  is_flow_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ip_flow_ptr->err_mask = 0;

  if (ip_flow_ptr->field_mask == (ipflow_field_mask_type)IPFLOW_MASK_NONE)
  {
    LOG_MSG_ERROR("Invalid IP_FLOW field mask: 0x%x in QOS flow type: 0x%x",
              ip_flow_ptr->field_mask, flow_type, 0);
    ip_flow_ptr->err_mask = (ipflow_field_mask_type)IPFLOW_MASK_ALL;
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error and continue with validation so that other parameters are validated
  -------------------------------------------------------------------------*/
  if (ip_flow_ptr->field_mask & ~(ipflow_field_mask_type)IPFLOW_MASK_ALL)
  {
    ip_flow_ptr->err_mask = (ip_flow_ptr->field_mask & ~(ipflow_field_mask_type)IPFLOW_MASK_ALL);
    is_flow_valid = FALSE;
  }

  return is_flow_valid;
  }

static boolean DSSInternalValidateAuxFlowSpec
(
  const ip_flow_spec_type            * ip_flow_spec_ptr,
  qos_spec_field_mask_enum_type        flow_type
)
{
  uint8    aux_flow_index;
  boolean  is_aux_flow_spec_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Validate that # of auxiliary flow specs are in range
    (0, MAX_ADDITIONAL_FLOWS_PER_REQ)
  -------------------------------------------------------------------------*/
  if (ip_flow_spec_ptr->num_aux_flows > MAX_ADDITIONAL_FLOWS_PER_REQ) {
     LOG_MSG_INVALID_INPUT("Too many auxiliary flows specified: %d, max = %d, for mask 0x%x",
              ip_flow_spec_ptr->num_aux_flows,
              MAX_ADDITIONAL_FLOWS_PER_REQ,
              flow_type);
     return FALSE;
  } else if (ip_flow_spec_ptr->num_aux_flows == 0) {
     LOG_MSG_INVALID_INPUT("Zero auxiliary flows specified for mask 0x%x", flow_type, 0, 0);
     return FALSE;
  } else if (ip_flow_spec_ptr->aux_flow_list_ptr == NULL) {
     LOG_MSG_INVALID_INPUT("NULL aux_flow_list_ptr for mask 0x%x", flow_type, 0, 0);
     return FALSE;
  }

  /*-------------------------------------------------------------------------
    Validate each auxiliary flow spec
  -------------------------------------------------------------------------*/
  for (aux_flow_index = 0;
       aux_flow_index < ip_flow_spec_ptr->num_aux_flows;
       aux_flow_index++)
  {
    is_aux_flow_spec_valid &=
      DSSInternalValidateFlowParam
      (
        ip_flow_spec_ptr->aux_flow_list_ptr + aux_flow_index,
        flow_type
      );
  }

  return is_aux_flow_spec_valid;
} /* DSSInternalValidateAuxFlowSpec () */

static AEEResult DSSInternalValidateQoSRequest
(
  dss_iface_ioctl_qos_request_type* qos_req_param
)
{
  boolean  is_qos_spec_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    One of Rx or Tx flow specs must be specified
  -------------------------------------------------------------------------*/
  if (!(qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_FLOW) &&
      !(qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_FLOW)) {
     LOG_MSG_INVALID_INPUT("Invalid QOS spec, mask=%d, specify Rx or Tx flow", qos_req_param->qos.field_mask, 0, 0);
     return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask & ~(qos_spec_field_mask_type)QOS_MASK_ALL) {
     LOG_MSG_INVALID_INPUT("Invalid QOS spec, mask=%d, Bits outside the allowed range "
              "are set", qos_req_param->qos.field_mask, 0, 0);
     return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Validate Rx flow spec and Rx fltr spec
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_FLOW) {

    /*-----------------------------------------------------------------------
      Validate Rx required flow spec
    -----------------------------------------------------------------------*/
    is_qos_spec_valid &=
      DSSInternalValidateFlowParam
      (
        &(qos_req_param->qos.rx.flow_template.req_flow),
        QOS_MASK_RX_FLOW
      );
    

    /*-----------------------------------------------------------------------
      Validate Rx minimum required flow spec
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW) 
    {
      is_qos_spec_valid &=
        DSSInternalValidateFlowParam
        (
          &(qos_req_param->qos.rx.flow_template.min_req_flow),
          QOS_MASK_RX_MIN_FLOW
        );
    }

    /*-----------------------------------------------------------------------
      Validate auxiliary Rx flow specs
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS) 
    {
      is_qos_spec_valid &=
         DSSInternalValidateAuxFlowSpec
           (
             &(qos_req_param->qos.rx.flow_template),
             QOS_MASK_RX_AUXILIARY_FLOWS
           );
    }

    is_qos_spec_valid &=
      DSSInternalValidateFilterSpec
      (
        &(qos_req_param->qos.rx.fltr_template),
        QOS_MASK_RX_FLOW
      );
    
  }
  else
  {
    /*-----------------------------------------------------------------------
      Validate that RX_MIN_FLOW or RX_AUXILIARY_FLOWS is not specified. It is
      mandatory to specify required flow spec if app wants to use QOS in a
      direction
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW)
    {
      LOG_MSG_INFO1("RX_MIN_FLOW is specified without RX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }

    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS)
    {
      LOG_MSG_INFO1("RX_AUXILIARY_FLOWS is specified without RX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }
  }

  /*-------------------------------------------------------------------------
    Validate Tx flow spec and Tx fltr spec
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_FLOW)
  {
    /*-----------------------------------------------------------------------
      Validate Tx required flow spec
    -----------------------------------------------------------------------*/
    is_qos_spec_valid &=
      DSSInternalValidateFlowParam
      (
        &(qos_req_param->qos.tx.flow_template.req_flow),
        QOS_MASK_TX_FLOW
      );

    /*-----------------------------------------------------------------------
      Validate Tx minimum required flow spec
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW)
    {
      is_qos_spec_valid &=
        DSSInternalValidateFlowParam
        (
          &(qos_req_param->qos.tx.flow_template.min_req_flow),
          QOS_MASK_TX_MIN_FLOW
        );
    }

    /*-----------------------------------------------------------------------
      Validate auxiliary Tx flow specs
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS)
    {
      is_qos_spec_valid &=
         DSSInternalValidateAuxFlowSpec
         (
           &(qos_req_param->qos.tx.flow_template),
           QOS_MASK_TX_AUXILIARY_FLOWS
         );
    }

    /*-----------------------------------------------------------------------
      Validate Tx fltr spec
    -----------------------------------------------------------------------*/
    is_qos_spec_valid &=
       DSSInternalValidateFilterSpec
       (
         &(qos_req_param->qos.tx.fltr_template),
         QOS_MASK_TX_FLOW
       );
  }
  else /* if TX_FLOW is not specified */
  {
    /*-----------------------------------------------------------------------
      Validate that TX_MIN_FLOW or TX_AUXILIARY_FLOWS is not specified. It is
      mandatory to specify required flow spec if app wants to use QOS in a
      direction
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW)
    {
      LOG_MSG_INFO1("TX_MIN_FLOW is specified without TX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }

    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS)
    {
      LOG_MSG_INVALID_INPUT("TX_AUXILIARY_FLOWS is specified without TX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }
  }

  if (is_qos_spec_valid == FALSE)
  {
    LOG_MSG_INVALID_INPUT("QOS Spec validation failed", 0, 0, 0);
    return QDS_EFAULT;
  }

  return AEE_SUCCESS;

} /* DSSInternalValidateQoSRequest() */


static AEEResult DSSInternalValidateQoSModifyRequest
(
  dss_iface_ioctl_qos_request_type* qos_req_param
)
{
  boolean  is_qos_spec_valid = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    One of Rx or Tx flow specs must be specified
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask == QOS_MASK_INVALID) {      
     LOG_MSG_INVALID_INPUT("Invalid QOS spec, mask=%d, APP is not changing anything", qos_req_param->qos.field_mask, 0, 0);
     return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    If a bit mask is specified, which is outside the supported range, flag
    error
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask & ~(qos_spec_field_mask_type)QOS_MODIFY_MASK_ALL) {
     LOG_MSG_INVALID_INPUT("Invalid QOS spec, mask=%d, Bits outside the allowed range "
              "are set", qos_req_param->qos.field_mask, 0, 0);
     return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Validate Rx flow spec and Rx fltr spec
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_FLOW) {

    /*-----------------------------------------------------------------------
      Validate Rx required flow spec
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.rx.flow_template.req_flow.field_mask != (ipflow_field_mask_type)IPFLOW_MASK_NONE) 
    {
      is_qos_spec_valid &=
        DSSInternalValidateFlowParam
        (
          &(qos_req_param->qos.rx.flow_template.req_flow),
          QOS_MASK_RX_FLOW
        );
    }

    /*-----------------------------------------------------------------------
      Validate Rx minimum required flow spec
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW) 
    {
      is_qos_spec_valid &=
        DSSInternalValidateFlowParam
        (
          &(qos_req_param->qos.rx.flow_template.min_req_flow),
          QOS_MASK_RX_MIN_FLOW
        );
    }

    /*-----------------------------------------------------------------------
      Validate auxiliary Rx flow specs
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS) 
    {
      is_qos_spec_valid &=
         DSSInternalValidateAuxFlowSpec
           (
             &(qos_req_param->qos.rx.flow_template),
             QOS_MASK_RX_AUXILIARY_FLOWS
           );
    }
  }
  else
  {
    /*-----------------------------------------------------------------------
      Validate that RX_MIN_FLOW or RX_AUXILIARY_FLOWS is not specified. It is
      mandatory to specify required flow spec if app wants to use QOS in a
      direction
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW)
    {
      LOG_MSG_INFO1("RX_MIN_FLOW is specified without RX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }

    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS)
    {
      LOG_MSG_INFO1("RX_AUXILIARY_FLOWS is specified without RX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }
  }

  /*-------------------------------------------------------------------------
    Validate Tx flow spec and Tx fltr spec
  -------------------------------------------------------------------------*/
  if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_FLOW)
  {
    /*-----------------------------------------------------------------------
      Validate Tx required flow spec
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.tx.flow_template.req_flow.field_mask != (ipflow_field_mask_type)IPFLOW_MASK_NONE)
    {
      is_qos_spec_valid &=
        DSSInternalValidateFlowParam
        (
          &(qos_req_param->qos.tx.flow_template.req_flow),
          QOS_MASK_TX_FLOW
        );
    }

    /*-----------------------------------------------------------------------
      Validate Tx minimum required flow spec
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW)
    {
      is_qos_spec_valid &=
        DSSInternalValidateFlowParam
        (
          &(qos_req_param->qos.tx.flow_template.min_req_flow),
          QOS_MASK_TX_MIN_FLOW
        );
    }

    /*-----------------------------------------------------------------------
      Validate auxiliary Tx flow specs
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS)
    {
      is_qos_spec_valid &=
         DSSInternalValidateAuxFlowSpec
         (
           &(qos_req_param->qos.tx.flow_template),
           QOS_MASK_TX_AUXILIARY_FLOWS
         );
    }

    /*-----------------------------------------------------------------------
      Validate Tx fltr spec
    -----------------------------------------------------------------------*/
  }
  else /* if TX_FLOW is not specified */
  {
    /*-----------------------------------------------------------------------
      Validate that TX_MIN_FLOW or TX_AUXILIARY_FLOWS is not specified. It is
      mandatory to specify required flow spec if app wants to use QOS in a
      direction
    -----------------------------------------------------------------------*/
    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW)
    {
      LOG_MSG_INFO1("TX_MIN_FLOW is specified without TX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }

    if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS)
    {
      LOG_MSG_INVALID_INPUT("TX_AUXILIARY_FLOWS is specified without TX_FLOW, mask = %d",
               qos_req_param->qos.field_mask, 0, 0);
      return QDS_EFAULT;
    }
  }

  if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MODIFY_MASK_RX_FLTR_MODIFY) 
  {
      is_qos_spec_valid &=
         DSSInternalValidateFilterSpec
         (
           &(qos_req_param->qos.rx.fltr_template),
           QOS_MASK_RX_FLOW
         );
  }


  if (qos_req_param->qos.field_mask & (qos_spec_field_mask_type)QOS_MODIFY_MASK_TX_FLTR_MODIFY)
  {
      is_qos_spec_valid &=
         DSSInternalValidateFilterSpec
         (
           &(qos_req_param->qos.tx.fltr_template),
           QOS_MASK_RX_FLOW
         );
  }


  if (is_qos_spec_valid == FALSE)
  {
    LOG_MSG_INVALID_INPUT("QOS Spec validation failed", 0, 0, 0);
    return QDS_EFAULT;
  }

  return AEE_SUCCESS;

} /* DSSInternalValidateQoSModifyRequest() */



static AEEResult DSSInternalValidatePrimaryQoSModifySpec
(
 dss_iface_ioctl_primary_qos_modify_type* qos_modify_param,
 boolean                           isModify
 )
{
   boolean  is_qos_spec_valid = TRUE;
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   /*-------------------------------------------------------------------------
   One of Rx or Tx flow specs must be specified
   -------------------------------------------------------------------------*/
      if (!(qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_FLOW) &&
         !(qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_FLOW)) {
             LOG_MSG_INVALID_INPUT("Invalid QOS spec, mask=%d, specify Rx or Tx flow", qos_modify_param->primary_qos_spec.field_mask, 0, 0);
             return QDS_EFAULT;
         }

      /*-------------------------------------------------------------------------
      If a bit mask is specified, which is outside the supported range, flag
      error
      -------------------------------------------------------------------------*/
      if (qos_modify_param->primary_qos_spec.field_mask & ~(qos_spec_field_mask_type)QOS_MASK_ALL) {
         LOG_MSG_INVALID_INPUT("Invalid QOS spec, mask=%d, Bits outside the allowed range "
            "are set", qos_modify_param->primary_qos_spec.field_mask, 0, 0);
         return QDS_EFAULT;
      }

      /*-------------------------------------------------------------------------
      Validate Rx flow spec and Rx fltr spec
      -------------------------------------------------------------------------*/
      if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_FLOW) {

         /*-----------------------------------------------------------------------
         Validate Rx required flow spec
         -----------------------------------------------------------------------*/
         if (isModify == FALSE || qos_modify_param->primary_qos_spec.rx_flow_template.req_flow.field_mask != (ipflow_field_mask_type)IPFLOW_MASK_NONE) 
         {
            is_qos_spec_valid &=
               DSSInternalValidateFlowParam
               (
               &(qos_modify_param->primary_qos_spec.rx_flow_template.req_flow),
               QOS_MASK_RX_FLOW
               );
         }

         /*-----------------------------------------------------------------------
         Validate Rx minimum required flow spec
         -----------------------------------------------------------------------*/
         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW) 
         {
            is_qos_spec_valid &=
               DSSInternalValidateFlowParam
               (
               &(qos_modify_param->primary_qos_spec.rx_flow_template.min_req_flow),
               QOS_MASK_RX_MIN_FLOW
               );
         }

         /*-----------------------------------------------------------------------
         Validate auxiliary Rx flow specs
         -----------------------------------------------------------------------*/
         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS) 
         {
            is_qos_spec_valid &=
               DSSInternalValidateAuxFlowSpec
               (
               &(qos_modify_param->primary_qos_spec.rx_flow_template),
               QOS_MASK_RX_AUXILIARY_FLOWS
               );
         }
      }
      else
      {
         /*-----------------------------------------------------------------------
         Validate that RX_MIN_FLOW or RX_AUXILIARY_FLOWS is not specified. It is
         mandatory to specify required flow spec if app wants to use QOS in a
         direction
         -----------------------------------------------------------------------*/
         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW)
         {
            LOG_MSG_INFO1("RX_MIN_FLOW is specified without RX_FLOW, mask = %d",
               qos_modify_param->primary_qos_spec.field_mask, 0, 0);
            return QDS_EFAULT;
         }

         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS)
         {
            LOG_MSG_INFO1("RX_AUXILIARY_FLOWS is specified without RX_FLOW, mask = %d",
               qos_modify_param->primary_qos_spec.field_mask, 0, 0);
            return QDS_EFAULT;
         }
      }

      /*-------------------------------------------------------------------------
      Validate Tx flow spec and Tx fltr spec
      -------------------------------------------------------------------------*/
      if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_FLOW)
      {
         /*-----------------------------------------------------------------------
         Validate Tx required flow spec
         -----------------------------------------------------------------------*/
         if (isModify == FALSE || qos_modify_param->primary_qos_spec.tx_flow_template.req_flow.field_mask != (ipflow_field_mask_type)IPFLOW_MASK_NONE)
         {
            is_qos_spec_valid &=
               DSSInternalValidateFlowParam
               (
               &(qos_modify_param->primary_qos_spec.tx_flow_template.req_flow),
               QOS_MASK_TX_FLOW
               );
         }

         /*-----------------------------------------------------------------------
         Validate Tx minimum required flow spec
         -----------------------------------------------------------------------*/
         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW)
         {
            is_qos_spec_valid &=
               DSSInternalValidateFlowParam
               (
               &(qos_modify_param->primary_qos_spec.tx_flow_template.min_req_flow),
               QOS_MASK_TX_MIN_FLOW
               );
         }

         /*-----------------------------------------------------------------------
         Validate auxiliary Tx flow specs
         -----------------------------------------------------------------------*/
         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS)
         {
            is_qos_spec_valid &=
               DSSInternalValidateAuxFlowSpec
               (
               &(qos_modify_param->primary_qos_spec.tx_flow_template),
               QOS_MASK_TX_AUXILIARY_FLOWS
               );
         }
      }
      else /* if TX_FLOW is not specified */
      {
         /*-----------------------------------------------------------------------
         Validate that TX_MIN_FLOW or TX_AUXILIARY_FLOWS is not specified. It is
         mandatory to specify required flow spec if app wants to use QOS in a
         direction
         -----------------------------------------------------------------------*/
         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW)
         {
            LOG_MSG_INFO1("TX_MIN_FLOW is specified without TX_FLOW, mask = %d",
               qos_modify_param->primary_qos_spec.field_mask, 0, 0);
            return QDS_EFAULT;
         }

         if (qos_modify_param->primary_qos_spec.field_mask & (qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS)
         {
            LOG_MSG_INVALID_INPUT("TX_AUXILIARY_FLOWS is specified without TX_FLOW, mask = %d",
               qos_modify_param->primary_qos_spec.field_mask, 0, 0);
            return QDS_EFAULT;
         }
      }

      if (is_qos_spec_valid == FALSE)
      {
         LOG_MSG_INVALID_INPUT("QOS Spec validation failed", 0, 0, 0);
         return QDS_EFAULT;
      }

      return AEE_SUCCESS;

} /* DSSInternalValidateQoSRequest() */

// See DSS_Conversion.h.
sint15 DSSConversion::IDS2DSErrorCode(AEEResult nRes)
{
   switch (nRes) {
      case AEE_SUCCESS:
         return DSS_SUCCESS;
      case AEE_ENOMEMORY:
         return DS_ENOMEM;
      case AEE_EFAILED:
         // TODO: check
         return DSS_ERROR;
      case QDS_EEOF:
         return DS_EEOF;
      case QDS_EINTR:
         return DS_EINTR;
      case QDS_EBADF:
         return DS_EBADF;
      case QDS_EFAULT:
         return DS_EFAULT;
      case AEE_EWOULDBLOCK:
         return DS_EWOULDBLOCK;
      case QDS_EAFNOSUPPORT:
         return DS_EAFNOSUPPORT;
      case QDS_EPROTOTYPE:
         return DS_EPROTOTYPE;
      case QDS_ESOCKNOSUPPORT:
         return DS_ESOCKNOSUPPORT;
      case QDS_EPROTONOSUPPORT:
         return DS_EPROTONOSUPPORT;
      case QDS_EMFILE:
         return DS_EMFILE;
      case QDS_EOPNOTSUPP:
         return DS_EOPNOTSUPP;
      case QDS_EADDRINUSE:
         return DS_EADDRINUSE;
      case QDS_EADDRREQ:
         return DS_EADDRREQ;
      case QDS_EINPROGRESS:
         return DS_EINPROGRESS;
      case QDS_EISCONN:
         return DS_EISCONN;
      case QDS_EIPADDRCHANGED:
         return DS_EIPADDRCHANGED;
      case QDS_ENOTCONN:
         return DS_ENOTCONN;
      case QDS_ECONNREFUSED:
         return DS_ECONNREFUSED;
      case QDS_ETIMEDOUT:
         return DS_ETIMEDOUT;
      case QDS_ECONNRESET:
         return DS_ECONNRESET;
      case QDS_ECONNABORTED:
         return DS_ECONNABORTED;
      case QDS_EPIPE:
         return DS_EPIPE;
      case QDS_ENETDOWN:
         return DS_ENETDOWN;
      case QDS_EMAPP:
         return DS_EMAPP;
      case QDS_EBADAPP:
         return DS_EBADAPP;
      case QDS_ESOCKEXIST:
         return DS_SOCKEXIST;
      case QDS_EINVAL:
         return DS_EINVAL;
      case QDS_EMSGSIZE:
         return DS_EMSGSIZE;
      case QDS_ENOPROTOOPT:
         return DS_ENOPROTOOPT;
      case QDS_EMSGTRUNC:
         return DS_EMSGTRUNC;
      case QDS_ENOROUTE:
         return DS_ENOROUTE;
      case QDS_ESHUTDOWN:
         return DS_ESHUTDOWN;
      case QDS_ENETUNREACH:
         return DS_ENETUNREACH;
      case QDS_EHOSTUNREACH:
         return DS_EHOSTUNREACH;
      case QDS_EHOSTDOWN:
         return DS_EHOSTDOWN;
      case QDS_ENONET:
         return DS_ENONET;
      case QDS_EPROTO:
         return DS_EPROTO;
      case QDS_EACCES:
         return DS_EACCES;
      case QDS_EQOSUNAWARE:
         return DS_EQOSUNAWARE;
      case QDS_ENOSRCADDR:
         return DS_ENOSRCADDR;
      case QDS_ENETISCONN:
         return DS_ENETISCONN;
      case QDS_ENETINPROGRESS:
         return DS_ENETINPROGRESS;
      case QDS_ENETNONET:
         return DS_ENETNONET;
      case QDS_ENETCLOSEINPROGRESS:
         return DS_ENETCLOSEINPROGRESS;
      case QDS_ENETEXIST:
         return DS_ENETEXIST;
      case QDS_ENETGOINGDORMANT:
         return DS_ENETGOINGDORMANT;
      case QDS_ENAMEERR:
         return DS_NAMEERR;
      case QDS_EINUSE:
         return DS_INUSE;
      case QDS_EHOSTNOTFOUND:
         return DS_EHOSTNOTFOUND;
      case QDS_ETRYAGAIN:
         return DS_ETRYAGAIN;
      case QDS_ENORECOVERY:
         return DS_ENORECOVERY;
      case QDS_ENOADDRESS:
         return DS_ENOADDRESS;
      case QDS_ETRUNCATED:
         return DS_ETRUNCATED;
      case QDS_ESYSTEM:
         return DS_ESYSTEM;
      case QDS_EMAXQUERIES:
         return DS_EMAXQUERIES;
      case  QDS_BCMCSDB_OVERFLOW:
         return DS_EDBOVERFLOW;
      case QDS_BCMCSDB_NO_OVERWRITE:
         return DS_EDBNOOVERWRITE;
      case AEE_EPRIVLEVEL: 
         /*-----------------------------------------------------------------------
           This is an internal error.
           Legacy APIs should have full privilege and therefore AEE_EPRIVLEVEL
           should never be returned.
         -----------------------------------------------------------------------*/
         LOG_MSG_ERROR ("Trying to return AEE_EPRIVLEVEL to legacy API", 0, 0, 0);
         ASSERT(0);
         return DSS_ERROR;

      default:
         // TODO: if the value is not defined, there should be a BREAKPOINT() and a debug print
         //TODO: what should the return value be?
         return DSS_ERROR;
   }
}

AEEResult DSSConversion::DS2IDSNetPolicy(const dss_net_policy_info_type *from, IPolicyPriv *to)
{
   AEEResult res;
   ds::AddrFamilyType addrFamily;
   int CDMAProfile = 0; // initialize to zero
   int UMTSProfile = 0; // initialize to zero
   ds::Net::IfaceNameType ifaceName;
   ds::Net::IfaceGroupType ifaceGroup;

   // if legacy policy is NULL, than IPolicy should contain the default values, which it already does since the creation. 
   // silently return SUCCESS
   if (NULL == from) {
      return AEE_SUCCESS;
   }

   // TODO: do I need the following?
   /*if (from->dss_netpolicy_private.cookie != DSS_NETPOLICY_COOKIE) {
   // Net Policy is not initialized;
   *dss_errno = DS_EFAULT;
   return DSS_ERROR;
   }*/


   /* 
     ipsec_disabled is deprecated and is not supported in QCM API.
     Thus, it's value is not converted here.
   */

   // Set CDMA and UMTS profile numbers.
   UMTSProfile = from->umts.pdp_profile_num;
   CDMAProfile = from->cdma.data_session_profile_id;

   switch (from->iface.kind){
      case DSS_IFACE_NAME:
         res = DS2IDSIfaceName (from->iface.info.name, &ifaceName);
         if (-1 != res) {
            res = to->SetIfaceName ((ds::Net::IfaceNameType) from->iface.info.name);
            if (AEE_SUCCESS != res){
               LOG_MSG_ERROR("DSSConversion: SetIfaceName failed: %d", res, 0, 0);
               return res;
            }
         } else {
               res = DS2IDSIfaceGroup (from->iface.info.name, &ifaceGroup);
               if (-1 != res) {
                  res = to->SetIfaceGroup ((ds::Net::IfaceGroupType) from->iface.info.name);
                  if (AEE_SUCCESS != res){
                     LOG_MSG_ERROR("DSSConversion: SetIfaceGroup failed: %d", res, 0, 0);
                     return res;
                  }
               } else {
                     res = to->SetIfaceGroup (ds::Net::IfaceGroup::GRP_ANY_DEFAULT);
                     if (AEE_SUCCESS != res){
                        LOG_MSG_ERROR("DSSConversion: SetIfaceGroup failed: %d", res, 0, 0);
                        return res;
                     }
                  }
         }

         break;

      case DSS_IFACE_ID:
      {
            //IDSNetPolicyPriv* pNetPolicyPriv;
            DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;

            IDS_ERR_RET(IDSNetPolicyPrivScope.Init(to));

            IDS_ERR_RET(IDSNetPolicyPrivScope.Fetch()->SetIfaceId((from->iface.info.id & 0xFF000000) | 0x00FFFF00));

            break;
      }

         default:
            return QDS_EFAULT;
   }

   res = DS2IDSAddrFamily(from->family, &addrFamily);
   if (AEE_SUCCESS != res) {
      return res;
   }

   IDS_ERR_RET(to->SetPolicyFlag ((PolicyFlagType) from->policy_flag));
   IDS_ERR_RET(to->SetAddressFamily(addrFamily));
   IDS_ERR_RET(to->SetCDMAProfileNumber(CDMAProfile));
   IDS_ERR_RET(to->SetUMTSProfileNumber(UMTSProfile));
   IDS_ERR_RET(to->SetRoutable(from->is_routeable));

   if (NULL != from->umts.apn.name)
   {
      IDS_ERR_RET(to->SetUMTSAPNName ((const char *) from->umts.apn.name));
   }
   
   IDS_ERR_RET(to->SetAppType((uint32)from->app_identifier));

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSNetPolicy(IPolicy *from, dss_net_policy_info_type *to, sint15 appid)
{
   PolicyFlagType IDSPolicyFlag;
   ds::Net::IfaceNameType IDSIfaceName;
   ds::AddrFamilyType IDSAddressFamily;
   int CDMAIDSProfileNumber;
   int UMTSIDSProfileNumber;
   int appType;
   boolean IDSRoutable;
   IfaceGroupType IDSIfaceGroup;
   DSSIDSNetPolicyPrivScope NetPolicyPrivScope;
   IfaceIdType ifaceId = INVALID_IFACE_ID;
   AEEResult res = AEE_SUCCESS;

   if (NULL == to || NULL == from) {
      return QDS_EFAULT;
   }

   res = NetPolicyPrivScope.Init(from);
   if (AEE_SUCCESS != res) {
       goto bail;
   }

   /* 
      ipsec_disabled is deprecated and is not supported in QCM API.
      Thus, its value inside "to" structure is only initialized and not converted.
   */
   dss_init_net_policy_info(to);

   res = NetPolicyPrivScope.Fetch()->GetIfaceId(&ifaceId);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   res = from->GetPolicyFlag(&IDSPolicyFlag);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetIfaceName(&IDSIfaceName);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetIfaceGroup(&IDSIfaceGroup);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetAddressFamily(&IDSAddressFamily);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetCDMAProfileNumber(&CDMAIDSProfileNumber);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetUMTSProfileNumber(&UMTSIDSProfileNumber);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetRoutable(&IDSRoutable);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = from->GetAppType(&appType);
   if (AEE_SUCCESS != res) {
     goto bail;
   }
   // If iface id is set, use iface id
   // Else if iface name is set, use iface name
   // Else if iface group is set, use iface group as iface name
   // Else use IFACE_ANY_DEFAULT as iface name.
   if (0 != ifaceId) {
      to->iface.kind = DSS_IFACE_ID;
      if ((0 < appid) && (DSS_MAX_APPS >= appid)) {
         // Valid appid
         IDS_ERR_RET(BuildIfaceId(NetPolicyPrivScope.Fetch(),appid,&(to->iface.info.id)));
      }
      else{
         // appid invalid or not provided
         IDS_ERR_RET(BuildIfaceId(NetPolicyPrivScope.Fetch(),-1,&(to->iface.info.id)));
      }
   }
   else {
      to->iface.kind = DSS_IFACE_NAME;

      if (0 != IDSIfaceName) {
         to->iface.info.name = (dss_iface_name_enum_type) IDSIfaceName;
      } else if (0 != IDSIfaceGroup) {
         to->iface.info.name = (dss_iface_name_enum_type) IDSIfaceGroup;
      } else {
         to->iface.info.name = DSS_IFACE_ANY_DEFAULT;
      }
      }

   to->umts.pdp_profile_num = UMTSIDSProfileNumber;
   to->cdma.data_session_profile_id = CDMAIDSProfileNumber;

   //TODO: to->SetAppType(nAppType); Why this was not set in the corresponding function that convert from DS to IDS?

   // TODO: How to update the cookie?

   res = IDS2DSPolicyFlag(IDSPolicyFlag, &to->policy_flag);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   // TODO: meanwhile, I should return error (unsupported) for this case (make sure that we don't use this option in the OEM)

   res = IDS2DSAddrFamily(IDSAddressFamily, &(to->family));
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   to->is_routeable = IDSRoutable;

   to->app_identifier = appType;

bail:
   return res;
}

AEEResult DSSConversion::DS2IDSPolicyFlag(dss_iface_policy_flags_enum_type DSFlag, PolicyFlagType* IDSFlag)
{
   *IDSFlag = (int)DSFlag;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSPolicyFlag(PolicyFlagType IDSPolicyFlag, dss_iface_policy_flags_enum_type* DSFlag)
{
   *DSFlag = (dss_iface_policy_flags_enum_type)IDSPolicyFlag;
   return AEE_SUCCESS;

}

AEEResult DSSConversion::DS2IDSIfaceName(dss_iface_name_enum_type DSIfaceName, ds::Net::IfaceNameType* IDSIfaceName)
{
   *IDSIfaceName = (int)DSIfaceName;
   return AEE_SUCCESS;
}

// All non-public interface conversions should be added here
AEEResult DSSConversion::DS2IDSIfaceNameInternal(dss_iface_name_enum_type_internal DSIfaceName, ds::Net::IfaceNameType* IDSIfaceName)
{
   *IDSIfaceName = (int)DSIfaceName;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSIfaceGroup(dss_iface_name_enum_type DSIfaceGroup, ds::Net::IfaceGroupType* IDSIfaceGroup)
{
   *IDSIfaceGroup = (int)DSIfaceGroup;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSAddrFamily(const int DSFamily,ds::AddrFamilyType* AddrFamily)
{
   switch(DSFamily)
   {
      case DSS_AF_INET:
         *AddrFamily = ds::AddrFamily::QDS_AF_INET;
      break;

      case DSS_AF_INET6:
         *AddrFamily = ds::AddrFamily::QDS_AF_INET6;
      break;

      case DSS_AF_UNSPEC:
         *AddrFamily = ds::AddrFamily::QDS_AF_UNSPEC;
      break;

      case DSS_AF_ANY:
         return QDS_EAFNOSUPPORT;

      default:
         *AddrFamily = ds::AddrFamily::QDS_AF_UNSPEC;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSAddrFamily(const ds::AddrFamilyType IDSFamily, int* DSFamily)
{
   switch(IDSFamily)
   {
      case ds::AddrFamily::QDS_AF_INET:
         *DSFamily = DSS_AF_INET;
         break;

      case ds::AddrFamily::QDS_AF_INET6:
         *DSFamily = DSS_AF_INET6;
         break;

      case ds::AddrFamily::QDS_AF_UNSPEC:
         *DSFamily = DSS_AF_UNSPEC;
         break;

      default:
         *DSFamily = DSS_AF_UNSPEC;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSEnumAddrFamily(ds::AddrFamilyType idsFamily, ip_addr_enum_type* DSFamily)
{
   // The conversion from IDS Address Family to ds Address Family and to ip_addr_enum_type
   // and thus we must use a direct convertion
   switch (idsFamily) {
      case ds::AddrFamily::QDS_AF_UNSPEC:
         *DSFamily = IP_ADDR_INVALID; //TODO: is this correct?
         break;
      case ds::AddrFamily::QDS_AF_INET:
         *DSFamily = IPV4_ADDR;
         break;
      case ds::AddrFamily::QDS_AF_INET6:
         *DSFamily = IPV6_ADDR;
         break;
      default:
         //TODO: BREAKPOINT()
         *DSFamily = IP_ADDR_INVALID;
         return QDS_EINVAL;
   }
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSEnumAddrFamily(ip_addr_enum_type dsFamily, ds::AddrFamilyType* idsFamily)
{
   switch (dsFamily) {
      case IP_ADDR_INVALID: //TODO: is this correct?
         *idsFamily = ds::AddrFamily::QDS_AF_UNSPEC;
         break;
      case IPV4_ADDR:
         *idsFamily = ds::AddrFamily::QDS_AF_INET;
         break;
      case IPV6_ADDR:
         *idsFamily = ds::AddrFamily::QDS_AF_INET6;
         break;
      case IP_ANY_ADDR:
         LOG_MSG_ERROR ("DS2IDSEnumAddrFamily IP_ANY_ADDR - This value is for internal usage only, it should never be propagated to user", 0, 0, 0); 
         ASSERT (0); 
         return QDS_EFAULT;
      default:
         return QDS_EFAULT;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSSockType(const int DSSockType, ds::Sock::SocketType *IDSSockType)
{
   switch (DSSockType) {
      case DSS_SOCK_STREAM:
         *IDSSockType = ds::Sock::Type::QDS_STREAM;
         break;
      case DSS_SOCK_DGRAM:
         *IDSSockType = ds::Sock::Type::QDS_DGRAM;
         break;
         // TODO: ?
         /*case SOCK_ICMP:
         return ds::Sock::Type::UNSPEC;*/
      default:
         return QDS_ESOCKNOSUPPORT;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSProtocol(const int DSProtocol, ds::Sock::ProtocolType *IDSProtocol)
{
   *IDSProtocol = DSProtocol;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSEventId(const int DSEventId , ds::Sock::SocketEventType* EventID)
{
   *EventID = DSEventId;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSOptLevel(const dss_sockopt_levels_type DSOptLevel, ds::Sock::OptLevelType* OptLevel)
{
   *OptLevel = (ds::Sock::OptLevelType)DSOptLevel;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSOptName(const dss_sockopt_names_type DSOptName, ds::Sock::OptNameType* OptName)
{  
   switch (DSOptName) {
      case DSS_IP_TTL:
         *OptName = ds::Sock::OptName::QDS_IP_TTL;
         break;
      case DSS_SO_RCVBUF:
         *OptName = ds::Sock::OptName::QDS_SO_RCVBUF;
         break;
      case DSS_SO_SNDBUF:
         *OptName = ds::Sock::OptName::QDS_SO_SNDBUF;
         break;
      case DSS_TCP_MAXSEG:
         *OptName = ds::Sock::OptName::QDS_TCP_MAXSEG;
         break;
      case DSS_SO_SDB_ACK_CB:
         // TODO
         break;
      case DSS_TCP_NODELAY:
         *OptName = ds::Sock::OptName::QDS_TCP_NODELAY;
         break;
      case DSS_SO_KEEPALIVE:
         *OptName = ds::Sock::OptName::QDS_SO_KEEPALIVE;
         break;
      case DSS_SO_TX_IFACE:
         *OptName = ds::Sock::OptName::QDS_SO_TX_IFACE;
         break;
      case DSS_TCP_DELAYED_ACK:
         *OptName = ds::Sock::OptName::QDS_TCP_DELAYED_ACK;
         break;
      case DSS_TCP_SACK:
         *OptName = ds::Sock::OptName::QDS_TCP_SACK;
         break;
      case DSS_TCP_TIME_STAMP:
         *OptName = ds::Sock::OptName::QDS_TCP_TIMESTAMP;
         break;
      case DSS_IP_RECVIF:
         *OptName = ds::Sock::OptName::QDS_IP_RECVIF;
         break;
      case DSS_IP_TOS:
         *OptName = ds::Sock::OptName::QDS_IP_TOS;
         break;
      case DSS_IPV6_TCLASS:
         *OptName = ds::Sock::OptName::QDS_IPV6_TCLASS;
         break;
      case DSS_SO_ERROR_ENABLE:
         *OptName = ds::Sock::OptName::QDS_SO_ERROR_ENABLE;
         break;
      case DSS_SO_ERROR:
         *OptName = ds::Sock::OptName::QDS_SO_ERROR;
         break;
      case DSS_IP_RECVERR:
         *OptName = ds::Sock::OptName::QDS_IP_RECVERR;
         break;
      case DSS_IPV6_RECVERR:
         *OptName = ds::Sock::OptName::QDS_IPV6_RECVERR;
         break;
         //      case DSS_TCP_EIFEL:
         //         // TODO
      case DSS_SO_QOS_SHARE_HANDLE:
         LOG_MSG_INVALID_INPUT("DSS_SO_QOS_SHARE_HANDLE socket option is deprecated",0,0,0);
         return (QDS_EOPNOTSUPP);
      case DSS_SO_REUSEADDR:
         *OptName = ds::Sock::OptName::QDS_SO_REUSEADDR;
         break;
      case DSS_SO_DISABLE_FLOW_FWDING:
         // TODO success is returned temporarily until network level API is defined.
         return AEE_SUCCESS;
         //         *OptName = ds::Sock::OptName::DS_SO_DISABLE_FLOW_FWDING;
         //         break;
      case DSS_IP_MULTICAST_TTL:
         *OptName = ds::Sock::OptName::QDS_IP_MULTICAST_TTL;
         break;
      case DSS_IP_MULTICAST_LOOP:
         *OptName = ds::Sock::OptName::QDS_IP_MULTICAST_LOOP;
         break;
      case DSS_TCP_FIONREAD:
         *OptName = ds::Sock::OptName::QDS_TCP_FIONREAD;
         break;
      case DSS_SO_FIONREAD:
         *OptName = ds::Sock::OptName::QDS_SO_FIONREAD;
         break;
      default:
         return QDS_EFAULT;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSOptNamePriv(const dss_sockopt_names_type DSOptName, ds::Sock::OptNameTypePriv* OptName)
{  
   switch (DSOptName) {
      case DSS_ICMP_ECHO_ID:
         *OptName = ds::Sock::OptNamePriv::QDS_ICMP_ECHO_ID;
         break;
      case DSS_ICMP_ECHO_SEQ_NUM:
         *OptName = ds::Sock::OptNamePriv::QDS_ICMP_ECHO_SEQ_NUM;
         break;
      default:
         return QDS_EFAULT;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSLinger(const dss_so_linger_type *from, ds::Sock::LingerType *to)
{
   to->lingerEnabled = (boolean)(from->l_onoff);
   to->timeInSec = from->l_linger;

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSLinger(const ds::Sock::LingerType *from, dss_so_linger_type *to)
{
   to->l_onoff = (int)(from->lingerEnabled);
   to->l_linger = from->timeInSec;

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSIpAddr(const ds::IPAddrType* idsAddr, ip_addr_type* dsAddr)
{
   AEEResult res = IDS2DSEnumAddrFamily(idsAddr->family, &(dsAddr->type));
   if (AEE_SUCCESS != res) {
      return res;
   }

   /*::ds::INAddr6Type addr;
   addr = idsAddr->addr;*/
   (void)memmove(&dsAddr->addr, (const void *)idsAddr->addr, 16); //TODO: why don't we have a const for this number?
   return res;
}

AEEResult DSSConversion::DS2IDSIpAddr(const ip_addr_type* dsAddr, ds::IPAddrType* idsAddr)
{
   IDS_ERR_RET(DS2IDSEnumAddrFamily(dsAddr->type, &idsAddr->family));
   (void)memmove(idsAddr->addr, &dsAddr->addr, 16); //TODO: why don't we have a const for this number?
   return AEE_SUCCESS;
}


AEEResult DSSConversion::IDS2DSIp6Addr(const ds::INAddr6Type idsAddr, ip_addr_type* dsAddr)
{
   dsAddr->type = IPV6_ADDR;
   (void)memmove(&dsAddr->addr, idsAddr, 16); //TODO: why don't we have a const for this number?
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSIp6Addr(const ip_addr_type* dsAddr, ds::INAddr6Type idsAddr)
{
   (void)memmove(idsAddr, &dsAddr->addr, 16); //TODO: why don't we have a const for this number?
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSSockIpAddr(const ip_addr_type* dsAddr, ds::SockAddrStorageType idsAddr)
{
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   IDS_ERR_RET(DS2IDSEnumAddrFamily(dsAddr->type, &family));

   family = ps_htons(family);

   (void)ds::Sock::AddrUtils::SetFamily(idsAddr, family);
   
   ds::SockAddrIN6Type* pSockAddrIn6 = (ds::SockAddrIN6Type*) (idsAddr);
   (void) memmove(pSockAddrIn6->addr, &dsAddr->addr, sizeof (ds::INAddr6Type)); //TODO: why don't we have a const for this number?

   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSMCastIpAddrSeq(ds::Net::IMCastManagerExt::SeqSockAddrStorageType* addrSeq,
                                              dss_iface_ioctl_mcast_addr_info_type*  mcast_addr_info_array)
{
  int i;

  for (i=0; i<addrSeq->dataLen ; i++)
  {
    IDS_ERR_RET(DS2IDSSockIpAddr(&(mcast_addr_info_array[i].ip_addr), (addrSeq->data[i])));
  }
  addrSeq->dataLenReq = i+1;

  return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSDomainName(const DomainName* idsDomainName,
                                          dss_iface_ioctl_domain_name_type* dsDomainName)
{
   //TODO: ASSERT(sizeof(ds::Net::DomainName) == PS_IFACE_MAX_DOMAIN_NAME_SIZE)
   (void)memmove(dsDomainName->domain_name, idsDomainName,
           PS_IFACE_MAX_DOMAIN_NAME_SIZE);
   return AEE_SUCCESS;
}

//TODO: consider adding generic code for flag masks conversion - can be imported from ResolverUtils.cpp.
AEEResult DSSConversion::IDS2DSBearerInfo(IBearerInfo* piBearerInfo,
                                          dss_iface_ioctl_bearer_tech_type* pdsBearerTech)
{
   ds::Net::IfaceNameType net = 0;
   IDS_ERR_RET(piBearerInfo->GetNetwork(&net));

   memset(pdsBearerTech, 0, sizeof(dss_iface_ioctl_bearer_tech_type));

   // TODO: why are IDSNetBearerTech's definitions so inconsistent with the
   // rest of the defs?
   switch (net) {
      case IfaceName::IFACE_CDMA_SN:
      {
         pdsBearerTech->current_network = PS_IFACE_NETWORK_CDMA;

         unsigned int cdmaMask = 0;
         IDS_ERR_RET(piBearerInfo->GetCDMATypeMask(&cdmaMask));
         pdsBearerTech->data.cdma_type.rat_mask = cdmaMask;

         unsigned int cdmaSoMask = 0;
         IDS_ERR_RET(piBearerInfo->GetCDMAServiceOptionsMask(&cdmaSoMask));
         pdsBearerTech->data.cdma_type.so_mask = cdmaSoMask;
         break;
      }
      case ds::Net::IfaceName::IFACE_UMTS:
      {
         pdsBearerTech->current_network = PS_IFACE_NETWORK_UMTS;

         unsigned int umtsMask = 0;
         IDS_ERR_RET(piBearerInfo->GetUMTSTypeMask(&umtsMask));
         pdsBearerTech->data.umts_type.rat_mask = umtsMask;
         break;
      }
      case ds::Net::IfaceName::IFACE_WLAN:
         pdsBearerTech->current_network = PS_IFACE_NETWORK_WLAN;
         break;
      // in case of ds::Net::IfaceName::INVALID we return an error - INVALID IfaceName is not allowed.
       
      // TODO: what about other network types?
      default:
         return QDS_EINVAL;
   }

   return AEE_SUCCESS;
}
AEEResult DSSConversion::IDS2DSNetworkDownReason(const NetDownReasonType from, dss_net_down_reason_type *to)
{
   *to = (dss_net_down_reason_type)from;
   return AEE_SUCCESS;
}

// Translate a ds::Network::StateType into a dss_iface_ioctl_state_type value.
AEEResult DSSConversion::IDS2DSIfaceState(const NetworkStateType from, dss_iface_ioctl_state_type *to)
{
   *to = (dss_iface_ioctl_state_type)from;
   return AEE_SUCCESS;
}

// Translate a ds::NetPhysLink::StateType into a dss_iface_ioctl_phys_link_state_type value.
AEEResult DSSConversion::IDS2DSPhyslinkState(const PhysLinkStateType from,
                                             dss_iface_ioctl_phys_link_state_type *to)
{
   *to = (dss_iface_ioctl_phys_link_state_type)from;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSPhyslinkState(const dss_iface_ioctl_phys_link_state_type from,
                                             PhysLinkStateType *to)
{
  *to = (unsigned int)from;
   return AEE_SUCCESS;
}

#ifdef FEATURE_BCMCS

AEEResult DSSConversion::DS2IDSUpdateParamBCMCSDB(const dss_iface_ioctl_bcmcs_db_update_type* dsUpdate,
                                                  ds::Net::BCMCSDBSpecType* idsUpdate)
{
   if (NULL == dsUpdate || NULL == idsUpdate)
   {
      LOG_MSG_ERROR ("Error in DS2IDSUpdateParamBCMCSDB conversion", 0, 0, 0); 
      ASSERT (0);
      return QDS_EFAULT;
   }
   
   memset(&idsUpdate->zone, 0, sizeof(BCMCSZoneType));
   memset(&idsUpdate->multicastAddr, 0, sizeof(ds::IPAddrType));
   switch (dsUpdate->zone.type) {
      case DSBCMCS_ZONE_1X:
         idsUpdate->zone.type = BCMCSZone::QDS_CDMA1X;
         break;
      case DSBCMCS_ZONE_HDR:
         idsUpdate->zone.type = BCMCSZone::QDS_HDR;
         break;
      default:
         return QDS_EFAULT;
   }
   (void)memcpy ((void*)(idsUpdate->zone.subnet), (const void*)dsUpdate->zone.zone.subnet, BCMCS_HDR_ADDR_LEN);
   IDS_ERR_RET (DS2IDSIpAddr(&dsUpdate->multicast_ip, &idsUpdate->multicastAddr));
   
   idsUpdate->programID = dsUpdate->program_id;
   idsUpdate->programIDLength = dsUpdate->program_id_len;
   idsUpdate->flowDiscriminatorLength = dsUpdate->flow_discrim_len;
   idsUpdate->flowDiscriminator = dsUpdate->flow_discrim;
   idsUpdate->BCMCSPort = dsUpdate->port;
   
   switch(dsUpdate->framing)
   {
      case DSBCMCS_FRAMING_NOT_SET:
         idsUpdate->framing = BCMCSFraming::FRMV_NOT_SET;
         break;

      case DSBCMCS_FRAMING_SEGMENT:
         idsUpdate->framing = BCMCSFraming::FRMV_SEGMENT;
         break;

      case DSBCMCS_FRAMING_HDLC:
         idsUpdate->framing = BCMCSFraming::FRMV_HDLC;
         break;

      default:
         return QDS_EFAULT;
   }

   switch(dsUpdate->protocol)
   {
      case DSBCMCS_PROTOCOL_NOT_SET:
         idsUpdate->protocol = BCMCSProtocol::PRTV_NOT_SET;
         break;

      case DSBCMCS_PROTOCOL_PPP:
        idsUpdate->protocol = BCMCSProtocol::PRTV_PPP;
         break;

      case DSBCMCS_PROTOCOL_IPv4:
         idsUpdate->protocol = BCMCSProtocol::PRTV_IPV4;
         break;

      case DSBCMCS_PROTOCOL_IPv6:
         idsUpdate->protocol = BCMCSProtocol::PRTV_IPV6;
         break;

      default:
         return QDS_EFAULT;
   }

   idsUpdate->protocol = dsUpdate->protocol;
   idsUpdate->crcLength = dsUpdate->crc_len;
   idsUpdate->flowFormat = dsUpdate->flow_format;
   idsUpdate->flowID = dsUpdate->flow_id;
   idsUpdate->flowIDLen = dsUpdate->flow_id_len;
   idsUpdate->overwrite = dsUpdate->overwrite;
   idsUpdate->flowPriority = dsUpdate->flow_priority;
   
   return AEE_SUCCESS;
}

#endif // FEATURE_BCMCS

AEEResult DSSConversion::IDS2DSRFCondition(RFConditionType idsCond, ps_iface_rf_conditions_enum_type* pdsCond)
{
   *pdsCond = (ps_iface_rf_conditions_enum_type)idsCond;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSSessionTimerSelect(dss_session_timer_select_enum_type dsSelect, Network1xPrivSessionTimerSelectType* idsSelect)
{
   *idsSelect = (Network1xPrivSessionTimerSelectType)dsSelect;
   return AEE_SUCCESS;
}

int DSSConversion::DS2IDSSDBFlags(uint32 dsFlags)
{
   int idsFlags = 0;
   if (0 != (dsFlags & MSG_EXPEDITE)) {
      idsFlags |= ds::Sock::SendFlags::QDS_MSG_EXPEDITE;
   }
   if (0 != (dsFlags & MSG_FAST_EXPEDITE)) {
      idsFlags |= ds::Sock::SendFlags::QDS_MSG_FAST_EXPEDITE;
   }
   if (0 != (dsFlags & MSG_WAKEUP)) {
      idsFlags |= ds::Sock::SendFlags::QDS_MSG_SOCK_WAKEUP;
   }
   return idsFlags;
}

AEEResult DSSConversion::DS2IDSQoSSpec(dss_iface_ioctl_qos_request_type* reqType,
                                       QoSSpecType* specType,
                                       boolean isModify,
                                       QoSModifyMaskType *modifyMask)
{
   int i;
   AEEResult nRet = AEE_SUCCESS;
   boolean bIgnoreFilters = FALSE;
   ds::Net::INetworkFactoryPriv *pNetworkFactoryPriv = NULL;

   nRet = (TRUE == isModify)? DSSInternalValidateQoSModifyRequest(reqType) : DSSInternalValidateQoSRequest(reqType);
   if (AEE_SUCCESS != nRet) {
      LOG_MSG_ERROR ("QoS Spec validation failed, res %d", nRet, 0, 0);
      return nRet;
   }

   if (NULL == modifyMask && isModify){
      LOG_MSG_ERROR ("Invalid QoS ModifyMask", 0, 0, 0);
      ASSERT(0);
   }

   if (isModify){
      IDS_ERR_RET(DS2IDSQoSMask(reqType->qos.field_mask, modifyMask));
   }

   specType->rxFilter = NULL;
   specType->txFilter = NULL;

   if( (qos_spec_field_mask_type)QOS_MASK_RX_FLOW & reqType->qos.field_mask ){

      //rx flows conversion from ds to IDS
      BAIL_ERR(DS2IDSQoSSpecFlows(&(reqType->qos.rx.flow_template),
                                     &(specType->rxFlows),
                                     &specType->rxFlowsLen,
                                     &specType->rxFlowsLenReq,
                                     &specType->rxMinFlow,
                                     reqType->qos.field_mask,
                                     false));

      bIgnoreFilters = FALSE ; 
      // We are deleting the flow , filters are not important
      if ( TRUE == isModify && 
         (ipflow_field_mask_type)IPFLOW_MASK_NONE == reqType->qos.rx.flow_template.req_flow.field_mask ) {
         bIgnoreFilters = TRUE ;
         if (NULL != modifyMask){
            *modifyMask |= QoSModifyMask::QDS_RX_SPEC_DELETE;
         }
      }

      //0 Len filters are allowed for modify (basically while deleting a flow.)
      if (!bIgnoreFilters)
      {

        // allocate an array of pointers to rx filters
        specType->rxFilterLen = reqType->qos.rx.fltr_template.num_filters;

        if (0 != specType->rxFilterLen ) {
           specType->rxFilter = (IIPFilterPriv**)ps_system_heap_mem_alloc(sizeof(IIPFilterPriv*)*(specType->rxFilterLen));
           if (NULL == specType->rxFilter) {
              LOG_MSG_ERROR("Can't allocate rxFilter", 0, 0, 0);
              nRet = AEE_ENOMEMORY;
              goto bail;
           }
        }


        for (i=0 ; i<reqType->qos.rx.fltr_template.num_filters; i++) {
           specType->rxFilter[i] = NULL;
        }

        
        // Get the NetworkFactoryPriv from DSS_Globals
        nRet = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);
        if(AEE_SUCCESS != nRet){
          goto bail;
        }

        DSSGenScope scopeNetworkFactory(pNetworkFactoryPriv,DSSGenScope::IDSIQI_TYPE); 

        for (i=0 ; i<reqType->qos.rx.fltr_template.num_filters; i++)
        {
            // allocate a new DSNetIPFilter.
           nRet = pNetworkFactoryPriv->CreateIPFilterSpec(reinterpret_cast<IIPFilterPriv**>(&(specType->rxFilter[i])));
           if (AEE_SUCCESS != nRet) {
              LOG_MSG_ERROR("Can't create NetIPFilterSpec", 0, 0, 0);
              goto bail;
           }

          // convert old filter to new
          BAIL_ERR(DS2IDSIPFilter( &(reqType->qos.rx.fltr_template.list_ptr[i]), specType->rxFilter[i]));
        }
      }
   }

   if( (qos_spec_field_mask_type)QOS_MASK_TX_FLOW & reqType->qos.field_mask ){
      //tx flows conversion from ds to IDS
      BAIL_ERR(DS2IDSQoSSpecFlows(&(reqType->qos.tx.flow_template),
                                      &(specType->txFlows),
                                      &specType->txFlowsLen,
                                      &specType->txFlowsLenReq,
                                      &specType->txMinFlow,
                                      reqType->qos.field_mask,
                                      true));


      bIgnoreFilters = FALSE ; 
      // We are deleting the flow , filters are not important
      if ( TRUE == isModify && 
         (ipflow_field_mask_type)IPFLOW_MASK_NONE == reqType->qos.tx.flow_template.req_flow.field_mask ) {
         bIgnoreFilters = TRUE ;
         if (NULL != modifyMask){
            *modifyMask |= QoSModifyMask::QDS_TX_SPEC_DELETE;
         }
      }

      //0 Len filters are allowed for modify (basically while deleting a flow.)
      if (!bIgnoreFilters)
      {

         // allocate an array of pointers to tx filters
         specType->txFilterLen = reqType->qos.tx.fltr_template.num_filters;
         specType->txFilter = (IIPFilterPriv**)ps_system_heap_mem_alloc(sizeof(IIPFilterPriv*)*(specType->txFilterLen));
         if (0 != specType->txFilterLen) {
            if (NULL == specType->txFilter) {
               LOG_MSG_ERROR("Can't allocate txFilter", 0, 0, 0);
               goto bail;
            }
         }

         // Get the NetworkFactory from DSSGlobals
         if (NULL == pNetworkFactoryPriv) {
            nRet = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);
            
            if(AEE_SUCCESS != nRet){
               goto bail;
            }
         }

         DSSGenScope scopeNetworkFactory(pNetworkFactoryPriv,DSSGenScope::IDSIQI_TYPE); 

         for (i=0 ; i<reqType->qos.tx.fltr_template.num_filters; i++) 
         {
            specType->txFilter[i] = NULL;
         }

         for (i=0 ; i<reqType->qos.tx.fltr_template.num_filters; i++) 
         {
            // allocate a new DSNetIPFilter.
            nRet = pNetworkFactoryPriv->CreateIPFilterSpec(reinterpret_cast<IIPFilterPriv**>(&(specType->txFilter[i])));
            if (AEE_SUCCESS != nRet) {
               LOG_MSG_ERROR("Can't create NetIPFilterSpec", 0, 0, 0);
               goto bail;
            }
 
            // convert old filter to new
            BAIL_ERR(DS2IDSIPFilter(&(reqType->qos.tx.fltr_template.list_ptr[i]), specType->txFilter[i]));
         }

      } /* if (0 != specType->txFilterLen || FALSE == isModify) */
   }



   if( (qos_spec_field_mask_type)QOS_MODIFY_MASK_TX_FLTR_MODIFY & reqType->qos.field_mask ) {
      // allocate an array of pointers to tx filters
      specType->txFilterLen = reqType->qos.tx.fltr_template.num_filters;
      specType->txFilter = (IIPFilterPriv**)ps_system_heap_mem_alloc(sizeof(IIPFilterPriv*)*(specType->txFilterLen));
      if (0 != specType->txFilterLen) {
        if (NULL == specType->txFilter) {
          LOG_MSG_ERROR("Can't allocate txFilter", 0, 0, 0);
          goto bail;
        }
      }

      // Get the NetworkFactory from DSSGlobals
      if (NULL == pNetworkFactoryPriv) {
         nRet = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);
         if(AEE_SUCCESS != nRet){
            goto bail;
         }
      }

      DSSGenScope scopeNetworkFactory(pNetworkFactoryPriv,DSSGenScope::IDSIQI_TYPE); 
      for (i=0 ; i<reqType->qos.tx.fltr_template.num_filters; i++) 
      {
         specType->txFilter[i] = NULL;
      }

      for (i=0 ; i<reqType->qos.tx.fltr_template.num_filters; i++) 
      {
         // allocate a new DSNetIPFilter.
         nRet = pNetworkFactoryPriv->CreateIPFilterSpec(reinterpret_cast<IIPFilterPriv**>(&(specType->txFilter[i])));
         if (AEE_SUCCESS != nRet) {
            LOG_MSG_ERROR("Can't create NetIPFilterSpec", 0, 0, 0);
            goto bail;
         }
         // convert old filter to new
         BAIL_ERR(DS2IDSIPFilter(reqType->qos.tx.fltr_template.list_ptr, specType->txFilter[i]));
      }
   }

   if( (qos_spec_field_mask_type)QOS_MODIFY_MASK_RX_FLTR_MODIFY & reqType->qos.field_mask ) {
      // allocate an array of pointers to rx filters
      specType->rxFilterLen = reqType->qos.rx.fltr_template.num_filters;
      specType->rxFilter = (IIPFilterPriv**)ps_system_heap_mem_alloc(sizeof(IIPFilterPriv*)*(specType->rxFilterLen));
      if (0 != specType->rxFilterLen) {
         if (NULL == specType->rxFilter) {
            LOG_MSG_ERROR("Can't allocate rxFilter", 0, 0, 0);
            goto bail;
         }
      }

      // Get the NetworkFactory from DSSGlobals
      if (NULL == pNetworkFactoryPriv) {
         nRet = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);
         if(AEE_SUCCESS != nRet){
            goto bail;
         }
      }

      DSSGenScope scopeNetworkFactory(pNetworkFactoryPriv,DSSGenScope::IDSIQI_TYPE); 
      for (i=0 ; i<reqType->qos.rx.fltr_template.num_filters; i++) 
      {
         specType->rxFilter[i] = NULL;
      }

      for (i=0 ; i<reqType->qos.rx.fltr_template.num_filters; i++) 
      {
         // allocate a new DSNetIPFilter.
         nRet = pNetworkFactoryPriv->CreateIPFilterSpec(reinterpret_cast<IIPFilterPriv**>(&(specType->rxFilter[i])));
         if (AEE_SUCCESS != nRet) {
            LOG_MSG_ERROR("Can't create NetIPFilterSpec", 0, 0, 0);
            goto bail;
         }
         // convert old filter to new
         BAIL_ERR(DS2IDSIPFilter(reqType->qos.rx.fltr_template.list_ptr, specType->rxFilter[i]));
      }
    }

   return AEE_SUCCESS;

bail:

   FreeQoSMemOnError(specType);
   return nRet;
}




AEEResult DSSConversion::DS2IDSQoSSpecPrimary(dss_iface_ioctl_primary_qos_modify_type* reqType, 
                                              ds::Net::QoSSpecPrimaryType* specType,
                                              boolean isModify,
                                              ds::Net::QoSModifyMaskType *modifyMask)
{
   AEEResult res;
   AEEResult nRet = AEE_SUCCESS;

   res = DSSInternalValidatePrimaryQoSModifySpec(reqType, isModify);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR ("QoS Spec validation failed, res %d", res, 0, 0);
      return res;
   }

   if (NULL == modifyMask && isModify){
      LOG_MSG_ERROR ("Invalid QoS ModifyMask", 0, 0, 0);
      ASSERT(0);
   }

   if (isModify){
      IDS_ERR_RET(DS2IDSQoSMask(reqType->primary_qos_spec.field_mask, modifyMask));
   }

   if( (qos_spec_field_mask_type)QOS_MASK_RX_FLOW & reqType->primary_qos_spec.field_mask ){

      //rx flows conversion from ds to IDS
      BAIL_ERR(DS2IDSQoSSpecFlows(&(reqType->primary_qos_spec.rx_flow_template),
         &(specType->rxFlows),
         &specType->rxFlowsLen,
         &specType->rxFlowsLenReq,
         &specType->rxMinFlow,
         reqType->primary_qos_spec.field_mask,
         false));
 
      // We are deleting the flow , filters are not important
      if ( TRUE == isModify && 
         (ipflow_field_mask_type)IPFLOW_MASK_NONE == reqType->primary_qos_spec.rx_flow_template.req_flow.field_mask) {
            *modifyMask |= QoSModifyMask::QDS_RX_SPEC_DELETE;
         }
   }

   if( (qos_spec_field_mask_type)QOS_MASK_TX_FLOW & reqType->primary_qos_spec.field_mask ){
      //tx flows conversion from ds to IDS
      BAIL_ERR(DS2IDSQoSSpecFlows(&(reqType->primary_qos_spec.tx_flow_template),
         &(specType->txFlows),
         &specType->txFlowsLen,
         &specType->txFlowsLenReq,
         &specType->txMinFlow,
         reqType->primary_qos_spec.field_mask,
         true));

      // We are deleting the flow , filters are not important
      if ( TRUE == isModify && 
         (ipflow_field_mask_type)IPFLOW_MASK_NONE == reqType->primary_qos_spec.tx_flow_template.req_flow.field_mask) {
            *modifyMask |= QoSModifyMask::QDS_TX_SPEC_DELETE;
         }
   }

   return AEE_SUCCESS;


bail:

   if (NULL != specType->rxFlows) {
      for (int i=0; i < specType->rxFlowsLen; ++i) {
         if (NULL != (specType->rxFlows)[i]) {
            DSSCommon::ReleaseIf((IQI**)& ((specType->rxFlows)[i]) );
         }
      }
      PS_SYSTEM_HEAP_MEM_FREE (specType->rxFlows);
   }

   if (NULL != specType->txFlows) {
      for (int i=0; i < specType->txFlowsLen; ++i) {
         if (NULL != (specType->txFlows)[i]) {
            DSSCommon::ReleaseIf((IQI**)& ((specType->txFlows)[i]) );
         }
      }
      PS_SYSTEM_HEAP_MEM_FREE (specType->txFlows);
   }

   return nRet;
}

AEEResult DSSConversion::IDS2DSQoSSpecErrMask(QoSSpecType* specType,dss_iface_ioctl_qos_request_type* reqType)
{

   // let's convert the rx flows
   IDS_ERR_RET(IDS2DSQoSSpecFlowsErrMask(specType->rxFlows,specType->rxMinFlow,specType->rxFlowsLen,&(reqType->qos.rx.flow_template)));

   // let's convert the tx flows
   IDS_ERR_RET(IDS2DSQoSSpecFlowsErrMask(specType->txFlows,specType->txMinFlow,specType->txFlowsLen,&(reqType->qos.tx.flow_template)));

   // let's convert the rx filters
   IDS_ERR_RET(IDS2DSQoSSpecFiltersErrMask(specType->rxFilter,specType->rxFilterLen,&(reqType->qos.rx.fltr_template)));
   // let's convert the tx filters
   IDS_ERR_RET(IDS2DSQoSSpecFiltersErrMask(specType->txFilter,specType->txFilterLen,&(reqType->qos.tx.fltr_template)));

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSSpecPrimaryErrMask(ds::Net::QoSSpecPrimaryType* specType, dss_iface_ioctl_primary_qos_modify_type* reqType)
{
   // convert the rx flows
   IDS_ERR_RET(IDS2DSQoSSpecFlowsErrMask(specType->rxFlows,specType->rxMinFlow,specType->rxFlowsLen,&(reqType->primary_qos_spec.rx_flow_template)));
   // convert the tx flows
   IDS_ERR_RET(IDS2DSQoSSpecFlowsErrMask(specType->txFlows,specType->txMinFlow,specType->txFlowsLen,&(reqType->primary_qos_spec.tx_flow_template)));

   return AEE_SUCCESS;
}
AEEResult DSSConversion::IDS2DSQoSSpecFlowsErrMask(IQoSFlowPriv** ppIDSNetQoSFlow,boolean min,int len,ip_flow_spec_type* flow_spec)
{
   int numAuxFlows;

   if (NULL == ppIDSNetQoSFlow) {
      return AEE_SUCCESS;
   }

   IDS_ERR_RET(IDS2DSQoSSpecFlowErrMask(ppIDSNetQoSFlow[0], &(flow_spec->req_flow)));

   numAuxFlows = (min) ? len - 2 : len - 1;
   // if there are auxiliary flows , convert them
   if (numAuxFlows > 0) {
      if (NULL == flow_spec->aux_flow_list_ptr) {
         LOG_MSG_ERROR ("aux_flow_list_ptr is NULL", 0, 0, 0); 
         ASSERT (0);
      }
      
      for (int i = 0; i < numAuxFlows; i++) {
         if (NULL == ppIDSNetQoSFlow[i + 1]) {
            LOG_MSG_ERROR ("flow number %i is not allocated in ppIDSNetQoSFlow", i + 1, 0, 0); 
            ASSERT (0);
         }
         
         IDS_ERR_RET(IDS2DSQoSSpecFlowErrMask(ppIDSNetQoSFlow[i + 1], &(flow_spec->aux_flow_list_ptr[i])));
      }
   }

   if (min) {
      IDS_ERR_RET(IDS2DSQoSSpecFlowErrMask(ppIDSNetQoSFlow[len-1], &(flow_spec->min_req_flow)));
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSSpecFiltersErrMask(IIPFilterPriv** ppIDSNetIPFilter,int len,ip_filter_spec_type* filter_spec)
   {
   if (NULL == ppIDSNetIPFilter || NULL == filter_spec->list_ptr){
      return AEE_SUCCESS;
   }

   for (int i=0;i<len;i++) {
      if (NULL == ppIDSNetIPFilter[i]) {
         LOG_MSG_ERROR ("filter number %i is not allocated in ppIDSNetIPFilter", i, 0, 0); 
         ASSERT (0);
      }
      
      IDS_ERR_RET(IDS2DSQoSSpecFilterErrMask(ppIDSNetIPFilter[i],&(filter_spec->list_ptr[i])));
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSSpecFlowErrMask(IQoSFlowPriv* pIDSNetQoSFlow,ip_flow_type* flow)
{
   int num;
   int num2;
   QoSFlowOptIDType* errors = NULL;
   IDS_ERR_RET(pIDSNetQoSFlow->GetErroneousOptions(NULL,0,&num));
   
   if (num == 0){
      return AEE_SUCCESS;
   }

   errors = (QoSFlowOptIDType*)ps_system_heap_mem_alloc(sizeof(QoSFlowOptIDType)*num);
   if (NULL == errors) {
      LOG_MSG_ERROR("Can't allocate array for erroneous options ", 0, 0, 0);
      return AEE_ENOMEMORY;
   }
   IDS_ERR_RET(pIDSNetQoSFlow->GetErroneousOptions(errors,num,&num2));

   flow->err_mask = (ipflow_field_mask_type)IPFLOW_MASK_NONE;
   for (int i=0;i<num;i++) {

      // in case of DATA_RATE_MIN_MAX we set the IPFLOW_MASK_DATA_RATE flow
      if (QoSFlowOptID::QDS_DATA_RATE_MIN_MAX == errors[i]){
         flow->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_DATA_RATE;
      }
      else{
         flow->err_mask |= errors[i];
      }
   }
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSSpecFilterErrMask(IIPFilterPriv* pIDSNetIPFilter,ip_filter_type* filter)
{
   int num;
   int num2;
   unsigned char ipVsn;
   IPFilterIDType* errors = NULL;
   IDS_ERR_RET(pIDSNetIPFilter->GetErroneousOptions(NULL,0,&num));

   if (0 == num) {
      return AEE_SUCCESS;   
   }

   errors = (IPFilterIDType*)ps_system_heap_mem_alloc(sizeof(IPFilterIDType)*num);
   if (NULL == errors) {
      LOG_MSG_ERROR("Can't allocate array for erroneous options ", 0, 0, 0);
      return AEE_ENOMEMORY;
   }
   IDS_ERR_RET(pIDSNetIPFilter->GetErroneousOptions(errors,num,&num2));

   filter->ip_hdr.v4.err_mask = 0; 
   filter->ip_hdr.v6.err_mask = 0;
   filter->next_prot_hdr.tcp.err_mask = 0;
   filter->next_prot_hdr.udp.err_mask = 0;
   filter->next_prot_hdr.esp.err_mask = 0;
   filter->next_prot_hdr.icmp.err_mask = 0;

   bool isIpv6 = FALSE;
   (void)pIDSNetIPFilter->GetIPVsn (&ipVsn);

   if ((unsigned char)IP_V6 == ipVsn) {
      isIpv6 = TRUE;
   } else if ((unsigned char)IP_V4 != ipVsn) {
      LOG_MSG_ERROR("Wrong IP version error in conversion", 0, 0, 0);
      return QDS_EFAULT;
   }

   for (int i=0;i<num;i++) {
      switch(errors[i]) {
         case (IPFilterID::QDS_NEXT_HDR_PROTO):
            if (isIpv6) {
               filter->ip_hdr.v6.err_mask |= (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP4_NEXT_HDR_PROT;
            } else {
               filter->ip_hdr.v4.err_mask |= (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP6_NEXT_HDR_PROT;
            }
            break;

         case (IPFilterID::QDS_SRC_ADDR):
            if (isIpv6) {
               filter->ip_hdr.v6.err_mask |= (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP4_SRC_ADDR;
            } else {
               filter->ip_hdr.v4.err_mask |= (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP6_SRC_ADDR;
            }
            break;

         case (IPFilterID::QDS_DST_ADDR):
            if (isIpv6) {
               filter->ip_hdr.v6.err_mask |= (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP4_DST_ADDR;
            } else {
               filter->ip_hdr.v4.err_mask |= (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP6_DST_ADDR;
            }
            break;

         case(IPFilterID::QDS_IPV4_TOS):
            filter->ip_hdr.v4.err_mask |= (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_TOS;
            break;

         case (IPFilterID::QDS_IPV6_TRF_CLASS):
            filter->ip_hdr.v6.err_mask |= (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_TRAFFIC_CLASS;
            break;

         case(IPFilterID::QDS_IPV6_FLOW_LABEL):
            filter->ip_hdr.v6.err_mask |= (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_FLOW_LABEL;
            break;

         case (IPFilterID::QDS_TCP_SRC_PORT):
            filter->next_prot_hdr.tcp.err_mask |= (ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_TCP_SRC_PORT;
            break;

         case (IPFilterID::QDS_TCP_DST_PORT):
            filter->next_prot_hdr.tcp.err_mask |= (ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_TCP_DST_PORT;
            break;

         case (IPFilterID::QDS_UDP_SRC_PORT):
            filter->next_prot_hdr.udp.err_mask |= (ipfltr_udp_hdr_field_mask_type)IPFLTR_MASK_UDP_SRC_PORT;
            break;

         case (IPFilterID::QDS_UDP_DST_PORT):
            filter->next_prot_hdr.udp.err_mask |= (ipfltr_udp_hdr_field_mask_type)IPFLTR_MASK_UDP_DST_PORT;
            break;

         case (IPFilterID::QDS_ESP_SPI):
            filter->next_prot_hdr.esp.err_mask |= (ipfltr_esp_hdr_field_mask_type)IPFLTR_MASK_ESP_SPI;
            break;

         case(IPFilterID::QDS_ICMP_MTYPE):
            filter->next_prot_hdr.icmp.err_mask |= (ipfltr_icmp_hdr_field_mask_type)IPFLTR_MASK_ICMP_MSG_TYPE;
            break;

         case(IPFilterID::QDS_ICMP_MCODE):
            filter->next_prot_hdr.icmp.err_mask |= (ipfltr_icmp_hdr_field_mask_type)IPFLTR_MASK_ICMP_MSG_CODE;
            break;
         default:
             return QDS_EFAULT;
      }

   }
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSQoSSpecBundle(dss_iface_ioctl_qos_request_ex_type* qos_request_ex, SeqQoSSpecType requestedQoSSpecs)
{
   dss_iface_ioctl_qos_request_type tmpReqType;
   AEEResult nRet = AEE_SUCCESS;

   // init tmpReqType fields that are common to all the qos requests in qos_request_ex
   tmpReqType.app_id = qos_request_ex->app_id;
   tmpReqType.cback_fn = qos_request_ex->cback_fn;
   tmpReqType.user_data = qos_request_ex->user_data;

   for (int i=0 ; i<qos_request_ex->num_qos_specs; i++ )
   {
      // extract one qos_request from qos_request_ex
      memcpy(&(tmpReqType.qos), &(qos_request_ex->qos_specs_ptr[i]), sizeof(tmpReqType.qos));

      // convert from old qos spec to new
      // QoS Modify is not supported in QoS Bundle, thus we ignore the modifyMask parameter
      BAIL_ERR(DSSConversion::DS2IDSQoSSpec(&tmpReqType, &requestedQoSSpecs.data[i], FALSE, NULL));
   }

   return AEE_SUCCESS;

bail:
   if (NULL != requestedQoSSpecs.data) {
      for (int i=0 ; i<qos_request_ex->num_qos_specs; i++ ) { 
        FreeQoSMemOnError(&(requestedQoSSpecs.data[i]));
      }
   }

   return nRet;

}

AEEResult DSSConversion::IDS2DSQoSSpecBundleErrMask(SeqQoSSpecType* requestedQoSSpecs,dss_iface_ioctl_qos_request_ex_type* qos_request_ex)
{
   dss_iface_ioctl_qos_request_type tmpReqType;

   for (int i=0 ; i<qos_request_ex->num_qos_specs; i++ )
   {
      // extract one qos_request from qos_request_ex
      memcpy(&(tmpReqType.qos), &(qos_request_ex->qos_specs_ptr[i]), sizeof(tmpReqType.qos));

      // convert from old qos spec to new
      IDS_ERR_RET(DSSConversion::IDS2DSQoSSpecErrMask(&(requestedQoSSpecs->data[i]),&tmpReqType));

      memcpy(&(qos_request_ex->qos_specs_ptr[i]),&(tmpReqType.qos),sizeof(tmpReqType.qos));
   }

   return AEE_SUCCESS;
}

/* DSSConversion::IDS2DSQoSSpec is not used , therefor was removed from code */
/* DSSConversion::IDS2DSIPFilter is not used , therefor was removed from code */

AEEResult DSSConversion::DS2IDSIPFilter(ip_filter_type* ipFilterType, IIPFilterPriv* NetIPFilter)
{
   IPFilterIPNextProtocolType NextHdrProt = 0;
   AEEResult res = AEE_SUCCESS;
   IIPFilterPriv* pIDSNetIPFilterPriv = NULL;

   // this variable shall determine whether the qos request should be failed
   // basically we should abort the qos request when there are severe errors in qos spec preventing us from correctly converting it
   // to the ds format. If there are errors not preventing the conversion, the request should still be issued to discover the additional ones
   bool abort_qos_request = false;

   IPFilterIPVersionType IPVsn = 0;
   uint8 field_mask = 0;
   uint8 next_hdr_prot = 0;
   switch(ipFilterType->ip_vsn){
      case IP_V4:
        IPVsn = IPFilterIPVersion::QDS_FILTER_IPV4;
        field_mask = ipFilterType->ip_hdr.v4.field_mask;
        next_hdr_prot = ipFilterType->ip_hdr.v4.next_hdr_prot;
        break;
      case IP_V6:
        IPVsn = IPFilterIPVersion::QDS_FILTER_IPV6;
        field_mask = ipFilterType->ip_hdr.v6.field_mask;
        next_hdr_prot = ipFilterType->ip_hdr.v6.next_hdr_prot;
        break;
      default:
         return QDS_EFAULT;
   }

   (void) NetIPFilter->SetIPVsn(IPVsn);

   if (((IP_V4 == ipFilterType->ip_vsn) && ((uint8)IPFLTR_MASK_IP4_NEXT_HDR_PROT & field_mask)) ||
      ((IP_V6 == ipFilterType->ip_vsn) && ((uint8)IPFLTR_MASK_IP6_NEXT_HDR_PROT & field_mask))  ) {

      // set the default value
      NextHdrProt = next_hdr_prot;

      switch( next_hdr_prot ){

         case PS_IPPROTO_TCP:
            NextHdrProt = IPFilterIPNextProtocol::QDS_FILTER_TCP;
            (void) NetIPFilter->SetNextHdrProt(NextHdrProt);

            if (ipFilterType->next_prot_hdr.tcp.field_mask & ~(ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_TCP_ALL)
            {
               ipFilterType->next_prot_hdr.tcp.err_mask =
                 (ipFilterType->next_prot_hdr.tcp.field_mask & ~(ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_TCP_ALL);
               abort_qos_request = true;
               break;
            }

            if((ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_TCP_SRC_PORT & ipFilterType->next_prot_hdr.tcp.field_mask) {
               IPFilterPortType SrcPort;
               memcpy( SrcPort.port, &(ipFilterType->next_prot_hdr.tcp.src.port), sizeof(ds::Net::INPortType) );
               SrcPort.range = (unsigned short)ipFilterType->next_prot_hdr.tcp.src.range;
               (void) NetIPFilter->SetSrcPort(&SrcPort);
            }
            if((ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_TCP_DST_PORT & ipFilterType->next_prot_hdr.tcp.field_mask) {
               IPFilterPortType DstPort;
               memcpy( DstPort.port, &(ipFilterType->next_prot_hdr.tcp.dst.port), sizeof(ds::Net::INPortType) );
               DstPort.range = (unsigned short)ipFilterType->next_prot_hdr.tcp.dst.range;
               (void) NetIPFilter->SetDstPort(&DstPort);
            }
            break;

         case PS_IPPROTO_UDP:
            NextHdrProt = IPFilterIPNextProtocol::QDS_FILTER_UDP;
            (void) NetIPFilter->SetNextHdrProt(NextHdrProt);

            if (ipFilterType->next_prot_hdr.udp.field_mask & ~(ipfltr_udp_hdr_field_mask_type)IPFLTR_MASK_UDP_ALL)
            {
               ipFilterType->next_prot_hdr.udp.err_mask =
                 (ipFilterType->next_prot_hdr.udp.field_mask & ~(ipfltr_udp_hdr_field_mask_type)IPFLTR_MASK_UDP_ALL);
               abort_qos_request = true;
               break;
            }

            if((ipfltr_tcp_hdr_field_mask_type)IPFLTR_MASK_UDP_SRC_PORT & ipFilterType->next_prot_hdr.udp.field_mask) {
               IPFilterPortType SrcPort;
               memcpy( SrcPort.port, &(ipFilterType->next_prot_hdr.udp.src.port), sizeof(ds::Net::INPortType) );
               SrcPort.range = (unsigned short)ipFilterType->next_prot_hdr.udp.src.range;
               (void) NetIPFilter->SetSrcPort(&SrcPort);
            }
            if((ipfltr_udp_hdr_field_mask_type)IPFLTR_MASK_UDP_DST_PORT & ipFilterType->next_prot_hdr.udp.field_mask) {
               IPFilterPortType DstPort;
               memcpy( DstPort.port, &(ipFilterType->next_prot_hdr.udp.dst.port), sizeof(ds::Net::INPortType) );
               DstPort.range = (unsigned short)ipFilterType->next_prot_hdr.udp.dst.range;
               (void) NetIPFilter->SetDstPort(&DstPort);
            }
            break;
         case PS_IPPROTO_ICMP:
            NextHdrProt = IPFilterIPNextProtocol::QDS_FILTER_ICMP;
            (void) NetIPFilter->SetNextHdrProt(NextHdrProt);

            if (ipFilterType->next_prot_hdr.icmp.field_mask & ~(ipfltr_icmp_hdr_field_mask_type)IPFLTR_MASK_ICMP_ALL)
            {
               ipFilterType->next_prot_hdr.icmp.err_mask =
                 (ipFilterType->next_prot_hdr.icmp.field_mask & ~(ipfltr_icmp_hdr_field_mask_type)IPFLTR_MASK_ICMP_ALL);
               abort_qos_request = true;
               break;
            }

            res = NetIPFilter->QueryInterface(AEEIID_IIPFilterPriv, (void**)&pIDSNetIPFilterPriv);
            if (AEE_SUCCESS != res) {
               LOG_MSG_ERROR("QueryInterface(AEEIID_IDSNetIPFilterPriv) failed: %d", res, 0, 0);
               abort_qos_request = true;
               break;
            }
            if ((ipfltr_icmp_hdr_field_mask_type)IPFLTR_MASK_ICMP_MSG_TYPE & ipFilterType->next_prot_hdr.icmp.field_mask) {
               (void) pIDSNetIPFilterPriv->SetICMPType(ipFilterType->next_prot_hdr.icmp.type);
            }
            if ((ipfltr_icmp_hdr_field_mask_type)IPFLTR_MASK_ICMP_MSG_CODE & ipFilterType->next_prot_hdr.icmp.field_mask) {
               (void) pIDSNetIPFilterPriv->SetICMPCode(ipFilterType->next_prot_hdr.icmp.code);
            }
            DSSCommon::ReleaseIf((IQI**)&pIDSNetIPFilterPriv);
            break;

         case PS_IPPROTO_ESP:
            NextHdrProt = IPFilterIPNextProtocol::QDS_FILTER_ESP;
            if (ipFilterType->next_prot_hdr.esp.field_mask & ~(ipfltr_esp_hdr_field_mask_type)IPFLTR_MASK_ESP_ALL)
            {
               ipFilterType->next_prot_hdr.esp.err_mask =
                 (ipFilterType->next_prot_hdr.esp.field_mask & ~(ipfltr_esp_hdr_field_mask_type)IPFLTR_MASK_ESP_ALL);
               abort_qos_request = true;
            }
            (void) NetIPFilter->SetEspSpi(ipFilterType->next_prot_hdr.esp.spi);
            
            break;
         default:
            /* Unsupported protocol */
            if (IP_V4 == ipFilterType->ip_vsn)
            {
               ipFilterType->ip_hdr.v4.err_mask |= (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_NEXT_HDR_PROT;
               //is_spec_valid = false;
            }
            else
            {
               ipFilterType->ip_hdr.v6.err_mask |= (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_NEXT_HDR_PROT;
               //is_spec_valid = false;
            }
            abort_qos_request = true;
            break;

      } /*switch */
   } /* if */


   // IPV4
   if (IP_V4 == ipFilterType->ip_vsn) {

      if (0 == ipFilterType->ip_hdr.v4.field_mask)
      {
         ipFilterType->ip_hdr.v4.err_mask = (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_ALL;
         abort_qos_request = true;
         goto bail;
      }

      if (ipFilterType->ip_hdr.v4.field_mask & ~(ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_ALL)
      {
         ipFilterType->ip_hdr.v4.err_mask =
            (ipFilterType->ip_hdr.v4.field_mask & ~(ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_ALL);
         abort_qos_request = true;
         goto bail;
      }

      if( (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_SRC_ADDR & ipFilterType->ip_hdr.v4.field_mask ){
            IPFilterIPv4AddrType SrcV4;

            memcpy( SrcV4.addr, &(ipFilterType->ip_hdr.v4.src.addr), sizeof(ds::Net::INAddrType) );
            memcpy( SrcV4.subnetMask, &(ipFilterType->ip_hdr.v4.src.subnet_mask), sizeof(ds::Net::INAddrType) );
            (void) NetIPFilter->SetSrcV4(&SrcV4);
      }



      if( (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_DST_ADDR & ipFilterType->ip_hdr.v4.field_mask ){
            IPFilterIPv4AddrType DstV4; //done
            memcpy( DstV4.addr, &(ipFilterType->ip_hdr.v4.dst.addr), sizeof(ds::Net::INAddrType) );
            memcpy( DstV4.subnetMask, &(ipFilterType->ip_hdr.v4.dst.subnet_mask), sizeof(ds::Net::INAddrType) );

            (void) NetIPFilter->SetDstV4(&DstV4);
      }

      if( (ipfltr_ip4_hdr_field_mask_type)IPFLTR_MASK_IP4_TOS & ipFilterType->ip_hdr.v4.field_mask ){
         IPFilterTOSType Tos; //done
         Tos.val = (unsigned char)ipFilterType->ip_hdr.v4.tos.val;
         Tos.mask = (unsigned char)ipFilterType->ip_hdr.v4.tos.mask;

         (void) NetIPFilter->SetTos(&Tos);
      }

   }


   // IPV6
   if (IP_V6 == ipFilterType->ip_vsn) {

      if (0 == ipFilterType->ip_hdr.v6.field_mask)
      {
         ipFilterType->ip_hdr.v6.err_mask = (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_ALL;
         abort_qos_request = true;
         goto bail;
      }

      if (ipFilterType->ip_hdr.v6.field_mask & ~(ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_ALL)
      {
         ipFilterType->ip_hdr.v6.err_mask =
          (ipFilterType->ip_hdr.v6.field_mask & ~(ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_ALL);
         abort_qos_request = true;
         goto bail;
      }

      if( (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_FLOW_LABEL & ipFilterType->ip_hdr.v6.field_mask ){
         IPFilterIPv6FlowLabelType FlowLabel = (long)ipFilterType->ip_hdr.v6.flow_label; //done
         (void) NetIPFilter->SetFlowLabel (FlowLabel);
      }

      if( (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_SRC_ADDR & ipFilterType->ip_hdr.v6.field_mask ){
         IPFilterIPv6AddrType SrcV6; //done

         memcpy( SrcV6.addr, &(ipFilterType->ip_hdr.v6.src.addr), sizeof(ds::INAddr6Type) );
         SrcV6.prefixLen = (unsigned char)ipFilterType->ip_hdr.v6.src.prefix_len;

         (void) NetIPFilter->SetSrcV6(&SrcV6);
      }

      if( (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_DST_ADDR & ipFilterType->ip_hdr.v6.field_mask ){
         IPFilterIPv6AddrType DstV6; //done

         memcpy( DstV6.addr, &(ipFilterType->ip_hdr.v6.dst.addr), sizeof(ds::INAddr6Type) );
         DstV6.prefixLen = (unsigned char)ipFilterType->ip_hdr.v6.dst.prefix_len;

         (void) NetIPFilter->SetDstV6(&DstV6);
      }

      if( (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_TRAFFIC_CLASS & ipFilterType->ip_hdr.v6.field_mask ){
         IPFilterIPv6TrafficClassType TrafficClass; //done
         TrafficClass.val = (unsigned char)ipFilterType->ip_hdr.v6.trf_cls.val;
         TrafficClass.mask = (unsigned char)ipFilterType->ip_hdr.v6.trf_cls.mask;

         (void) NetIPFilter->SetTrafficClass(&TrafficClass);
      }


      if( (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_SRC_ADDR & ipFilterType->ip_hdr.v6.field_mask ){
         IPFilterIPv6AddrType SrcV6;

         memcpy( SrcV6.addr, &(ipFilterType->ip_hdr.v6.src.addr), sizeof(ds::Net::INAddrType) );
         SrcV6.prefixLen = (unsigned char)ipFilterType->ip_hdr.v6.src.prefix_len;
         (void) NetIPFilter->SetSrcV6(&SrcV6);
      }

      if( (ipfltr_ip6_hdr_field_mask_type)IPFLTR_MASK_IP6_DST_ADDR & ipFilterType->ip_hdr.v6.field_mask ){
         IPFilterIPv6AddrType DstV6;

         memcpy( DstV6.addr, &(ipFilterType->ip_hdr.v6.dst.addr), sizeof(ds::Net::INAddrType) );
         DstV6.prefixLen = (unsigned char)ipFilterType->ip_hdr.v6.dst.prefix_len;
         (void) NetIPFilter->SetDstV6(&DstV6);
      }

   }

bail :
   // we can abort the qos request by returning the error
   if (true == abort_qos_request)
   {
     return QDS_EFAULT;
   }
   
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSQoSSpecFlows(ip_flow_spec_type* ipFlowSpecType,
                                            IQoSFlowPriv*** pppFlows,
                                            int* flowsLen,
                                            int* flowsLenReq,
                                            boolean* minFlow,
                                            qos_spec_field_mask_type field_mask,
                                            bool isTxFlow)
{
   // allocate one pointer to requested flow
   AEEResult res = AEE_SUCCESS;
   int localFlowsLen = 1;
   *minFlow = FALSE;
   ip_flow_type * tmpFlowPtr = NULL;

   if (NULL == pppFlows) {
      LOG_MSG_ERROR ("pppFlows is NULL",0, 0, 0); 
      ASSERT (0);
      return QDS_EINVAL;
   }

   (*pppFlows) = NULL;

   if (false == isTxFlow)
   {
      if ((qos_spec_field_mask_type)QOS_MASK_RX_MIN_FLOW & field_mask)
      {
         // add one pointer for the min flow
         localFlowsLen++;
         *minFlow = TRUE;
      }

      if ((qos_spec_field_mask_type)QOS_MASK_RX_AUXILIARY_FLOWS & field_mask)
      {
         // add another num_aux_flows pointers for the auxiliary flows.
         localFlowsLen += ipFlowSpecType->num_aux_flows;
      }
   }
   else 
   {
      if ((qos_spec_field_mask_type)QOS_MASK_TX_MIN_FLOW & field_mask)
      {
         // add one pointer for the min flow
         localFlowsLen++;
         *minFlow = TRUE;
      }

      if ((qos_spec_field_mask_type)QOS_MASK_TX_AUXILIARY_FLOWS & field_mask)
      {
         // add another num_aux_flows pointers for the auxiliary flows.
         localFlowsLen += ipFlowSpecType->num_aux_flows;
      }
   }

   *pppFlows = (IQoSFlowPriv **)ps_system_heap_mem_alloc(sizeof(IQoSFlowPriv*)*localFlowsLen);
   if (NULL == *pppFlows) {
      return AEE_ENOMEMORY;
   }
   for (int i=0; i<localFlowsLen; i++) {
      (*pppFlows)[i] = NULL;
   }

   // Get the NetworkFactory from DSSGlobals
   ds::Net::INetworkFactoryPriv *pNetworkFactoryPriv;
   res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);
   if(AEE_SUCCESS != res){
      goto bail;
   }

   // allocate new IDSNetQoSFlows and put them in rxFlows
   for (int i=0; i<localFlowsLen; i++)
   {
      // allocate a new DSNetQoSFlow and store it in the i'th place in the flow array.
      res =pNetworkFactoryPriv->CreateQoSFlowSpec(reinterpret_cast<IQoSFlowPriv**>(&((*pppFlows)[i])));
      if (AEE_SUCCESS != res) {
         DSSCommon::ReleaseIf((IQI**)&pNetworkFactoryPriv);
         goto bail;
      }
   }

   DSSCommon::ReleaseIf((IQI**)&pNetworkFactoryPriv);

   for (int i=0; i<localFlowsLen; i++)
   {
      if (0 == i)
      {
         // this is the first iteration, so we convert the requested flow
         tmpFlowPtr = &ipFlowSpecType->req_flow;
      }
      else if(localFlowsLen-1 == i && TRUE == *minFlow)
      {
         // This is the last iteration, so we convert the minimum flow
         tmpFlowPtr = &ipFlowSpecType->min_req_flow;
      }
      else
      {
         // This is one of the auxiliary flows
         tmpFlowPtr = &(ipFlowSpecType->aux_flow_list_ptr[i-1]);
      }

      //rx flow conversion from ds to IDS
      res = DSSConversion::DS2IDSQoSSpecFlow( tmpFlowPtr, (*pppFlows)[i]);
      if( AEE_SUCCESS != res ){
         goto bail;
      }
   }

   *flowsLen = localFlowsLen;
   *flowsLenReq = localFlowsLen;
   return res;

bail:
   for (int i=0; i < localFlowsLen; ++i) {
      if (NULL != (*pppFlows)[i]) {
         DSSCommon::ReleaseIf((IQI**)& ((*pppFlows)[i]) );
      }
   }
   PS_SYSTEM_HEAP_MEM_FREE (*pppFlows);

   return res;
}

//HM: The function IDS2DSQoSSpecFlow() is wrong. We need to update the 
//field masks of the flow spec being returned.
//
//Also, the convention is that if a value is not set in the spec, 
//DSNET code returns EINVAL for the same. In such a case, field mask
//should not be updated. The translation should not end here.
//
//I am using DSNET internal function as a temporary hack. 

namespace ds
{
namespace Net
{
namespace Conversion
{

ds::ErrorType DS2PSQoSFlowSpec
(
  IQoSFlowPriv*      pIDSNetQoSFlowSpec,
  ip_flow_type*      pPSFlowSpec
);

ds::ErrorType DS2PSIPFilterSpec
(
  IIPFilterPriv*     pIDSNetIPFilterSpec,
  ip_filter_type*      pPSFilterSpec
);

   }
   }
   }


AEEResult DSSConversion::IDS2DSQoSSpecFlow(/*in*/IQoSFlowPriv* QoSFlow, /*rout*/ip_flow_type* ipFlowType)
{
  return ds::Net::Conversion::DS2PSQoSFlowSpec (QoSFlow, ipFlowType);
}

AEEResult DSSConversion::IDS2DSIPFilterSpec(/*in*/IIPFilterPriv* Filter, /*rout*/ip_filter_type* ipFilterType)
{
  return ds::Net::Conversion::DS2PSIPFilterSpec (Filter, ipFilterType);
}

AEEResult DSSConversion::DS2IDSQoSSpecFlow(ip_flow_type* ipFlowType, IQoSFlowPriv* QoSFlow)
{
   QoSFlowOptIPTrafficClassType trfClass;
   ipFlowType->err_mask = (ipflow_field_mask_type)IPFLOW_MASK_NONE; 
   // this variable shall determine whether the qos request should be failed
   // basically we should abort the qos request when there are severe errors in qos spec preventing us from correctly converting it
   // to the ds format. If there are errors not preventing the conversion, the request should still be issued to discover the additional ones
   bool abort_qos_request = false;

   // In case none of the mask bits are set nothing will be done.
   if(ipFlowType->field_mask == (ipflow_field_mask_type)IPFLOW_MASK_NONE){
      return AEE_SUCCESS;
   }

   // In case none of the mask bits are set nothing will be done.
   if(ipFlowType->field_mask == (ipflow_field_mask_type)IPFLOW_MASK_NONE){
      return AEE_SUCCESS;
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_TRF_CLASS & ipFlowType->field_mask ) {
      trfClass = (QoSFlowOptIPTrafficClassType)ipFlowType->trf_class;
     
     if (AEE_SUCCESS != QoSFlow->SetTrfClass(trfClass))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_TRF_CLASS; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_LATENCY & ipFlowType->field_mask ) {
     QoSFlowOptLatencyType  Latency = ipFlowType->latency;
     if (AEE_SUCCESS != QoSFlow->SetLatency (Latency)) 
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_LATENCY; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_LATENCY_VAR & ipFlowType->field_mask ) {
     QoSFlowOptLatencyVarianceType LatencyVar = ipFlowType->latency_var;
     if (AEE_SUCCESS != QoSFlow->SetLatencyVar (LatencyVar))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_LATENCY_VAR; 
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_MIN_POLICED_PKT_SIZE & ipFlowType->field_mask ) {
     QoSFlowOptMinPolicedPacketSizeType MinPolicedPktSize = ipFlowType->min_policed_pkt_size;
     if (AEE_SUCCESS != QoSFlow->SetMinPolicedPktSize(MinPolicedPktSize))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_MIN_POLICED_PKT_SIZE; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_MAX_ALLOWED_PKT_SIZE & ipFlowType->field_mask ) {
     QoSFlowOptMaxAllowedPacketSizeType MaxAllowedPktSize = ipFlowType->max_allowed_pkt_size;
     if (AEE_SUCCESS != QoSFlow->SetMaxAllowedPktSize(MaxAllowedPktSize))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_MAX_ALLOWED_PKT_SIZE; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_UMTS_RES_BER & ipFlowType->field_mask ) {
     if (AEE_SUCCESS != QoSFlow->SetUmtsResBer ((QoSFlowOptUMTSResidualBitErrorRateType) ipFlowType->umts_params.res_ber))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_UMTS_RES_BER; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_UMTS_TRF_PRI & ipFlowType->field_mask ) {
     if (AEE_SUCCESS != QoSFlow->SetUmtsTrfPri((QoSFlowOptUMTSTrafficPriorityType) ipFlowType->umts_params.trf_pri))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_UMTS_TRF_PRI; 
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_CDMA_PROFILE_ID & ipFlowType->field_mask ) {
     QoSFlowOptCDMAProfileIDType CdmaProfileID = (unsigned short)ipFlowType->cdma_params.profile_id;
     if (AEE_SUCCESS != QoSFlow->SetCdmaProfileID(CdmaProfileID))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_CDMA_PROFILE_ID; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_WLAN_USER_PRI & ipFlowType->field_mask ) {
     if (AEE_SUCCESS != QoSFlow->SetWlanUserPriority((QoSFlowOptWLANUserPriorityType) ipFlowType->wlan_params.user_priority))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_WLAN_USER_PRI; 
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_WLAN_MIN_SERVICE_INTERVAL & ipFlowType->field_mask ) {
     QoSFlowOptWLANMinServiceIntervalType WlanMinServiceInterval = (long)ipFlowType->wlan_params.min_service_interval;
     if (AEE_SUCCESS != QoSFlow->SetWlanMinServiceInterval(WlanMinServiceInterval))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_WLAN_MIN_SERVICE_INTERVAL; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_WLAN_MAX_SERVICE_INTERVAL & ipFlowType->field_mask ) {
     QoSFlowOptWLANMaxServiceIntervalType WlanMaxServiceInterval = (long)ipFlowType->wlan_params.max_service_interval;
     if (AEE_SUCCESS != QoSFlow->SetWlanMaxServiceInterval(WlanMaxServiceInterval))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_WLAN_MAX_SERVICE_INTERVAL; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_WLAN_INACTIVITY_INTERVAL & ipFlowType->field_mask ) {
     QoSFlowOptWLANInactivityIntervalType WlanInactivityInterval = (long)ipFlowType->wlan_params.inactivity_interval;
     if (AEE_SUCCESS != QoSFlow->SetWlanInactivityInterval(WlanInactivityInterval))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_WLAN_INACTIVITY_INTERVAL; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_CDMA_FLOW_PRIORITY & ipFlowType->field_mask ) {
     QoSFlowOptCDMAFlowPriorityType CdmaFlowPriority = (unsigned char)ipFlowType->cdma_params.flow_priority;
     if (AEE_SUCCESS != QoSFlow->SetCdmaFlowPriority(CdmaFlowPriority))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_CDMA_FLOW_PRIORITY; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_UMTS_IM_CN_FLAG & ipFlowType->field_mask ) {
     QoSFlowOptUMTSImsSignalingContextType UmtsImCnFlag = ipFlowType->umts_params.im_cn_flag;
     if (AEE_SUCCESS != QoSFlow->SetUmtsImCnFlag(UmtsImCnFlag))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_UMTS_IM_CN_FLAG; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_UMTS_SIG_IND & ipFlowType->field_mask ) {
     QoSFlowOptUMTSHighPriorityDataType UmtsSigInd = ipFlowType->umts_params.sig_ind;
     if (AEE_SUCCESS != QoSFlow->SetUmtsSigInd(UmtsSigInd))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_UMTS_SIG_IND; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_DATA_RATE & ipFlowType->field_mask ) {
     
     if( DATA_RATE_FORMAT_MIN_MAX_TYPE == ipFlowType->data_rate.format_type ) {

        QoSFlowOptDataRateMinMaxType DataRateMinMax;

        DataRateMinMax.maxRate = (long)ipFlowType->data_rate.format.min_max.max_rate;
        DataRateMinMax.guaranteedRate = (long)ipFlowType->data_rate.format.min_max.guaranteed_rate;

        if (AEE_SUCCESS != QoSFlow->SetDataRateMinMax (&DataRateMinMax))
        {
           ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_DATA_RATE; 
           abort_qos_request = true;
        }
     }

     else if( DATA_RATE_FORMAT_TOKEN_BUCKET_TYPE == ipFlowType->data_rate.format_type ) {
        QoSFlowOptDataRateTokenBucketType DataRateTokenBucket; // = &(ipFlowType->data_rate.format.token_bucket);

        DataRateTokenBucket.peakRate = (long)ipFlowType->data_rate.format.token_bucket.peak_rate;
        DataRateTokenBucket.tokenRate = (long)ipFlowType->data_rate.format.token_bucket.token_rate;
        DataRateTokenBucket.size = (long)ipFlowType->data_rate.format.token_bucket.size;

        if (AEE_SUCCESS != QoSFlow->SetDataRateTokenBucket (&DataRateTokenBucket))
        {
           ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_DATA_RATE; 
           abort_qos_request = true;
        }
     }
     else {
        LOG_MSG_ERROR ("Invlaid data rate format %d", ipFlowType->data_rate.format_type, 0, 0);
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_DATA_RATE; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_PKT_ERR_RATE & ipFlowType->field_mask ) {
     QoSFlowOptPacketErrorRateType PktErrRate; //= ipFlowType->pkt_err_rate;

     PktErrRate.multiplier = (unsigned short)ipFlowType->pkt_err_rate.multiplier;
     PktErrRate.exponent = (unsigned short)ipFlowType->pkt_err_rate.exponent;

     if (AEE_SUCCESS != QoSFlow->SetPktErrRate (&PktErrRate))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_PKT_ERR_RATE; 
        abort_qos_request = true;
     }
   }

   if( (ipflow_field_mask_type)IPFLOW_MASK_NOMINAL_SDU_SIZE & ipFlowType->field_mask ) {
     QoSFlowOptNominalSDUSizeType NominalSDUSize; // = ipFlowType->nominal_sdu_size;

     NominalSDUSize.fixed = ipFlowType->nominal_sdu_size.is_fixed;
     NominalSDUSize.size = (long)ipFlowType->nominal_sdu_size.size;

     if (AEE_SUCCESS != QoSFlow->SetNominalSDUSize (&NominalSDUSize))
     {
        ipFlowType->err_mask |= (ipflow_field_mask_type)IPFLOW_MASK_NOMINAL_SDU_SIZE; 
        abort_qos_request = true;
     }
   }

   // we can abort the qos request by returning the error
   if (true == abort_qos_request)
   {
      return QDS_EFAULT;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSQoSMask(qos_spec_field_mask_type qosMaskType, QoSModifyMaskType* modifyMask)
{
   *modifyMask = (QoSModifyMaskType)qosMaskType;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSMask(qos_spec_field_mask_type* qosMaskType, QoSModifyMaskType modifyMask)
{
   *qosMaskType = (qos_spec_field_mask_type)modifyMask;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSMCast(dss_iface_ioctl_mcast_join_type* mcast_join,
                                     ds::SockAddrStorageType addr,
                                     IMCastManagerPriv* pNetMCastManagerPriv,
                                     IQI** ppInfo)
{
   IMBMSSpecPriv* mbmsInfo =  NULL;
   AEEResult res = AEE_SUCCESS;
   ds::SockAddrINType* pSockAddrIn = (struct ds::SockAddrINType*) addr;
   ds::SockAddrIN6Type* pSockAddrIn6 = (struct ds::SockAddrIN6Type*) addr;

   ds::AddrFamilyType tmpFamily = ds::AddrFamily::QDS_AF_UNSPEC;

   if(0 == pNetMCastManagerPriv){
      return QDS_EFAULT;
   }
   
   IDS_ERR_RET(DS2IDSEnumAddrFamily(mcast_join->ip_addr.type,  &tmpFamily));
   
   tmpFamily = ps_htons(tmpFamily);
   
   (void)ds::Sock::AddrUtils::SetFamily(addr,tmpFamily);

   if( IPV4_ADDR == mcast_join->ip_addr.type )
   {
      memcpy( (uint8*)&(pSockAddrIn->addr), &(mcast_join->ip_addr.addr.v4), sizeof( uint32 ) );
      pSockAddrIn->port = mcast_join->port;
   }
   else if ( IPV6_ADDR == mcast_join->ip_addr.type )
   {
      memcpy( (void*)(pSockAddrIn6->addr), (void*)(mcast_join->ip_addr.addr.v6), (sizeof(uint64))*2 );
      pSockAddrIn6->port = mcast_join->port;
   }
   else
   {
      return QDS_EFAULT;
   }


   dss_iface_ioctl_mbms_mcast_param_type *mbms_mcast_param_ptr = (dss_iface_ioctl_mbms_mcast_param_type *)mcast_join->mcast_param_ptr;
   if (NULL == mbms_mcast_param_ptr) {
      // MCAST info will be null for now.
      *ppInfo = NULL;
      return AEE_SUCCESS;
   } else {
      
      res = pNetMCastManagerPriv->CreateMBMSSpecPriv(&mbmsInfo);
      if (AEE_SUCCESS != res) {
         goto bail;
      }

      res = mbmsInfo->SetTMGI(mbms_mcast_param_ptr->tmgi);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetSessionStartTime(mbms_mcast_param_ptr->session_start_time);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetSessionEndTime(mbms_mcast_param_ptr->session_end_time);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetPriority((unsigned short int)(mbms_mcast_param_ptr->priority));
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetService((MBMSServiceType)mbms_mcast_param_ptr->service_type);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetServiceMethod((MBMSServiceMethodType)mbms_mcast_param_ptr->service_method);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetSelectedService(mbms_mcast_param_ptr->selected_service);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      res = mbmsInfo->SetServiceSecurity(mbms_mcast_param_ptr->service_security);
      if (AEE_SUCCESS != res) {
         goto bail;
      }
      *ppInfo = (IQI*)mbmsInfo;
      return res;
   }

bail:
   DSSCommon::ReleaseIf((IQI**)&mbmsInfo);

   return res;
}

AEEResult DSSConversion::DS2IDSMCastBundle(dss_iface_ioctl_mcast_join_ex_type* mcast_join_ex,
                                           ds::SockAddrStorageType* addrSeq,
                                           ds::Net::MCastJoinFlagsType * mcastFlags,
                                           ds::Net::IMCastManagerPriv* pNetMCastManagerPriv,
                                           IQI** info)
{
   sint15 nRet = DSS_SUCCESS;
   dss_iface_ioctl_mcast_join_type   mcast_join;

   for( uint32 i = 0; mcast_join_ex->num_flows > i; i++ ){
      memset(&mcast_join, 0, sizeof(dss_iface_ioctl_mcast_join_type));
      //perform the conversion
      mcast_join.app_id = mcast_join_ex->dss_nethandle;
      mcast_join.event_cb = mcast_join_ex->event_cb;
      mcast_join.handle = mcast_join_ex->handle[i];
      memcpy((void*)&(mcast_join.ip_addr), (const void*)&(mcast_join_ex->ip_addr[i]), sizeof(ip_addr_type));
      mcast_join.mcast_param_ptr = mcast_join_ex->mcast_param_ptr[i];
      mcast_join.port= mcast_join_ex->port[i];
      mcast_join.user_data_ptr = mcast_join_ex->user_data_ptr;
      mcastFlags[i] = mcast_join_ex->mcast_request_flags[i];

      nRet = DSSConversion::DS2IDSMCast(&mcast_join, addrSeq[i], pNetMCastManagerPriv, &(info[i]));
      if(DSS_SUCCESS != nRet){
         return nRet;
      }
   }

   return DSS_SUCCESS;
}

AEEResult DSSConversion::IDS2DSMCastJoinEx(const ::ds::SockAddrStorageType* addrSeq,
                                           IQI* info,
                                           dss_iface_ioctl_mcast_join_ex_type* mcast_Control_Bundle)
{
   AEEResult res = AEE_SUCCESS;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   int j = 0;
   for( uint32 i = 0; (mcast_Control_Bundle->num_flows) > i ; i++ )
   {

     if( DSS_IFACE_IOCTL_MCAST_REG_SETUP_ALLOWED == mcast_Control_Bundle->mcast_request_flags[i] ) {
         struct ds::SockAddrINType* pSockAddrIn = (struct ds::SockAddrINType*) addrSeq[j];
         struct ds::SockAddrIN6Type* pSockAddrIn6 = (struct ds::SockAddrIN6Type*) addrSeq[j];

         dss_iface_ioctl_mbms_mcast_param_type* mbms_mcast_param_ptr = (dss_iface_ioctl_mbms_mcast_param_type *)mcast_Control_Bundle->mcast_param_ptr[j];
         IDS_ERR_RET(IDS2DSMCast(info, mbms_mcast_param_ptr));

         (void)ds::Sock::AddrUtils::GetFamily(addrSeq[j],&family);
         res = IDS2DSEnumAddrFamily(family, &(mcast_Control_Bundle->ip_addr[j].type));

         if(AEE_SUCCESS != res) {
            return res;
         }

         if( ds::AddrFamily::QDS_AF_INET == family )
         {
            memcpy( &(mcast_Control_Bundle->ip_addr[j].addr.v4), (uint8*)&pSockAddrIn->addr, sizeof( uint32 ) );
         }
         else if ( ds::AddrFamily::QDS_AF_INET6 == family )
         {
            // The memory that ds::INAddr6Type takes is the same as 2*uint64. 
            //In order to avoid KlockWork error of possible buffer overflow the copy is of 2*uint64
            memcpy( (void*)mcast_Control_Bundle->ip_addr[j].addr.v6, (void*)pSockAddrIn6->addr, 2*sizeof (uint64));
         }
         else
         {
            return QDS_EINVAL;
         }

         j++;
      }
   }

   return AEE_SUCCESS;
}
AEEResult DSSConversion::IDS2DSMCastJoin(const ds::SockAddrStorageType addr,
                                         IQI* info,
                                         dss_iface_ioctl_mcast_join_type* mcast_join)
{
   AEEResult res = AEE_SUCCESS;
   dss_iface_ioctl_mbms_mcast_param_type *mbms_mcast_param_ptr = (dss_iface_ioctl_mbms_mcast_param_type *)mcast_join->mcast_param_ptr;
   ds::SockAddrINType* pSockAddrIn = (ds::SockAddrINType*) addr;
   ds::SockAddrIN6Type* pSockAddrIn6 = (ds::SockAddrIN6Type*) addr;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   
   (void)ds::Sock::AddrUtils::GetFamily(addr,&family);

   IDS_ERR_RET(IDS2DSMCast(info, mbms_mcast_param_ptr));

   res = IDS2DSEnumAddrFamily(family, &(mcast_join->ip_addr.type));
   if (AEE_SUCCESS != res) {
      return res;
   }

   if( ds::AddrFamily::QDS_AF_INET == family )
   {
      memcpy( &(mcast_join->ip_addr.addr.v4),(uint8*)&(pSockAddrIn->addr), sizeof( uint32 ) );
   }
   else if ( ds::AddrFamily::QDS_AF_INET6 == family )
   {
      // The memory that ds::INAddr6Type takes is the same as 2*uint64. 
      //In order to avoid KlockWork error of possible buffer overflow the copy is of 2*uint64 
      memcpy( (void*)(mcast_join->ip_addr.addr.v6), (void*)(pSockAddrIn6->addr), (sizeof (uint64))*2);
   }
   else
   {
      return QDS_EINVAL;
   }

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSMCast(IQI* info,
                                     dss_iface_ioctl_mbms_mcast_param_type* mbms_mcast_param_ptr)
{
   IMBMSSpecPriv *mbmsInfo = (IMBMSSpecPriv *)info;

   uint64 TMGI;
   IDS_ERR_RET(mbmsInfo->GetTMGI(&TMGI));
   mbms_mcast_param_ptr->tmgi = TMGI;

   uint64 sessionStartTime;
   IDS_ERR_RET(mbmsInfo->GetSessionStartTime(&sessionStartTime));
   mbms_mcast_param_ptr->session_start_time = sessionStartTime;

   uint64 sessionEndTime;
   IDS_ERR_RET(mbmsInfo->GetSessionEndTime (&sessionEndTime));
   mbms_mcast_param_ptr->session_end_time = sessionEndTime;

   unsigned short int Priority;
   IDS_ERR_RET(mbmsInfo->GetPriority(&Priority));
   mbms_mcast_param_ptr->priority = (uint16)Priority;

   MBMSServiceType service;
   IDS_ERR_RET(mbmsInfo->GetService(&service));
   mbms_mcast_param_ptr->service_type = (ps_iface_ioctl_mbms_service_type)service;

   MBMSServiceMethodType serviceMethod;
   IDS_ERR_RET(mbmsInfo->GetServiceMethod(&serviceMethod));
   mbms_mcast_param_ptr->service_method = (ps_iface_ioctl_mbms_service_method)serviceMethod;

   boolean serviceSecurity;
   IDS_ERR_RET(mbmsInfo->GetServiceSecurity(&serviceSecurity));
   mbms_mcast_param_ptr->service_security = serviceSecurity;

   boolean selectedService;
   IDS_ERR_RET(mbmsInfo->GetSelectedService(&selectedService));
   mbms_mcast_param_ptr->selected_service = selectedService;

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSDormancyInfoCode(DormancyInfoCodeType dormInfoCode,
                                                dss_iface_ioctl_dormancy_info_code_enum_type* DSDormInfoCode)
{
   *DSDormInfoCode = (dss_iface_ioctl_dormancy_info_code_enum_type)dormInfoCode;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSMode(QoSModeType qosMode, dss_iface_ioctl_qos_mode_type* DSQoSMode)
{
   DSQoSMode->qos_mode = (ps_iface_ioctl_qos_mode_enum_type)qosMode;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSSDBAckStatus(ds::Sock::DoSAckStatusType status, dss_sdb_ack_status_enum_type* DSStatus)
{
   *DSStatus = (dss_sdb_ack_status_enum_type)status;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSQoSAwareInfoCode(QoSInfoCodeType infoCode, dss_iface_ioctl_extended_info_code_enum_type* DSInfoCode)
{
   *DSInfoCode = (dss_iface_ioctl_extended_info_code_enum_type)infoCode;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSInertiaFailureInfoCode(Network1xPrivHDRRev0RateInertiaFailureCodeType infoCode,
                                                      dss_iface_ioctl_hdr_rev0_rate_inertia_failure_code_enum_type* DSInfoCode)
{
   *DSInfoCode = (dss_iface_ioctl_hdr_rev0_rate_inertia_failure_code_enum_type)infoCode;
   return AEE_SUCCESS;
}
AEEResult DSSConversion::DS2IDSMCastStatus(dss_iface_ioctl_mcast_info_code_enum_type regStatus, MCastInfoCodeType* infoCode)
{
   *infoCode = (MCastInfoCodeType)regStatus;
   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSIPv6PrefixState(IPv6AddrStateType netIPv6PrefState,ps_iface_ipv6_addr_state_enum_type *prefStatus)
{
   *prefStatus = (ps_iface_ipv6_addr_state_enum_type)netIPv6PrefState;
   return AEE_SUCCESS;
}
AEEResult DSSConversion::DS2IDSStaticNatEntry(ps_iface_ioctl_static_nat_entry_type*  pPSIPNatEntry,
                                              ds::Net::IPNatStaticEntryType*         pDSNetIPNatEntry)
{
   if (NULL == pPSIPNatEntry || NULL == pDSNetIPNatEntry)
   {
      ASSERT (0);
   }

   pDSNetIPNatEntry->protocol = pPSIPNatEntry->protocol;

   memcpy(pDSNetIPNatEntry->private_ip_addr, 
          &pPSIPNatEntry->private_ip_addr, 4);

   memcpy(pDSNetIPNatEntry->private_port, &pPSIPNatEntry->private_port,
          2);

   memcpy(pDSNetIPNatEntry->global_port, &pPSIPNatEntry->global_port,
          2);

   return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSStaticNatEntry(ds::Net::IPNatStaticEntryType*         pDSNetIPNatEntry,
                                              ps_iface_ioctl_static_nat_entry_type*  pPSIPNatEntry)
{
   if (NULL == pPSIPNatEntry || NULL == pDSNetIPNatEntry)
   {
      ASSERT (0);
   }

   pPSIPNatEntry->protocol = pDSNetIPNatEntry->protocol;

   memcpy(&pPSIPNatEntry->private_ip_addr,
      pDSNetIPNatEntry->private_ip_addr, 4);

   memcpy(&pPSIPNatEntry->private_port, pDSNetIPNatEntry->private_port,
         2);

   memcpy(&pPSIPNatEntry->global_port, pDSNetIPNatEntry->global_port,
          2);

   return AEE_SUCCESS;
}
AEEResult DSSConversion::DS2IDSDMZEntry(dss_iface_ioctl_dmz_type*  pPSDMZEntry,
                                        ds::Net::DMZEntryType*     pDSNetDMZEntry)
{
  if (NULL == pPSDMZEntry || NULL == pDSNetDMZEntry)
  {
    ASSERT (0);
  }

  memcpy(pDSNetDMZEntry->dmz_ip_addr, 
         &pPSDMZEntry->dmz_ip_addr.addr.v4,
         sizeof(uint32));

  return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSDMZEntry(ds::Net::DMZEntryType*     pDSNetDMZEntry,
                                        dss_iface_ioctl_dmz_type*  pPSIPDMZEntry)
{
  if (NULL == pPSIPDMZEntry || NULL == pDSNetDMZEntry)
  {
    ASSERT (0);
  }

  memcpy(&pPSIPDMZEntry->dmz_ip_addr.addr.v4,
         pDSNetDMZEntry->dmz_ip_addr,
         sizeof(uint32));
  
  return AEE_SUCCESS;
}

void DSSConversion::FreeQoSMemOnError(QoSSpecType* specType)
{
   if (NULL != specType->rxFilter){
      for (int i=0 ; i<specType->rxFilterLen; i++) {
         if (specType->rxFilter[i] != NULL){
            DSSCommon::ReleaseIf((IQI**)(&(specType->rxFilter[i])));
         }
      }
      PS_SYSTEM_HEAP_MEM_FREE (specType->rxFilter);
   }

   if (NULL != specType->txFilter){
      for (int i=0 ; i<specType->txFilterLen; i++) {
         if (specType->txFilter[i] != NULL){
            DSSCommon::ReleaseIf((IQI**)(&(specType->txFilter[i])));
         }
      }
      PS_SYSTEM_HEAP_MEM_FREE (specType->txFilter);
   }

   if (NULL != specType->rxFlows) {
      for (int i=0; i < specType->rxFlowsLen; ++i) {
         if (NULL != (specType->rxFlows)[i]) {
            DSSCommon::ReleaseIf((IQI**)& ((specType->rxFlows)[i]) );
         }
      }
      PS_SYSTEM_HEAP_MEM_FREE (specType->rxFlows);
   }

   if (NULL != specType->txFlows) {
      for (int i=0; i < specType->txFlowsLen; ++i) {
         if (NULL != (specType->txFlows)[i]) {
            DSSCommon::ReleaseIf((IQI**)& ((specType->txFlows)[i]) );
         }
      }
      PS_SYSTEM_HEAP_MEM_FREE (specType->txFlows);
   }
}

AEEResult DSSConversion::IDS2DSRcvMsgFlag(unsigned int idsRcvMsgFlags,
                                          unsigned int *dsRcvMsgFlags)
{
  if ( NULL == dsRcvMsgFlags )
  {
    return QDS_EFAULT;
  }
  *dsRcvMsgFlags = 0;
  //Only support DSS_MSG_ERRQUEUE so verify no other flags
  if (0 != ( idsRcvMsgFlags & ~DSS_MSG_ERRQUEUE ) )
  {
    LOG_MSG_INVALID_INPUT( "Invalid flags 0x%x ", idsRcvMsgFlags,0 , 0);
    return QDS_EOPNOTSUPP;
  }

  if ( idsRcvMsgFlags & DSS_MSG_ERRQUEUE )
  {
    *dsRcvMsgFlags |= ds::Sock::RecvFlags::QDS_MSG_ERRQUEUE;
  }

  return AEE_SUCCESS;
}

AEEResult DSSConversion::IDS2DSStatistics(const NetworkStatsType* statsType, dss_iface_ioctl_get_iface_stats_type* stats)
{
  if (NULL == statsType || NULL == stats)
  {
    ASSERT (0);
  }

  stats->bytes_rx = statsType->bytesRX;
  stats->bytes_tx = statsType->bytesTX;
  stats->total_bytes_transferred_since_power_up = statsType->totalBytesTransferredSincePowerUp;
  stats->bytes_transferred = statsType->bytesTransferred;
  stats->pkts_rx = statsType->pktsRX;
  stats->mcast_pkts_rx = statsType->mcastPktsRX;
  stats->pkts_dropped_rx = statsType->pktsDroppedRX;
  stats->pkts_tx = statsType->pktsTX;
  stats->mcast_pkts_tx = statsType->mcastPktsTX;
  stats->pkts_dropped_tx = statsType->pktsDroppedTX;
  stats->total_open_time_since_power_up = statsType->totalOpenTimeSincePowerUp;
  stats->total_active_time_since_power_up = statsType->totalActiveTimeSincePowerUp;
  stats->first_io_timestamp = statsType->firstIOTimeStamp;
  stats->last_up_state_timestamp = statsType->lastUpStateTimeStamp;
  stats->state = statsType->state;

  return AEE_SUCCESS;
}

AEEResult DSSConversion::DS2IDSFMCTunnelParams(dss_iface_ioctl_uw_fmc_tunnel_params_type * pDSFMCTunnelParams,
                                               ds::Net::FMCTunnelParamsType * pIDSFMCTunnelParams)
{
   if(0 == pDSFMCTunnelParams || 0 == pIDSFMCTunnelParams){
      return QDS_EFAULT;
   }
   
   pIDSFMCTunnelParams->streamId = pDSFMCTunnelParams->stream_id;
   
   pIDSFMCTunnelParams->IsNatPresent = pDSFMCTunnelParams->is_nat_present;

   pIDSFMCTunnelParams->addrLen = pDSFMCTunnelParams->addr_len;

   return DS2IDSSockAddrType((struct ps_sockaddr * )pDSFMCTunnelParams->tunnel_end_point_info_ptr,
                            pIDSFMCTunnelParams->tunnelEndPointAddr,
                            pDSFMCTunnelParams->addr_len);
}

AEEResult DSSConversion::DS2IDSSockAddrType(struct ps_sockaddr *pDSSockddrType, 
                                           ds::SockAddrStorageType pIDSSockAddr,
                                           uint16 addrLen)
{
   ds::AddrFamilyType tempFamily;
   AEEResult res = AEE_SUCCESS;

   if(0 == pDSSockddrType || 0 == pIDSSockAddr){
      return QDS_EFAULT;
   }

   if (DSS_AF_INET != pDSSockddrType->ps_sa_family &&
       DSS_AF_INET6 != pDSSockddrType->ps_sa_family){
     LOG_MSG_ERROR("Illegal Address Family", 0, 0, 0);
     goto bail;
   }

   if (DSS_AF_INET == pDSSockddrType->ps_sa_family){
     if (sizeof(struct ps_sockaddr_in) != addrLen) {
       LOG_MSG_ERROR("Illegal IPv4 address length", 0, 0, 0);
       goto bail;
     }
   }

   if (DSS_AF_INET6 == pDSSockddrType->ps_sa_family){
     if (sizeof(struct ps_sockaddr_in6) != addrLen) {
       LOG_MSG_ERROR("Illegal IPv6 address length", 0, 0, 0);
       goto bail;
     }
   }

   // Memcpy ps_sockaddr buffer into SockAddrStorageType storage buffer
   addrLen = min(addrLen, sizeof(ds::SockAddrStorageType));

   memcpy(pIDSSockAddr, pDSSockddrType, addrLen);

   res = DS2IDSAddrFamily(pDSSockddrType->ps_sa_family, &tempFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Cannot convert address family, res = 0x%x", res, 0, 0);
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(pIDSSockAddr, ps_htons(tempFamily));

   return AEE_SUCCESS;

bail:

   return QDS_EFAULT;
}

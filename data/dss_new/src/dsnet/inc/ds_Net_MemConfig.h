#ifndef DS_NET_MEM_CONFIG_H
#define DS_NET_MEM_CONFIG_H

/*===========================================================================
  @file DSNetMemConfig.h

  This file provides macros to configure memory buffers for ds Network and
  Sockets libraries during compile time.

  TODO: Write detailed explanation.

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/inc/ds_Net_MemConfig.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
2008-03-15 pp  CMI: Public/Private split.
2008-06-27 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "dssocket_defs.h"
#include "dss_config.h"

/*===========================================================================

                     PUBLIC DATA DEFINITIONS

===========================================================================*/

/*-------------------------------------------------------------------------
  Max number of network objects available system wide.
--------------------------------------------------------------------------*/
#define MAX_NETWORK_OBJS          (DSS_MAX_APPS)

/*-------------------------------------------------------------------------
  Max number of Default QoS objects available system wide.
--------------------------------------------------------------------------*/
#define MAX_QOS_DEFAULT_OBJS      (MAX_NETWORK_OBJS)

/*-------------------------------------------------------------------------
  Max number of Secondary QoS objects available system wide.
  This number is heuristically determined by the currently expected 
  secondary QoS usage.
--------------------------------------------------------------------------*/
#define MAX_QOS_SECONDARY_OBJS    (32)

/*-------------------------------------------------------------------------
  Max number of MCast Flows available system wide (heuristic estimate).
--------------------------------------------------------------------------*/
#define MAX_MCAST_OBJS            (32)

/*-------------------------------------------------------------------------
  Max number of technology specific objects (heuristic estimate).
--------------------------------------------------------------------------*/
#define MAX_NETWORK_1X_OBJS       (20)
#define MAX_NETWORK_UMTS_OBJS     (20)
#define MAX_NETWORK_MBMS_OBJS     (10)
#define MAX_NETWORK_BCMCS_OBJS    (10)
#define MAX_QOS_1X_OBJS           (MAX_QOS_SECONDARY_OBJS)

/*-------------------------------------------------------------------------
  Max number of phys link objects available system wide.
--------------------------------------------------------------------------*/
#define MAX_PHYS_LINK_OBJS        (MAX_QOS_SECONDARY_OBJS)

/*-------------------------------------------------------------------------
  Max number of network policy objects available system wide.
--------------------------------------------------------------------------*/
#define MAX_POLICY_OBJS           (50)

/*-------------------------------------------------------------------------
  Max number of network policy objects available system wide.
--------------------------------------------------------------------------*/
#define MAX_EVENT_NODE_OBJS       (200)

#endif /* DS_NET_MEM_CONFIG_H */


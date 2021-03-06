/******************************************************************************

                          N E T M G R _ T C . C

******************************************************************************/

/******************************************************************************

  @file    netmgr_tc.c
  @brief   Network Manager traffic control

  DESCRIPTION
  Implementation of NetMgr Linux traffic control module.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010-2014 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
02/23/10   ar         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#ifdef NETMGR_QOS_ENABLED

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "ds_list.h"
#include "ds_string.h"
#include "netmgr_defs.h"
#include "netmgr_exec.h"
#include "netmgr_platform.h"
#include "netmgr_util.h"
#include "netmgr_tc_i.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
  Constants for queue discipline and class configuration
---------------------------------------------------------------------------*/
#define NETMGR_TC_QDISC_ROOT_MAJOR                    1
#define NETMGR_TC_QDISC_ROOT_MINOR                    0
#define NETMGR_TC_QDISC_DEFAULT_CLASS                 10
#define NETMGR_TC_QDISC_HTB_RATE2QTM                  60  /* bandwidth borroring chunks */
#define NETMGR_TC_FILTER_PRIORITY                     1
#define NETMGR_TC_TCP_ACK_FILTER_PRIORITY             0
#define NETMGR_TC_QDISC_DEFAULT_CLASS_TCP_ACK_PRIO    20

#define NETMGR_TC_CLASS_MINOR_ID_INIT    10  /* starting sequence value    */
#define NETMGR_TC_CLASS_MINOR_ID_INC     10  /* sequence value increment   */
#define NETMGET_TC_ASSIGN_CLASS_MINOR_ID(link,val)               \
        {                                                        \
          val = netmgr_tc_cfg.links[link].next_class_minor;      \
          netmgr_tc_cfg.links[link].next_class_minor +=          \
            NETMGR_TC_CLASS_MINOR_ID_INC;                        \
        }

#define NETMGR_TC_CHAIN_NAME_PREFIX      "qcom"
#define NETMGR_TC_MAX_COMMAND_LENGTH     256
#define NETMGR_TC_IPTABLES_TOOL_V4       "iptables"
#define NETMGR_TC_IPTABLES_TOOL_V6       "ip6tables"
#define NETMGR_TC_HANDLE_LEN             5
#define NETMGR_TC_TCP_PROTO_IANA_NUM     6
#define NETMGR_TC_UDP_PROTO_IANA_NUM     17
#define NETMGR_TC_INVALID_PROTO          -1

#define NETMGR_TC_CHAIN_NAME_POSTROUTING "qcom_qos_postrouting"
#define NETMGR_TC_DEFAULT_FLOW_MARKING   0

LOCAL struct netmgr_tc_cfg_s netmgr_tc_cfg;

/*---------------------------------------------------------------------------
  CDMA QoS specification table    Reference: 3GPP2 C.R101-G (TSB-58)
  Used to map profile ID to traffic control priority and datarate.
---------------------------------------------------------------------------*/
struct netmgr_tc_cdma_qos_spec_s {
  uint16      profile_id;
  uint32      datarate;   /* bps units */
  uint8       priority;
};

LOCAL const struct netmgr_tc_cdma_qos_spec_s
netmgr_tc_cdma_qos_spec_tbl[] = {
  {    0, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_BESTEFFORT },
  {    1, 32000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {    2, 64000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {    3, 96000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {    4, 128000,    NETMGR_TC_CLASS_PRIO_STREAMING       },
  {    5, 32000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {    6, 64000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {    7, 96000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {    8, 144000,    NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {    9, 384000,    NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   10, 768000,    NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   11, 1536000,   NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   12, 32000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   13, 64000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   14, 96000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   15, 144000,    NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   16, 384000,    NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   17, 32000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   18, 64000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   19, 96000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   20, 144000,    NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note1*/
  {   21, 32000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   22, 64000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   23, 96000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   24, 144000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   25, 384000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   26, 768000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   27, 1536000,   NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   28, 32000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   29, 64000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   30, 96000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   31, 144000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   32, 384000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   33, 768000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   34, 1536000,   NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   35, 32000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   36, 64000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   37, 96000,     NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   38, 140004,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   39, 384000,    NETMGR_TC_CLASS_PRIO_INTERACTIVE     },  /*Note1*/
  {   40, 32000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   41, 64000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   42, 96000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   43, 144000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   44, 384000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   45, 768000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   46, 1536000,   NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   47, 32000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   48, 64000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   49, 96000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   50, 144000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   51, 384000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   52, 768000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   53, 1536000,   NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   54, 32000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   55, 64000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   56, 96000,     NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   57, 144000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   58, 384000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   59, 768000,    NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  {   60, 1536000,   NETMGR_TC_CLASS_PRIO_BACKGROUND      },  /*Note1*/
  /* Reserved range: 61-255 */
  {  256, 12200,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2,3*/
  {  257, 15000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  258, 12200,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2,3*/
  {  259, 15000,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  260, 8000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  261, 12200,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2,3*/
  {  262, 15000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  263, 12200,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2,3*/
  {  264, 15000,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  265, 8000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  266, 12200,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2,3*/
  {  267, 15000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  268, 12200,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2,3*/
  {  269, 15000,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  270, 8000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  271, 12200,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2,3*/
  {  272, 15000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  273, 12200,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2,3*/
  {  274, 15000,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  275, 8000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  276, 12200,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2,4*/
  {  277, 5000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  278, 5000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  279, 5000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  280, 5000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  281, 12200,     NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2,3*/
  {  282, 5000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  283, 5000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  284, 5000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  285, 5000,      NETMGR_TC_CLASS_PRIO_STREAMING       },  /*Note2*/
  {  286, 6000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  287, 6000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  288, 3000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  289, 6000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  290, 6000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  291, 3000,      NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  {  292, 10000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },  /*Note2*/
  /* Reserved range: 293-511 */
  {  512, 16000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  513, 24000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  514, 32000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  515, 48000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  516, 64000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  /* Reserved range: 517-767 */
  {  768, 24000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  769, 32000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  770, 40000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  771, 48000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  772, 56000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  773, 64000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  774, 24000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  775, 32000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  776, 40000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  777, 48000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  778, 56000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  779, 64000,     NETMGR_TC_CLASS_PRIO_CONVERSATIONAL  },
  {  780, 24000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  781, 48000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  782, 64000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  783, 96000,     NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  784, 120000,    NETMGR_TC_CLASS_PRIO_STREAMING       },
  {  785, 128000,    NETMGR_TC_CLASS_PRIO_STREAMING       },
  /* Reserved range: 786-1023 */
  { 1024, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_STREAMING }, /*Note2*/
  /* Reserved range: 1025-1279 */
  { 1280, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_CONVERSATIONAL }, /*Note2*/
  { 1281, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_STREAMING      }, /*Note2*/
  { 1282, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note2*/
  { 1283, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_CONVERSATIONAL }, /*Note1,2*/
  /* Reserved range: 1284-1535 */
  { 1536, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note1,2*/
  /* Reserved range: 1537-1791 */
  { 1792, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note2*/
  { 1793, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note2*/
  { 1794, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note2*/
  { 1795, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note2*/
  { 1796, NETMGR_TC_DEFAULT_DATARATE, NETMGR_TC_CLASS_PRIO_INTERACTIVE    }, /*Note2*/
  /* Reserved range: 1797-2047 */

  /* Proprietary flow profile ID list may be appended to this table. */
};
/* Note1: priority assigned based on spec interpretation */
/* Note2: data rate assigned based on spec interpretation */
/* Note3: assuming 12.2kbps AMR codec */

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  netmgr_tc_flow_alloc
===========================================================================*/
/*!
@brief
  Allocate dynamic memory for flow info buffer

@return
  netmgr_tc_flow_info_t * - pointer to allocated buffer on success,
                            NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL netmgr_tc_flow_info_t *
netmgr_tc_flow_alloc( void )
{
  netmgr_tc_flow_info_t * flow_buf = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  if((flow_buf = netmgr_malloc(sizeof(netmgr_tc_flow_info_t))) == NULL ) {
    netmgr_log_err("netmgr_tc_flow_alloc: netmgr_malloc failed\n");
  }

  NETMGR_LOG_FUNC_EXIT;

  /* Return ptr to buffer, or NULL if none available */
  return flow_buf;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_free
===========================================================================*/
/*!
@brief
  Release dynamic memory for flow info buffer

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_tc_flow_free
(
  netmgr_tc_flow_info_t **flow_info
)
{
  ds_dll_el_t *node = NULL;

  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_ASSERT(flow_info);
  NETMGR_ASSERT(*flow_info);

  netmgr_free((*flow_info)->class_handle);
  netmgr_free(*flow_info);
  *flow_info = NULL;

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_tc_handle_info_alloc
===========================================================================*/
/*!
@brief
  Allocate dynamic memory for handle info buffer

@return
  netmgr_tc_handle_info_t * - pointer to allocated buffer on success,
                              NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL netmgr_tc_handle_info_t*
netmgr_tc_handle_info_alloc()
{
  netmgr_tc_handle_info_t* handle = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  if ((handle = netmgr_malloc(sizeof(netmgr_tc_handle_info_t))) == NULL)
  {
    netmgr_log_err("netmgr_tc_handle_info_alloc: netmgr_malloc failed\n");
  }

  NETMGR_LOG_FUNC_EXIT;
  return handle;
}

/*===========================================================================
  FUNCTION  netmgr_tc_match_flows
===========================================================================*/
/*!
@brief
  Compares flow objects to determine if they match.  Used by
  linked-list search route.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL long int
netmgr_tc_match_flows
(
  const void * first,
  const void * second
)
{
  /* Return zero when the flows match, nonzero when they do not*/
  return ( ((netmgr_tc_flow_info_t*)first)->qos_flow.flow_id -
           ((netmgr_tc_flow_info_t*)second)->qos_flow.flow_id );
}

/*===========================================================================
  FUNCTION  netmgr_tc_create_root_qdisc
===========================================================================*/
/*!
@brief
  Create root queue discipline for specified link.
  The HTB qdisc is used for shaping and scheduling capabilities.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_root_qdisc
(
  int                       link,
  netmgr_tc_handle_info_t * class_handle
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int result =  NETMGR_FAILURE;
  int status;
  netmgr_tc_handle_info_t * root_handle = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  /* Configure the object handles */
  root_handle = netmgr_tc_handle_info_alloc();
  if( !root_handle )
  {
    netmgr_log_err("failed to allocate root handle for link %d", link);
    goto error;
  }

  root_handle->major = NETMGR_TC_QDISC_ROOT_MAJOR;
  root_handle->minor = 0;

  class_handle->major = root_handle->major;
  class_handle->minor = 1;

  /* Create root qdisc on network interface */
  if (netmgr_main_get_tcpackprio_enabled())
  {
      std_strlprintf(cmd, sizeof(cmd),
                     "tc qdisc add dev %s root handle %d:0 htb r2q %d default %d",
                     netmgr_kif_get_name( link ),
                     NETMGR_TC_QDISC_ROOT_MAJOR,
                     NETMGR_TC_QDISC_HTB_RATE2QTM,
                     NETMGR_TC_QDISC_DEFAULT_CLASS_TCP_ACK_PRIO);
  }
  else
  {
      std_strlprintf(cmd, sizeof(cmd),
                     "tc qdisc add dev %s root handle %d:0 htb r2q %d default %d",
                     netmgr_kif_get_name( link ),
                     NETMGR_TC_QDISC_ROOT_MAJOR,
                     NETMGR_TC_QDISC_HTB_RATE2QTM,
                     NETMGR_TC_QDISC_DEFAULT_CLASS);
  }

  result = ds_system_call(cmd, std_strlen(cmd));

  if (NETMGR_SUCCESS != result)
  {
    netmgr_log_err("failed to create root qdisc for link %d", link);
    goto error;
  }

  netmgr_tc_cfg.links[link].root_qdisc = root_handle;

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  netmgr_free( root_handle );
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_create_leaf_qdisc
===========================================================================*/
/*!
@brief
  Create leaf queue discipline for specified class link.
  The prio qdisc is used for scheduling capabilities.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_leaf_qdisc
(
  int       link,
  netmgr_tc_handle_info_t* parent
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int status;
  int result = NETMGR_SUCCESS;

  NETMGR_LOG_FUNC_ENTRY;

  snprintf(cmd, sizeof(cmd),
           "tc qdisc add dev %s parent %d:%d handle %d:0 prio flow enable",
           netmgr_kif_get_name( link ),
           parent->major,  /* parent major */
           parent->minor,  /* parent minor */
           parent->minor); /* qdisc major is minor of parent */

  result = ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_tc_create_class
===========================================================================*/
/*!
@brief
  Create internal class for specified link root qdisc.
  The HTB classs is used for shaping capabilities.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_class
(
  int          link,
  netmgr_tc_handle_info_t* parent,
  uint8        priority,
  uint32       datarate,
  netmgr_tc_handle_info_t* handle
)
{
  int result =  NETMGR_FAILURE;
  int status;
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT(handle);

  if( NULL == parent )
  {
    snprintf(cmd, sizeof(cmd),
             "tc class add dev %s parent root classid %d:%d "
             "htb prio %d rate %lubit ceil %lubit",
             netmgr_kif_get_name( link ),
             handle->major,              /* class major id */
             handle->minor,              /* class minor id */
             priority,                   /* priority level */
             ((datarate == 0)? NETMGR_TC_DEFAULT_DATARATE : datarate), /* rate in bps */
             NETMGR_TC_MAX_DATARATE);    /* ceiling in bps */
  }
  else
  {
    snprintf(cmd, sizeof(cmd),
             "tc class add dev %s parent %d:%d classid %d:%d "
             "htb prio %d rate %lubit ceil %lubit",
             netmgr_kif_get_name( link ),
             parent->major,              /* parent major id */
             parent->minor,              /* parent minor id */
             handle->major,              /* class major id */
             handle->minor,              /* class minor id */
             priority,                   /* priority level */
             ((datarate == 0)? NETMGR_TC_DEFAULT_DATARATE : datarate), /* rate in bps */
             NETMGR_TC_MAX_DATARATE);    /* ceiling in bps */
  }

  result = ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_tc_create_filter
===========================================================================*/
/*!
@brief
  Create filter (classifier) on specified link root qdisc to route
  packets MARKed with flow handle to designated class.  MARK is set
  using IPTables mangle rule to assign flowid to skb->mark so VED can
  access value.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_filter
(
  int                             link,
  netmgr_tc_flow_info_t         * flow_info,
  const netmgr_tc_handle_info_t * parent,
  const netmgr_tc_handle_info_t * class_info
)
{
  int result =  NETMGR_SUCCESS;
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int status;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT( flow_info );
  NETMGR_ASSERT( parent );
  NETMGR_ASSERT( class_info );

  snprintf(cmd, sizeof(cmd),
           "tc filter add dev %s parent %d:%d "
           "prio %d protocol ip handle 0x%lx fw classid %d:%d",
           netmgr_kif_get_name( link ),
           parent->major,
           parent->minor,
           NETMGR_TC_FILTER_PRIORITY,
           flow_info->qos_flow.flow_id,
           class_info->major,
           class_info->minor);

  result = ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return result;
}



/*===========================================================================
  FUNCTION  netmgr_tc_create_tcp_ack_filter
===========================================================================*/
/*!
@brief
  Create u32 filter on specified link root qdisc to route packets to
  designated class. The filters ensures that only TCP ACKs are filtered
  and sent through the prioritized class.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_tcp_ack_filter
(
  int                             link,
  netmgr_tc_flow_info_t         * flow_info,
  const netmgr_tc_handle_info_t * parent,
  const netmgr_tc_handle_info_t * class_info
)
{
  int result =  NETMGR_SUCCESS;
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int status;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT( flow_info );
  NETMGR_ASSERT( parent );
  NETMGR_ASSERT( class_info );

  /* The following uses a u32 filter to filter TCP ACKs.
     ip protocol 6 ensures that it is an IPV4\IPV6 packet.
     0x05 0x0f at 0 ensures that the header lengh is 32 bits
     0x0000 0xffc0 at 2 ensures that the IP packet lengh is 52 bytes.
     0x10 0xff at 33 ensures that the TCP ACK bit is set.
 */
  snprintf(cmd, sizeof(cmd),
           "tc filter add dev %s parent %d:%d "
           "protocol ip prio %d u32 match ip protocol 6 0xff "
           "match u8 0x05 0x0f at 0 match u16 0x0000 0xffc0 at 2 "
           "match u8 0x10 0xff at 33 flowid %d:%d",
           netmgr_kif_get_name( link ),
           parent->major,
           parent->minor,
           NETMGR_TC_TCP_ACK_FILTER_PRIORITY,
           class_info->major,
           class_info->minor);

  result = ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return result;
}


/*===========================================================================
  FUNCTION  netmgr_tc_find_flow_info
===========================================================================*/
/*!
@brief
  Search through flow list to find the specified flow info structure.

@return
  flow info if found, NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
netmgr_tc_flow_info_t*
netmgr_tc_find_flow_info
(
  int link,
  uint32 flow_id
)
{
  ds_dll_el_t* node = NULL;
  netmgr_tc_flow_info_t flow_buf;
  netmgr_tc_flow_info_t* flow_info = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  flow_buf.qos_flow.flow_id = flow_id;
  node = ds_dll_search(netmgr_tc_cfg.links[link].flow_list,
                       &flow_buf,
                       netmgr_tc_match_flows);

  if (node == NULL)
  {
    netmgr_log_high("netmgr_tc_find_flow_info: flow 0x%lx not found", flow_id);
    NETMGR_LOG_FUNC_EXIT;
    return NULL;
  }

  flow_info = (netmgr_tc_flow_info_t*) ds_dll_data(node);

  if (flow_info == NULL)
  {
    netmgr_log_err("netmgr_tc_find_flow_info: flow 0x%ld data not found", flow_id);
  }

  NETMGR_LOG_FUNC_EXIT;
  return flow_info;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_enable
===========================================================================*/
/*!
@brief
  Install the flow traffic control elements.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_flow_enable
(
  int link,
  netmgr_tc_flow_info_t *flow_info
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int status;
  int i;
  int result = NETMGR_FAILURE;
  char * ipt_tool = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  if (flow_info == NULL)
  {
    netmgr_log_err( "NULL flow info pointer, ignoring\n" );
    return result;
  }

  /* Link all the chains into the mangle table. */
  for (i = 0; i < flow_info->qos_flow.num_filter; i++)
  {
    ipt_tool = (QMI_QOS_IP_VERSION_6 == flow_info->qos_flow.filter_list[i].qos_filter.ip_version)?
                NETMGR_TC_IPTABLES_TOOL_V6 : NETMGR_TC_IPTABLES_TOOL_V4;

    snprintf(cmd, sizeof(cmd), "%s -t mangle -A OUTPUT -j %s_0x%08lx.%d",
             ipt_tool,
             NETMGR_TC_CHAIN_NAME_PREFIX,
             flow_info->qos_flow.flow_id,
             i);

    if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
    {
      goto error;
    }
  }

  flow_info->qos_flow.state = NETMGR_TC_FLOW_ACTIVE;
  result = NETMGR_SUCCESS;

error:
  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_tc_add_matching_rule_postrouting
===========================================================================*/
/*!
@brief
  Adds a matching rule in postrouting. All other packets which are not
  marked with qcom qos will be marked as 0x0, which is the default mark
  value for qos.

  This function can enter duplicable entries of the same rule in
  the postrouting chain. This happens in the case when there are multiple
  filters associated with the same flow. These however will be deleted
  eventually when the flow is deleted.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
netmgr_tc_add_matching_rule_postrouting
(
  int link,
  uint32 ip_family,
  uint32 flow_id
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int length;
  char * ipt_tool = NULL;
  const unsigned int maxlength = sizeof(cmd)-1;

  NETMGR_LOG_FUNC_ENTRY;

  if (ip_family != NETMGR_IPV6_ADDR &&
        ip_family != NETMGR_IPV4_ADDR )
  {
    netmgr_log_err("invalid IP version revceived");
    goto error;
  }

  ipt_tool = (NETMGR_IPV6_ADDR == ip_family)?
                NETMGR_TC_IPTABLES_TOOL_V6 : NETMGR_TC_IPTABLES_TOOL_V4;

  memset( cmd, 0x0, sizeof(cmd) );
  length =  snprintf(cmd,
                     maxlength,
                     "%s -t mangle -I %s -o %s -m mark --mark  0x%08lx -j ACCEPT",
                     ipt_tool,
                     NETMGR_TC_CHAIN_NAME_POSTROUTING,
                     netmgr_kif_get_name(link),
                     flow_id);

  if( (unsigned int)length >= maxlength )
     goto error;

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_iptables_v4_rule_helper
===========================================================================*/
/*!
@brief
  Helper function to create the iptables V4 rule

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_iptables_v4_rule_helper
(
  int link,
  uint32 flow_id,
  uint32 rule_id,
  const qmi_qos_granted_filter_data_type * qos_filter,
  int   protocol
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int length;
  const unsigned int maxlength = sizeof(cmd)-1;
  int range;
  int start_port;

  /* Build the new rule string and add to the parent chain*/
  memset( cmd, 0x0, sizeof(cmd) );
  length = snprintf(cmd, maxlength, "iptables -t mangle -A %s_0x%08lx.%ld",
                    NETMGR_TC_CHAIN_NAME_PREFIX,
                    flow_id,
                    rule_id);

  if( (unsigned int)length >= maxlength )
    goto error;

  /* Process address and subnet mask in network-byte order */
  if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_SRC_ADDR)
  {
    length += snprintf(cmd + length, maxlength - length, " --src %d.%d.%d.%d/%d.%d.%d.%d",
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_ip_addr & 0xFF000000) >> 24,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_ip_addr & 0x00FF0000) >> 16,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_ip_addr & 0x0000FF00) >> 8,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_ip_addr & 0x000000FF) >> 0,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_subnet_mask & 0xFF000000) >> 24,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_subnet_mask & 0x00FF0000) >> 16,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_subnet_mask & 0x0000FF00) >> 8,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.src_addr.ipv4_subnet_mask & 0x000000FF) >> 0 );

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_DEST_ADDR)
  {
    length += snprintf(cmd + length, maxlength - length, " --dst %d.%d.%d.%d/%d.%d.%d.%d",
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_ip_addr & 0xFF000000) >> 24,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_ip_addr & 0x00FF0000) >> 16,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_ip_addr & 0x0000FF00) >> 8,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_ip_addr & 0x000000FF) >> 0,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_subnet_mask & 0xFF000000) >> 24,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_subnet_mask & 0x00FF0000) >> 16,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_subnet_mask & 0x0000FF00) >> 8,
                       (unsigned int) (qos_filter->qos_filter.filter_desc.dest_addr.ipv4_subnet_mask & 0x000000FF) >> 0 );

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  if (protocol != NETMGR_TC_INVALID_PROTO)
  {
    length += snprintf(cmd + length, maxlength - length, " --protocol %d", protocol);

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_TOS)
  {
    length += snprintf(cmd + length, maxlength - length, " -m tos --tos %d",
                       qos_filter->qos_filter.filter_desc.tos.tos_value);

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  do
  {
    if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_TCP_SRC_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.tcp_src_ports.range;
      start_port = qos_filter ->qos_filter.filter_desc.tcp_src_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_UDP_SRC_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.udp_src_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.udp_src_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask &
               QMI_QOS_FILTER_PARAM_TRANSPORT_SRC_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.transport_src_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.transport_src_ports.start_port;
    }
    else
    {
      break;
    }

    if (range == 0)
      length += snprintf(cmd + length, maxlength - length, " --sport %d",
                         start_port);
    else
      length += snprintf(cmd + length, maxlength - length, " --sport %d:%d",
                         start_port,
                         start_port + range);

    if( (unsigned int)length >= maxlength )
      goto error;
  } while (0);

  do
  {
    if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_TCP_DEST_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.tcp_dest_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.tcp_dest_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_UDP_DEST_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.udp_dest_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.udp_dest_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask &
               QMI_QOS_FILTER_PARAM_TRANSPORT_DEST_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.transport_dest_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.transport_dest_ports.start_port;
    }
    else
    {
      break;
    }

    if (range == 0)
      length += snprintf(cmd + length,
                         maxlength - length,
                         " --dport %d",
                         start_port);
    else
      length += snprintf(cmd + length,
                         maxlength - length,
                         " --dport %d:%d",
                         start_port,
                         start_port + range);

    if( (unsigned int)length >= maxlength )
      goto error;
  } while (0);

  /* The mark value is the flow ID. */
  length += snprintf(cmd + length,
                     maxlength - length,
                     " -j MARK --set-mark 0x%08lx",
                     flow_id);

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  /* Once marked as desired, dont fall through,
   * ACCEPT and jump out of OUTPUT chain
   */
  memset( cmd, 0x0, sizeof(cmd) );
  length = snprintf(cmd,
                    maxlength,
                    "iptables -t mangle -A %s_0x%08lx.%ld",
                    NETMGR_TC_CHAIN_NAME_PREFIX,
                    flow_id,
                    rule_id);
  if( (unsigned int)length >= maxlength )
     goto error;

  length += snprintf(cmd + length,
                     maxlength - length,
                     " -m mark --mark  0x%08lx -j ACCEPT",
                     flow_id);
  if( (unsigned int)length >= maxlength )
     goto error;

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  /*Install mark match rule in the postrouting chain.*/
  netmgr_tc_add_matching_rule_postrouting(link,NETMGR_IPV4_ADDR, flow_id);

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}


/*===========================================================================
  FUNCTION  netmgr_tc_create_iptables_rule_v4
===========================================================================*/
/*!
@brief
  Create netfilter mangel table OUTPUT rule for QoS IPv4 filter. This will
  assign skb->mark field to teh specified flow ID for packet
  forwarding via traffic control classifier(filter).

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_iptables_rule_v4
(
  int link,
  uint32 flow_id,
  uint32 rule_id,
  const qmi_qos_granted_filter_data_type * qos_filter
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  const unsigned int maxlength = sizeof(cmd)-1;
  int range;
  int start_port;

  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_ASSERT( qos_filter );

  /* Create a chain */
  memset( cmd, 0x0, sizeof(cmd) );
  snprintf(cmd, maxlength, "iptables -t mangle -N %s_0x%08lx.%ld",
           NETMGR_TC_CHAIN_NAME_PREFIX,
           flow_id,
           rule_id);

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  if (!(qos_filter->qos_filter.filter_desc.param_mask &
        QMI_QOS_FILTER_PARAM_TRANS_PROTOCOL))
  {
    if ((qos_filter->qos_filter.filter_desc.param_mask &
         (QMI_QOS_FILTER_PARAM_TRANSPORT_DEST_PORTS |
           QMI_QOS_FILTER_PARAM_TRANSPORT_SRC_PORTS)))
    {
      if (netmgr_tc_iptables_v4_rule_helper( link,
                                             flow_id,
                                             rule_id,
                                             qos_filter,
                                             NETMGR_TC_TCP_PROTO_IANA_NUM) != NETMGR_SUCCESS)
      {
        netmgr_log_err("create_iptables_rule_v4: Error creating a V4 TCP filter");
        goto error;
      }

      if (netmgr_tc_iptables_v4_rule_helper( link,
                                             flow_id,
                                             rule_id,
                                             qos_filter,
                                             NETMGR_TC_UDP_PROTO_IANA_NUM) != NETMGR_SUCCESS)
      {
        netmgr_log_err("create_iptables_rule_v4: Error creating a V4 TCP filter");
        goto error;
      }
    }
    else
    {
      if (netmgr_tc_iptables_v4_rule_helper( link,
                                             flow_id,
                                             rule_id,
                                             qos_filter,
                                             NETMGR_TC_INVALID_PROTO) != NETMGR_SUCCESS)
      {
        netmgr_log_err("create_iptables_rule_v4: Error creating a V4 TCP filter");
        goto error;
      }
    }
  }
  else
  {
    if (netmgr_tc_iptables_v4_rule_helper( link,
                                           flow_id,
                                           rule_id,
                                           qos_filter,
                                           qos_filter->qos_filter.filter_desc.protocol) != NETMGR_SUCCESS)
    {
      netmgr_log_err("create_iptables_rule_v4: Error creating a V4 filter");
      goto error;
    }
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  /* Deleting the chain deletes all the rules.  This chain must not be referenced by any other rules. */
  snprintf(cmd, sizeof(cmd), "iptables -t mangle -X %s_0x%08lx.%ld",
           NETMGR_TC_CHAIN_NAME_PREFIX,
           flow_id,
           rule_id);
  (void) ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_iptables_v6_rule_helper
===========================================================================*/
/*!
@brief
  Helper function to create the iptables V6 rule

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_iptables_v6_rule_helper
(
  int link,
  uint32 flow_id,
  uint32 rule_id,
  const qmi_qos_granted_filter_data_type * qos_filter,
  int protocol
)
{

  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int length;
  const unsigned int maxlength = sizeof(cmd)-1;
  int range;
  int start_port;

  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_ASSERT( qos_filter );

  /* Build the new rule string */
  memset( cmd, 0x0, sizeof(cmd) );
  length = snprintf(cmd, maxlength, "ip6tables -t mangle -A %s_0x%08lx.%ld",
                    NETMGR_TC_CHAIN_NAME_PREFIX,
                    flow_id,
                    rule_id);

  if( (unsigned int)length >= maxlength )
    goto error;

  /* Process address and subnet mask in network-byte order */
  if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_IPV6_SRC_ADDR)
  {
    length += snprintf(cmd + length, maxlength - length,
                       " --src %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/%d",
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[0],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[1],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[2],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[3],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[4],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[5],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[6],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[7],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[8],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[9],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[10],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[11],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[12],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[13],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[14],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr[15],
                       qos_filter->qos_filter.filter_desc.ipv6_src_addr.ipv6_filter_prefix_len);

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_IPV6_DEST_ADDR)
  {
    length += snprintf(cmd + length, maxlength - length,
                       " --dst %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x/%d",
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[0],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[1],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[2],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[3],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[4],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[5],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[6],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[7],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[8],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[9],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[10],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[11],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[12],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[13],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[14],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr[15],
                       qos_filter->qos_filter.filter_desc.ipv6_dest_addr.ipv6_filter_prefix_len);

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  if (protocol != NETMGR_TC_INVALID_PROTO)
  {
    length += snprintf(cmd + length, maxlength - length, " --protocol %d", protocol);

    if( (unsigned int)length >= maxlength )
      goto error;
  }

  do
  {
    if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_TCP_SRC_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.tcp_src_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.tcp_src_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_UDP_SRC_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.udp_src_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.udp_src_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask &
               QMI_QOS_FILTER_PARAM_TRANSPORT_SRC_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.transport_src_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.transport_src_ports.start_port;
    }
    else
    {
      break;
    }

    if (range == 0)
      length += snprintf(cmd + length, maxlength - length, " --sport %d",
                         start_port);
    else
      length += snprintf(cmd + length, maxlength - length, " --sport %d:%d",
                         start_port,
                         start_port + range);

    if( (unsigned int)length >= maxlength )
      goto error;
  } while (0);

  do
  {
    if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_TCP_DEST_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.tcp_dest_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.tcp_dest_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask & QMI_QOS_FILTER_PARAM_UDP_DEST_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.udp_dest_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.udp_dest_ports.start_port;
    }
    else if (qos_filter->qos_filter.filter_desc.param_mask &
                QMI_QOS_FILTER_PARAM_TRANSPORT_DEST_PORTS)
    {
      range = qos_filter->qos_filter.filter_desc.transport_dest_ports.range;
      start_port = qos_filter->qos_filter.filter_desc.transport_dest_ports.start_port;
    }
    else
    {
      break;
    }

    if (range == 0)
      length += snprintf(cmd + length, maxlength - length, " --dport %d",
                         start_port);
    else
      length += snprintf(cmd + length, maxlength - length, " --dport %d:%d",
                         start_port,
                         start_port + range);

    if( (unsigned int)length >= maxlength )
      goto error;
  } while (0);

  /* The mark value is the flow ID. */
  length += snprintf(cmd + length, maxlength - length, " -j MARK --set-mark 0x%08lx", flow_id);

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  /* Once marked as desired, dont fall through, ACCEPT and jump out of OUTPUT chain */
  memset( cmd, 0x0, sizeof(cmd) );
  length = snprintf(cmd, maxlength, "ip6tables -t mangle -A %s_0x%08lx.%ld",
                    NETMGR_TC_CHAIN_NAME_PREFIX,
                    flow_id,
                    rule_id);
  if( (unsigned int)length >= maxlength )
     goto error;

  length += snprintf(cmd + length, maxlength - length, " -m mark --mark  0x%08lx -j ACCEPT", flow_id);
  if( (unsigned int)length >= maxlength )
     goto error;

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  netmgr_tc_add_matching_rule_postrouting(link,NETMGR_IPV6_ADDR, flow_id);

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_create_iptables_rule_v6
===========================================================================*/
/*!
@brief
  Create netfilter mangel table OUTPUT rule for QoS IPv6 filter. This will
  assign skb->mark field to teh specified flow ID for packet
  forwarding via traffic control classifier(filter).

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_create_iptables_rule_v6
(
  int link,
  uint32 flow_id,
  uint32 rule_id,
  const qmi_qos_granted_filter_data_type * qos_filter
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int length;
  const unsigned int maxlength = sizeof(cmd)-1;

  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_ASSERT( qos_filter );

  /* Create a chain */
  memset( cmd, 0x0, sizeof(cmd) );
  snprintf(cmd, maxlength, "ip6tables -t mangle -N %s_0x%08lx.%ld",
           NETMGR_TC_CHAIN_NAME_PREFIX,
           flow_id,
           rule_id);

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  if (!(qos_filter->qos_filter.filter_desc.param_mask &
        QMI_QOS_FILTER_PARAM_TRANS_PROTOCOL))
  {
    if ((qos_filter->qos_filter.filter_desc.param_mask &
         (QMI_QOS_FILTER_PARAM_TRANSPORT_DEST_PORTS |
           QMI_QOS_FILTER_PARAM_TRANSPORT_SRC_PORTS)))
    {
      if (netmgr_tc_iptables_v6_rule_helper( link,
                                             flow_id,
                                             rule_id,
                                             qos_filter,
                                             NETMGR_TC_TCP_PROTO_IANA_NUM) != NETMGR_SUCCESS)
      {
        netmgr_log_err("create_iptables_rule_v6: Error creating a V6 TCP filter");
        goto error;
      }

      if (netmgr_tc_iptables_v6_rule_helper( link,
                                             flow_id,
                                             rule_id,
                                             qos_filter,
                                             NETMGR_TC_UDP_PROTO_IANA_NUM) != NETMGR_SUCCESS)
      {
        netmgr_log_err("create_iptables_rule_v6: Error creating a V6 TCP filter");
        goto error;
      }
    }
    else
    {
      if (netmgr_tc_iptables_v6_rule_helper( link,
                                             flow_id,
                                             rule_id,
                                             qos_filter,
                                             NETMGR_TC_INVALID_PROTO) != NETMGR_SUCCESS)
      {
        netmgr_log_err("create_iptables_rule_v6: Error creating a V6 TCP filter");
        goto error;
      }
    }
  }
  else
  {
    if (netmgr_tc_iptables_v6_rule_helper( link,
                                           flow_id,
                                           rule_id,
                                           qos_filter,
                                           qos_filter->qos_filter.filter_desc.protocol) != NETMGR_SUCCESS)
    {
      netmgr_log_err("create_iptables_rule_v6: Error creating a V6 filter");
      goto error;
    }
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  /* Deleting the chain deletes all the rules.  This chain must not be referenced by any other rules. */
  snprintf(cmd, sizeof(cmd), "ip6tables -t mangle -X %s_0x%08lx.%ld",
           NETMGR_TC_CHAIN_NAME_PREFIX,
           flow_id,
           rule_id);
  (void) ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_create
===========================================================================*/
/*!
@brief
 Create flow object and install the traffic control elements.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_flow_create
(
  int                                link,
  const netmgr_qmi_qos_flow_info_t  *qos_flow
)
{
  int i;
  int result;
  netmgr_tc_flow_info_t * flow_info = NULL;
  netmgr_tc_handle_info_t * class_handle = NULL;
  netmgr_tc_handle_info_t * rclass_handle = NULL;
  netmgr_tc_handle_info_t * tcp_ack_class_handle = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT( qos_flow );

  /* Allocate a flow info buffer */
  flow_info = netmgr_tc_flow_alloc();
  NETMGR_ASSERT( flow_info );

  /* Assign flow attributes */
  flow_info->qos_flow = *qos_flow;

  /* Allocate handle for classes */
  class_handle = netmgr_tc_handle_info_alloc();
  if( !class_handle )
  {
    netmgr_log_err("failed to allocate class handle for flow 0x%lx", qos_flow->flow_id);
    goto error;
  }

  /* Check for primary flow */
  if( NETMGR_IS_DEFAULT_FLOW( qos_flow->flow_id ) )
  {
    rclass_handle = netmgr_tc_handle_info_alloc();
    if( !rclass_handle )
    {
      netmgr_log_err("failed to allocate root class handle for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    /* Install kernel traffic control elements for primary flow */
    /* Create root HTB qdisc */
    result = netmgr_tc_create_root_qdisc(link, rclass_handle);

    if (result == NETMGR_FAILURE)
    {
      netmgr_log_err("error creating root qdisc for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    /* Create root class, using maximum datarate */
    result = netmgr_tc_create_class(link,
                                    NULL,  /* indicates root */
                                    0,     /* priority irrelevant on root */
                                    NETMGR_TC_MAX_DATARATE,
                                    rclass_handle);

    if (result == NETMGR_FAILURE)
    {
      netmgr_log_err("error creating default class for link %d", link);
      goto error;
    }

    if (netmgr_main_get_tcpackprio_enabled())
    {
        /* Create child class for TCP ack flow */
        tcp_ack_class_handle = netmgr_tc_handle_info_alloc();
        if( !tcp_ack_class_handle )
        {
          netmgr_log_err("failed to allocate class handle for flow 0x%lx", qos_flow->flow_id);
          goto error;
        }

        tcp_ack_class_handle->major = rclass_handle->minor;
        NETMGET_TC_ASSIGN_CLASS_MINOR_ID(link, tcp_ack_class_handle->minor);

        result = netmgr_tc_create_class(link,
                                        rclass_handle,
                                        0,       /* Highest priority for TCP Ack's*/
                                        NETMGR_TC_MAX_DATARATE,
                                        tcp_ack_class_handle);
        if (result == NETMGR_FAILURE)
        {
          netmgr_log_err("error creating class for flow 0x%lx", qos_flow->flow_id);
          goto error;
        }

        netmgr_tc_cfg.links[link].tcp_ack_class = tcp_ack_class_handle;

        /* Create leaf qdisc */
        result = netmgr_tc_create_leaf_qdisc(link, tcp_ack_class_handle);

        if (result == NETMGR_FAILURE)
        {
          netmgr_log_err("error creating leaf qdisc for flow 0x%lx", qos_flow->flow_id);
          goto error;
        }
    }

    /* Create child class for default flow */
    class_handle->major = rclass_handle->minor;
    NETMGET_TC_ASSIGN_CLASS_MINOR_ID(link, class_handle->minor);

    result = netmgr_tc_create_class(link,
                                    rclass_handle,
                                    qos_flow->priority,
                                    NETMGR_TC_MAX_DATARATE,
                                    class_handle);
    if (result == NETMGR_FAILURE)
    {
      netmgr_log_err("error creating class for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    netmgr_tc_cfg.links[link].root_class = rclass_handle;
    netmgr_tc_cfg.links[link].default_class = class_handle;
    flow_info->class_handle = class_handle;

    /* Create leaf qdisc */
    result = netmgr_tc_create_leaf_qdisc(link, flow_info->class_handle);

    if (result == NETMGR_FAILURE)
    {
      netmgr_log_err("error creating leaf qdisc for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    if (netmgr_main_get_tcpackprio_enabled())
    {
        /* Create a filter for TCP Ack's to go over default flow  */
        result = netmgr_tc_create_tcp_ack_filter(link,
                                                 flow_info,
                                                 netmgr_tc_cfg.links[link].root_qdisc,
                                                 tcp_ack_class_handle);

        if (result == NETMGR_FAILURE)
        {
          netmgr_log_err("error creating leaf qdisc for flow 0x%lx", qos_flow->flow_id);
          goto error;
        }
    }
    /* Add flow object to list for this link */
    ds_dll_enq( netmgr_tc_cfg.links[link].flow_list, NULL, (void*)flow_info );

    /* Primary flow does not have classifier/filter, relies only on
       root qdisc default class assignment. */
  }
  else
  {
    /* Check whether root class is non-null */
    if( NULL == netmgr_tc_cfg.links[link].root_class )
    {
      netmgr_log_err("Root class is NULL for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    /* Install kernel traffic control elements for secondary flow */
    class_handle->major = netmgr_tc_cfg.links[link].root_class->minor;
    NETMGET_TC_ASSIGN_CLASS_MINOR_ID(link, class_handle->minor);

    /* Create child class */
    result = netmgr_tc_create_class(link,
                                    netmgr_tc_cfg.links[link].root_class,
                                    qos_flow->priority,
                                    qos_flow->datarate,
                                    class_handle);

    if (result == NETMGR_FAILURE)
    {
      netmgr_log_err("error creating class for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    flow_info->class_handle = class_handle;

    /* Create leaf qdisc */
    result = netmgr_tc_create_leaf_qdisc(link, flow_info->class_handle);

    if (result == NETMGR_FAILURE)
    {
      netmgr_log_err("error creating leaf qdisc for flow 0x%lx", qos_flow->flow_id);
      goto error;
    }

    /* direct matching egress traffic to the new class */
    netmgr_tc_create_filter(link,
                            flow_info,
                            netmgr_tc_cfg.links[link].root_qdisc,
                            flow_info->class_handle);

    /* Mark all matching packets with the value of the link */
    for (i = 0; i < qos_flow->num_filter; i++)
    {
      switch( qos_flow->filter_list[i].qos_filter.ip_version )
      {
        case QMI_QOS_IP_VERSION_4:
          netmgr_tc_create_iptables_rule_v4( link,flow_info->qos_flow.flow_id,
                                             i,
                                             &qos_flow->filter_list[i] );
          break;
        case QMI_QOS_IP_VERSION_6:
          netmgr_tc_create_iptables_rule_v6( link,flow_info->qos_flow.flow_id,
                                             i,
                                             &qos_flow->filter_list[i] );
          break;
        default:
          netmgr_log_err( "unsupported filter IP version[%d]",
                          qos_flow->filter_list[i].qos_filter.ip_version );
          break;
      }

    }

    /* Add flow object to list for this link */
    ds_dll_enq( netmgr_tc_cfg.links[link].flow_list, NULL, (void*)flow_info );

    /* Link the chain into the mangle table */
    if (netmgr_tc_flow_enable( link, flow_info ) != NETMGR_SUCCESS)
    {
      goto error;
    }
  }

  flow_info->qos_flow.state = NETMGR_TC_FLOW_ACTIVE;

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  netmgr_tc_flow_free( &flow_info );
  netmgr_free( class_handle );
  netmgr_free( rclass_handle );
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_control_hdlr
===========================================================================*/
/*!
@brief
  Enable/disable packet scheduling from specific leaf qdisc.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_flow_control_hdlr
(
  int link,
  uint32 flow_id,
  netmgr_tc_flow_state_t state
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int status;
  netmgr_tc_flow_info_t* flow_info;
  int i;
  int result = NETMGR_SUCCESS;
  char tcm_str[NETMGR_TC_HANDLE_LEN];
  uint32 tcm_handle;

  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_ASSERT(link >= 0 && link < netmgr_tc_cfg.nlink);

  flow_info = netmgr_tc_find_flow_info(link, flow_id);
  if (flow_info == NULL)
  {
    result = NETMGR_FAILURE;
    goto error;
  }

  /* tcm handle is a string representation of a hex (base 16)
   * number that needs to be left shifted by 4 hex digits to
   * derive the handle that is used by kernel */
  memset(tcm_str, 0, NETMGR_TC_HANDLE_LEN);
  snprintf(tcm_str,sizeof(tcm_str),"%d",flow_info->class_handle->minor);
  tcm_handle = strtoul(tcm_str, NULL, 16);
  tcm_handle <<= 16;

  netmgr_kif_ifioctl_flow_control(netmgr_kif_get_name(link),
                                  tcm_handle,
                                  (NETMGR_TC_FLOW_DISABLED == state)?0:1);

  if ((NETMGR_IS_DEFAULT_FLOW( flow_id )) && (netmgr_main_get_tcpackprio_enabled()))
  {
     memset(tcm_str, 0, NETMGR_TC_HANDLE_LEN);
     snprintf(tcm_str,sizeof(tcm_str),"%d",netmgr_tc_cfg.links[link].tcp_ack_class->minor);
     tcm_handle = strtoul(tcm_str, NULL, 16);
     tcm_handle <<= 16;

     netmgr_kif_ifioctl_flow_control(netmgr_kif_get_name(link),
                                     tcm_handle,
                                     (NETMGR_TC_FLOW_DISABLED == state)?0:1);
  }
error:
  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_filter_dump
===========================================================================*/
/*!
@brief
  Writes data in Filters to QXDM logs

@return
  None.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/

static void
netmgr_tc_flow_filter_dump
(
  const netmgr_qmi_qos_flow_info_t  *qos_flow
)
{

  int tmp_cntr =0;
  qmi_qos_granted_filter_data_type *fltr_lst;

  if (qos_flow == NULL)
    return;

  netmgr_log_low("flow handle=0x%08x\n",
                 (unsigned int)qos_flow->flow_id );
  netmgr_log_low("flow is_new=%s\n",
                 ((TRUE==qos_flow->is_new)?"TRUE":"FALSE") );
  netmgr_log_low("flow priority=%d\n", qos_flow->priority );
  netmgr_log_low("flow datarate=%d\n", (int)qos_flow->datarate );
  netmgr_log_low("flow num_filter=%d\n", qos_flow->num_filter );
  netmgr_log_low("flow filter_list=%p\n", qos_flow->filter_list );

  for (tmp_cntr = 0; tmp_cntr < qos_flow->num_filter; tmp_cntr++)
  {
    fltr_lst = &(qos_flow->filter_list[tmp_cntr]);
    netmgr_log_low( "flow filter_list addrs=%p\n", fltr_lst );

    netmgr_log_low("[%d] param mask = %d\n",
                    tmp_cntr, fltr_lst->qos_filter.filter_desc.param_mask);
    netmgr_log_low("[%d] filter_index = %d\n",
                    tmp_cntr, fltr_lst->filter_index);
    netmgr_log_low("[%d]Filter ID = %d\n",
                    tmp_cntr, fltr_lst->qos_filter.filter_desc.filter_id);
    netmgr_log_low("[%d] precedence = %d\n",
                    tmp_cntr, fltr_lst->qos_filter.filter_desc.precedence);
    netmgr_log_low("[%d] esp security poicy index = %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.esp_security_policy_index);
    netmgr_log_low("[%d] IP version = %d\n",
                    tmp_cntr, fltr_lst->qos_filter.ip_version);
    netmgr_log_low("[%d] IPV6 Flow Label = %d\n",
                     tmp_cntr,
                     fltr_lst->qos_filter.filter_desc.ipv6_flow_label);
    netmgr_log_low("[%d] IPV6 Src addr = %s\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.ipv6_src_addr.ipv6_ip_addr);
    netmgr_log_low("[%d] IPV6 Src addr frefix len = %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.ipv6_src_addr.ipv6_filter_prefix_len);
    netmgr_log_low("[%d] IPV6 Dest addr = %s\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.ipv6_dest_addr.ipv6_ip_addr);
    netmgr_log_low("[%d] IPV6 Dest addr frefix len = %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.ipv6_dest_addr.ipv6_filter_prefix_len);
    netmgr_log_low("[%d] IPV6 Traffic class Value = %d, Mask %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.ipv6_traffic_class.traffic_class_value,
                    fltr_lst->qos_filter.filter_desc.ipv6_traffic_class.traffic_class_mask);
    netmgr_log_low("[%d] Protocol = %d\n",
                    tmp_cntr, fltr_lst->qos_filter.filter_desc.protocol);
    netmgr_log_low("[%d] TOS Value = %d TOS Mask %d \n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.tos.tos_value,
                    fltr_lst->qos_filter.filter_desc.tos.tos_mask);
    netmgr_log_low("[%d] TCP SRC start port = %d, Range %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.tcp_src_ports.start_port,
                    fltr_lst->qos_filter.filter_desc.tcp_src_ports.range);
    netmgr_log_low("[%d] TCP Dest start port = %d, Range %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.tcp_dest_ports.start_port,
                    fltr_lst->qos_filter.filter_desc.tcp_dest_ports.range);
    netmgr_log_low("[%d] UDP SRC start port = %d, Range %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.udp_src_ports.start_port,
                    fltr_lst->qos_filter.filter_desc.udp_src_ports.range);
    netmgr_log_low("[%d] UDP Dest start port = %d, Range %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.udp_dest_ports.start_port,
                    fltr_lst->qos_filter.filter_desc.udp_dest_ports.range);
    netmgr_log_low("[%d] Transport SRC start port = %d, Range %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.transport_src_ports.start_port,
                    fltr_lst->qos_filter.filter_desc.transport_src_ports.range);
    netmgr_log_low("[%d] Transport Dest start port = %d, Range %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.transport_dest_ports.start_port,
                    fltr_lst->qos_filter.filter_desc.transport_dest_ports.range);
    netmgr_log_low("[%d] IPV4 SRC Addr  = %d, subnet %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.src_addr.ipv4_ip_addr,
                    fltr_lst->qos_filter.filter_desc.src_addr.ipv4_subnet_mask);
    netmgr_log_low("[%d] IPV4 DEST Addr  = %d, subnet %d\n",
                    tmp_cntr,
                    fltr_lst->qos_filter.filter_desc.dest_addr.ipv4_ip_addr,
                    fltr_lst->qos_filter.filter_desc.dest_addr.ipv4_subnet_mask);
  }
}

/*===========================================================================
  FUNCTION  netmgr_tc_filter_delete_hdlr
===========================================================================*/
/*!
@brief
 Remove the iptable filter rules associated with a leaf class of HTB

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note
  The caller is assumed to delete the flow object from  flow_list dll.


  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_tc_filter_delete_hdlr
(
  netmgr_tc_flow_info_t *flow_info,
  int       link,
  uint32    flow_id
)
{
  int i;
  char * ipt_tool = NULL;
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];

  if (flow_info == NULL)
  {
    netmgr_log_err("filter_delete_hdlr: invalid INPUT parameter received");
    return;
  }

  for (i = 0; i < flow_info->qos_flow.num_filter; i++)
  {
    ipt_tool = (QMI_QOS_IP_VERSION_6 == flow_info->qos_flow.filter_list[i].
                 qos_filter.ip_version)? NETMGR_TC_IPTABLES_TOOL_V6 :
                 NETMGR_TC_IPTABLES_TOOL_V4;

    /* Remove the reference to the chain from the output table */
    snprintf(cmd, sizeof(cmd), "%s -t mangle -D OUTPUT -j %s_0x%08lx.%d",
             ipt_tool,
             NETMGR_TC_CHAIN_NAME_PREFIX,
             flow_id,
             i);
    if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
    {
      netmgr_log_high("Error removing rule from OUTPUT table. Ignoring");
    }

    /* Remove all the rules from the chain */
    snprintf(cmd, sizeof(cmd), "%s -t mangle -F %s_0x%08lx.%d",
             ipt_tool,
             NETMGR_TC_CHAIN_NAME_PREFIX,
             flow_id,
             i);
    if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
    {
      netmgr_log_high("Error flushing chain. Ignoring");
    }

    /* Delete the chain */
    snprintf(cmd, sizeof(cmd), "%s -t mangle -X %s_0x%08lx.%d",
             ipt_tool,
             NETMGR_TC_CHAIN_NAME_PREFIX,
             flow_id,
             i);
    if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
    {
      netmgr_log_high("Error deleting chain. Ignoring");
    }
    /*Delete the corresponding  matching rule from postrouting chain */
    snprintf(cmd,
             sizeof(cmd),
             "%s -t mangle -D %s -o %s -m mark --mark 0x%08lx -j ACCEPT",
             ipt_tool,
             NETMGR_TC_CHAIN_NAME_POSTROUTING,
             netmgr_kif_get_name(link),
             flow_id);

    if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
    {
      netmgr_log_high("Error deleting rule from postrouting. Ignoring");
    }
  }
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_modify_hdlr
===========================================================================*/
/*!
@brief
 Change the traffic control elements on Modem QoS flow modify.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_flow_modify_hdlr
(
  int       link,
  const netmgr_qmi_qos_flow_info_t  *qos_flow
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int result = NETMGR_FAILURE;
  unsigned int length = 0;
  int i = 0;
  netmgr_tc_flow_info_t *flow_info = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT(link >= 0 && link < netmgr_tc_cfg.nlink);
  NETMGR_ASSERT(qos_flow != NULL);

  flow_info = netmgr_tc_find_flow_info(link, qos_flow->flow_id);

  if (flow_info == NULL)
  {
    netmgr_log_err("flow_modify_hdlr: the flow object does not exist");
    goto error;
  }

  /*disble flow on the leaf qdisc before modifying parent class*/
  if (NETMGR_SUCCESS != ( netmgr_tc_flow_control_hdlr(link,
                                                      qos_flow->flow_id,
                                                      NETMGR_TC_FLOW_DISABLED)))
  {
    netmgr_log_err("could not disable exisiting flow");
    goto error;
  }

  length = snprintf(cmd,
                    sizeof(cmd),
                    "tc qdisc replace dev %s parent %d:%d classid %d:%d"
                    "htb prio %d rate %lubit ceil %lubit",
                    netmgr_kif_get_name( link ),
                    netmgr_tc_cfg.links[link].root_class->major,
                    netmgr_tc_cfg.links[link].root_class->minor,
                    flow_info->class_handle->major,
                    flow_info->class_handle->minor,
                    qos_flow->priority,
                    ((qos_flow->datarate == 0)? NETMGR_TC_DEFAULT_DATARATE :
                       qos_flow->datarate),
                    NETMGR_TC_MAX_DATARATE);

  if (length >= sizeof(cmd))
  {
    netmgr_log_err("command message truncated;");
    goto error;
  }

  if (NETMGR_SUCCESS != (ds_system_call(cmd, strlen(cmd))))
  {
    netmgr_log_err("could not modify the leaf qdisc class");
    goto error;
  }

  netmgr_tc_filter_delete_hdlr(flow_info,
                               link,
                               flow_info->qos_flow.flow_id);

  /* Since the flowID does not change, there is no need to
   * change the tc filter installed at the root
   */

  /* Mark all matching packets with the value of the link */
  for (i = 0; i < qos_flow->num_filter; i++)
  {
    switch( qos_flow->filter_list[i].qos_filter.ip_version )
    {
      case QMI_QOS_IP_VERSION_4:
        netmgr_tc_create_iptables_rule_v4(link, flow_info->qos_flow.flow_id,
                                           i,
                                           &qos_flow->filter_list[i] );
        break;
      case QMI_QOS_IP_VERSION_6:
        netmgr_tc_create_iptables_rule_v6(link, flow_info->qos_flow.flow_id,
                                           i,
                                           &qos_flow->filter_list[i] );
        break;
      default:
        netmgr_log_err( "unsupported filter IP version[%d]",
                        qos_flow->filter_list[i].qos_filter.ip_version );
        break;
    }
  }

  /* Link the chain into the mangle table */
  if (netmgr_tc_flow_enable( link, flow_info ) != NETMGR_SUCCESS)
  {
    netmgr_log_err("error in linking iptables chain to mangle table");
    goto error;
  }

  /*replace old qos spec with the new one*/
  flow_info->qos_flow = *qos_flow;

  /* If we reach here QOS modification is successul,
   * re-enable the leaf qdisc at this point
   */
  if (NETMGR_SUCCESS != ( netmgr_tc_flow_control_hdlr(link,
                                                      qos_flow->flow_id,
                                                      NETMGR_TC_FLOW_ACTIVE)))
  {
    netmgr_log_err("could not re-enable flow");
    goto error;
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;

error:
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_activate_hdlr
===========================================================================*/
/*!
@brief
  Install the traffic control elements on Modem QoS flow create/resume.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_flow_activate_hdlr
(
  int                                link,
  const netmgr_qmi_qos_flow_info_t  *qos_flow
)
{
  int result = NETMGR_FAILURE;
  netmgr_tc_flow_info_t *flow_info = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT( qos_flow );
  NETMGR_ASSERT(link >= 0 && link < netmgr_tc_cfg.nlink);

  netmgr_log_low( "flow handle=0x%08x\n", (unsigned int)qos_flow->flow_id );
  netmgr_log_low( "flow is_new=%s\n", ((TRUE==qos_flow->is_new)?"TRUE":"FALSE") );
  netmgr_log_low( "flow priority=%d\n", qos_flow->priority );
  netmgr_log_low( "flow datarate=%d\n", (int)qos_flow->datarate );
  netmgr_log_low( "flow num_filter=%d\n", qos_flow->num_filter );
  netmgr_log_low( "flow filter_list=%p\n", qos_flow->filter_list );

  /* Find existing flow object.  Do this for supposedly new flows to
   * guard against repeated activation events. */
  flow_info = netmgr_tc_find_flow_info(link, qos_flow->flow_id);

  /* Check for newly created flow */
  if( qos_flow->is_new ) {
    /* Check for pre-existing flow ID */
    if( !flow_info )
    {
      /* Create flow object and install QoS measures */
      result = netmgr_tc_flow_create( link, qos_flow );
    }
    else
    {
      /* This may happen if previously a suspend or activated flow event
       * happened. In this case network may be updating either flow or filter
       * or both. Hence we need to modify the flow and filter spec installed
       * in the kernel
       */
       netmgr_log_med( "flow is already active, modifying flow.\n" );
       netmgr_tc_flow_modify_hdlr(link,qos_flow);
    }
  }
  else
  {
    if( flow_info )
    {
      /* This may happen if previously a suspend or activated flow event
       * happened. In this case network may be updating either flow or filter
       * or both. Hence we need to modify the flow and filter spec installed
       * in the kernel
      */
      netmgr_log_med( "flow is already active, modifying flow.\n" );
      netmgr_tc_flow_modify_hdlr(link,qos_flow);

      if ( NETMGR_TC_FLOW_ACTIVE != flow_info->qos_flow.state )
      {
        /* Resume QoS measures for a pre-existing flow */
        result = netmgr_tc_flow_enable( link, flow_info );
      }
    }
    else
    {
      netmgr_log_err( "Cannot find flow object[0x%x], ignoring\n", qos_flow->flow_id );
    }
  }

  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_tc_flow_delete_hdlr
===========================================================================*/
/*!
@brief
 Remove the traffic control elements on Modem QoS flow release.

@return
  int - NETMGR_SUCCESS on successful operation,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_flow_delete_hdlr
(
  int       link,
  uint32    flow_id
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  ds_dll_el_t* node = NULL;
  ds_dll_el_t* tail = NULL;
  netmgr_tc_flow_info_t flow_buf;
  netmgr_tc_flow_info_t* flow_info = NULL;
  const void* dummy = NULL;
  const void* data = NULL;
  int i;
  int result = NETMGR_FAILURE;
  char * ipt_tool = NULL;

  NETMGR_LOG_FUNC_ENTRY;
  NETMGR_ASSERT(link >= 0 && link < netmgr_tc_cfg.nlink);

  if (NETMGR_IS_DEFAULT_FLOW(flow_id))
  {
    if (netmgr_tc_cfg.links[link].default_class)
    {
      netmgr_log_med("Delete default class");
      /* Delete the default class */
      snprintf(cmd, sizeof(cmd), "tc class delete dev %s parent %d:%d "
               "classid %d:%d",
               netmgr_kif_get_name( link ),
               netmgr_tc_cfg.links[link].root_class->major,
               netmgr_tc_cfg.links[link].root_class->minor,
               netmgr_tc_cfg.links[link].default_class->major,
               netmgr_tc_cfg.links[link].default_class->minor);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_high("Error deleting default class. Ignoring");
      }

      netmgr_free(netmgr_tc_cfg.links[link].default_class);
      netmgr_tc_cfg.links[link].default_class = NULL;
    }

    if (netmgr_main_get_tcpackprio_enabled())
    {
        if (netmgr_tc_cfg.links[link].tcp_ack_class)
        {
          netmgr_log_med("Delete tcp ack class");
          /* Delete the default class */
          snprintf(cmd, sizeof(cmd), "tc class delete dev %s parent %d:%d "
                   "classid %d:%d",
                   netmgr_kif_get_name( link ),
                   netmgr_tc_cfg.links[link].root_class->major,
                   netmgr_tc_cfg.links[link].root_class->minor,
                   netmgr_tc_cfg.links[link].tcp_ack_class->major,
                   netmgr_tc_cfg.links[link].tcp_ack_class->minor);

          if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
          {
            netmgr_log_high("Error deleting tcp ack class. Ignoring");
          }

          netmgr_free(netmgr_tc_cfg.links[link].tcp_ack_class);
          netmgr_tc_cfg.links[link].tcp_ack_class = NULL;
        }
    }

    if (netmgr_tc_cfg.links[link].root_class)
    {
      netmgr_log_med("Delete root class");
      /* Delete the root class */
      snprintf(cmd, sizeof(cmd), "tc class delete dev %s parent %d:0 "
               "classid %d:%d",
               netmgr_kif_get_name( link ),
               NETMGR_TC_QDISC_ROOT_MAJOR,
               netmgr_tc_cfg.links[link].root_class->major,
               netmgr_tc_cfg.links[link].root_class->minor);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_high("Error deleting root class. Ignoring");
      }

      netmgr_free(netmgr_tc_cfg.links[link].root_class);
      netmgr_tc_cfg.links[link].root_class = NULL;
    }

    if (netmgr_tc_cfg.links[link].root_qdisc)
    {
      netmgr_log_med("Delete root qdisc");
      /* Delete the root qdisc */
      snprintf(cmd, sizeof(cmd), "tc qdisc delete dev %s root handle %d:0 "
               "htb r2q %d",
               netmgr_kif_get_name( link ),
               NETMGR_TC_QDISC_ROOT_MAJOR,
               NETMGR_TC_QDISC_HTB_RATE2QTM);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_high("Error deleting root qdisc. Ignoring");
      }

      netmgr_free(netmgr_tc_cfg.links[link].root_qdisc);
      netmgr_tc_cfg.links[link].root_qdisc = NULL;
    }

    /* Empty the flow list */
    netmgr_log_med("Empty the flow list");
    while (NULL != (node = ds_dll_deq(netmgr_tc_cfg.links[link].flow_list, NULL, &data)))
    {
      flow_info = (netmgr_tc_flow_info_t*) (data);
      if (flow_info)
      {
        netmgr_log_med("Freeing flow_id=0x%lx", flow_info->qos_flow.flow_id);

        /* The default class is already deleted above, so just free the flow_info */
        if (NETMGR_QMI_PRIMARY_FLOW_ID == flow_info->qos_flow.flow_id)
        {
          netmgr_free(flow_info);
        }
        else
        {
          netmgr_tc_flow_free(&flow_info);
        }
      }

      ds_dll_free(node);
    }

    /* Start with the initial MINOR_ID value */
    netmgr_tc_cfg.links[ link ].next_class_minor = NETMGR_TC_CLASS_MINOR_ID_INIT;

    result = NETMGR_SUCCESS;
  }
  else
  {
    /* find the tail of the flow list */
    node = ds_dll_next(netmgr_tc_cfg.links[link].flow_list, &dummy);
    while (NULL != node)
    {
      tail = node;
      node = ds_dll_next(tail, &dummy);
    }

    /* search for the flow object and remove it from the dll */
    flow_buf.qos_flow.flow_id = flow_id;
    node = ds_dll_delete(netmgr_tc_cfg.links[link].flow_list,
                         &tail,
                         &flow_buf,
                         netmgr_tc_match_flows);

    if (node == NULL)
    {
      netmgr_log_err("flow %lx not found", flow_id);
      goto done;
    }

    flow_info = (netmgr_tc_flow_info_t*) ds_dll_data(node);

    if (flow_info == NULL)
    {
      netmgr_log_err("flow %lx data not found", flow_id);
      goto done;
    }

    netmgr_tc_filter_delete_hdlr(flow_info, link, flow_id);

    if(netmgr_tc_cfg.links[link].root_class)
    {
      /* delete the filter */
      snprintf(cmd, sizeof(cmd), "tc filter delete dev %s parent %d:%d "
               "prio %d protocol ip handle 0x%lx fw classid %d:%d",
               netmgr_kif_get_name( link ),
               NETMGR_TC_QDISC_ROOT_MAJOR,
               NETMGR_TC_QDISC_ROOT_MINOR,
               NETMGR_TC_FILTER_PRIORITY,
               flow_id,
               flow_info->class_handle->major,
               flow_info->class_handle->minor);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_high("Error deleting filter. Ignoring");
      }

      /* delete the leaf qdisc */
      snprintf(cmd, sizeof(cmd), "tc qdisc delete dev %s parent %d:%d "
               "handle %d:0 prio flow enable",
               netmgr_kif_get_name( link ),
               flow_info->class_handle->major,
               flow_info->class_handle->minor,
               flow_info->class_handle->minor);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_high("Error deleting leaf qdisc. Ignoring");
      }

      /* delete the class */
      snprintf(cmd, sizeof(cmd), "tc class delete dev %s parent %d:%d "
               "classid %d:%d",
               netmgr_kif_get_name( link ),
               netmgr_tc_cfg.links[link].root_class->major,
               netmgr_tc_cfg.links[link].root_class->minor,
               flow_info->class_handle->major,
               flow_info->class_handle->minor);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_high("Error deleting class. Ignoring");
      }
    }
    else
    {
      netmgr_log_med("root class deleted, skipping heirarchy update");
    }

    netmgr_tc_flow_free(&flow_info);
    ds_dll_free(node);
    result = NETMGR_SUCCESS;
  }

done:
  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_tc_link_cleanup
===========================================================================*/
/*!
@brief
  Performs cleanup of flow objects.  Invoked at process termination.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_tc_link_cleanup
(
  int link
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];

  NETMGR_LOG_FUNC_ENTRY;

  snprintf(cmd, sizeof(cmd), "tc qdisc del dev %s root",
           netmgr_kif_get_name( link ));
  (void) ds_system_call(cmd, std_strlen(cmd));

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_tc_link_init
===========================================================================*/
/*!
@brief
 Initializes link state information.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_tc_link_init( int link )
{
  NETMGR_LOG_FUNC_ENTRY;

  /* Initializing specified interface */
  if( netmgr_tc_cfg.link_array[ link ].enabled )
  {
    memset((void*) &netmgr_tc_cfg.links[ link ], 0x0, sizeof(netmgr_tc_cfg.links[ link ]));

    netmgr_tc_cfg.links[ link ].root_qdisc = NULL;
    netmgr_tc_cfg.links[ link ].next_class_minor = NETMGR_TC_CLASS_MINOR_ID_INIT;
    netmgr_tc_cfg.links[ link ].flow_list = ds_dll_init( netmgr_tc_cfg.links[ link ].flow_list );
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_tc_reset_link
===========================================================================*/
/*!
@brief
  Reinitialize link data structures on reset command.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_tc_reset_link( int link )
{
  NETMGR_LOG_FUNC_ENTRY;

  /* Cleanup dynamic memory */
  netmgr_tc_link_cleanup( link );

  /* Reinitialize link data structures */
  netmgr_tc_link_init( link );

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}

/*===========================================================================
  FUNCTION  netmgr_tc_create_postrouting_mangle_chain
===========================================================================*/
/*!
@brief
 In this new postrouting mangle chain, we make sure only packets
 marked by this module in the output chain, will be accepted and
 rest of the packets will be explicitly marked with "0".

@return
  NETMGR_SUCCESS, NETMGR_FAILURE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
netmgr_tc_create_postrouting_mangle_chain
(
  uint32 ip_family
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int length;
  const unsigned int maxlength = sizeof(cmd)-1;
  char * ipt_tool = NULL;
  int i;

  NETMGR_LOG_FUNC_ENTRY;

  if (ip_family != NETMGR_IPV6_ADDR &&
        ip_family != NETMGR_IPV4_ADDR )
  {
    netmgr_log_err("invalid IP version revceived");
    goto error;
  }

  ipt_tool = (NETMGR_IPV6_ADDR == ip_family)?
                 NETMGR_TC_IPTABLES_TOOL_V6 : NETMGR_TC_IPTABLES_TOOL_V4;

  memset( cmd, 0x0, sizeof(cmd) );
  length = snprintf(cmd,
                    maxlength,
                    "%s -t mangle -N %s",
                    ipt_tool,
                    NETMGR_TC_CHAIN_NAME_POSTROUTING);

  if( (unsigned int)length >= maxlength )
    goto error;

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    netmgr_log_err("failed to create the postrouing chain");
    goto error;
  }

  /* wildcard entry to mark all non QOS packets with
   * default mark value for enabled interfaces
   */
  for( i = 0; i < NETMGR_MAX_LINK; ++i )
  {

    memset( cmd, 0x0, sizeof(cmd) );
    length = snprintf(cmd,
                    maxlength,
                    "%s -t mangle -A %s -o %s -j MARK --set-mark 0x%x",
                    ipt_tool,
                    NETMGR_TC_CHAIN_NAME_POSTROUTING,
                    netmgr_kif_get_name(netmgr_tc_cfg.link_array[i].link_id),
                    NETMGR_TC_DEFAULT_FLOW_MARKING);

    if( (unsigned int)length >= maxlength )
      goto error;

    if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
    {
      goto error;
    }
  }
  memset( cmd, 0x0, sizeof(cmd) );
  /*Link the chain to postrouting chain of mangle table*/
  length = snprintf(cmd,
                    maxlength,
                    "%s -t mangle -A POSTROUTING -j %s",
                    ipt_tool,
                    NETMGR_TC_CHAIN_NAME_POSTROUTING);

  if( (unsigned int)length >= maxlength )
    goto error;

  if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
  {
    goto error;
  }

  return NETMGR_SUCCESS;

error:
  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_delete_postrouting_mangle_chain
===========================================================================*/
/*!
@brief
  Deletes the postrouting chain. This should only be called during process
  exit as part of cleanup procedure.

@return
  NETMGR_SUCCESS, NETMGR_FAILURE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
netmgr_tc_delete_postrouting_mangle_chain
(
  uint32 ip_family
)
{
  char cmd[NETMGR_TC_MAX_COMMAND_LENGTH];
  int length;
  char * ipt_tool = NULL;
  const unsigned int maxlength = sizeof(cmd)-1;

  NETMGR_LOG_FUNC_ENTRY;

  if (ip_family != NETMGR_IPV6_ADDR &&
        ip_family != NETMGR_IPV4_ADDR )
  {
    netmgr_log_err("invalid IP version revceived");
    return;
  }

  ipt_tool = (NETMGR_IPV6_ADDR == ip_family)?
                 NETMGR_TC_IPTABLES_TOOL_V6 : NETMGR_TC_IPTABLES_TOOL_V4;

  memset( cmd, 0x0, sizeof(cmd) );

  /*remove the reference of the chain from the POSTROUTING chain*/
  length = snprintf(cmd,
                    sizeof(cmd),
                    "%s -t mangle -D POSTROUTING -j %s",
                    ipt_tool,
                    NETMGR_TC_CHAIN_NAME_POSTROUTING);

  if( (unsigned int)length >= maxlength)
  {
    netmgr_log_err("snprintf buffer overflow");
    return;
  }

  (void) ds_system_call(cmd, std_strlen(cmd));

  memset( cmd, 0x0, sizeof(cmd) );
  /* Deleting the chain deletes all the rules.
   This chain must not be referenced by any other rules. */
  length = snprintf(cmd,
                    sizeof(cmd),
                    "%s -t mangle -X %s",
                    ipt_tool,
                    NETMGR_TC_CHAIN_NAME_POSTROUTING);

  if( (unsigned int)length >= maxlength)
  {
    netmgr_log_err("snprintf buffer overflow");
    return;
  }

  (void) ds_system_call(cmd, std_strlen(cmd));
}

/*===========================================================================
  FUNCTION  netmgr_tc_reset_link_all
===========================================================================*/
/*!
@brief
  Reinitialize link data structures for all links.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_tc_reset_link_all
(
  void
)
{
  int i;

  NETMGR_LOG_FUNC_ENTRY;

  for( i = 0; i < NETMGR_MAX_LINK; ++i )
  {
    /* skip if this interface is not used */
    if (netmgr_tc_cfg.link_array[i].enabled == FALSE)
    {
      netmgr_log_low( "ignoring link[%d]\n", i );
      continue;
    }

    (void)netmgr_tc_reset_link( i );
  }

  netmgr_tc_delete_postrouting_mangle_chain(NETMGR_IPV4_ADDR);
  netmgr_tc_delete_postrouting_mangle_chain(NETMGR_IPV6_ADDR);

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  netmgr_tc_get_qos_params_by_profile_id
===========================================================================*/
/*!
@brief
  Lookup the datarate and priority QoS parameters based on CDMA profile ID.

@return
  int - NETMGR_SUCCESS on successful operations, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - Initializes the QMI Driver
*/
/*=========================================================================*/
int netmgr_tc_get_qos_params_by_profile_id
(
  uint16      profile_id,
  uint32    * datarate,
  uint8     * priority
)
{
  uint16 i;

  NETMGR_ASSERT( datarate );
  NETMGR_ASSERT( priority );

  /* Loop over table records */
  for( i=0; i<ds_arrsize( netmgr_tc_cdma_qos_spec_tbl ); i++ ) {
    /* Check for profile ID match */
    if( profile_id == netmgr_tc_cdma_qos_spec_tbl[i].profile_id ) {
      /* Return QoS parameters to caller */
      *datarate = netmgr_tc_cdma_qos_spec_tbl[i].datarate;
      *priority = netmgr_tc_cdma_qos_spec_tbl[i].priority;
      netmgr_log_low( "netmgr_tc_get_qos_params_by_profile_id: "
                      "datarate=%d priority=%d\n",
                      *datarate, *priority );
      return NETMGR_SUCCESS;
    }
  }

  netmgr_log_err( "netmgr_tc_get_qos_params_by_profile_id: "
                  "cannot find mapping for profile==%d\n",
                  profile_id );

  return NETMGR_FAILURE;
}

/*===========================================================================
  FUNCTION  netmgr_tc_init
===========================================================================*/
/*!
@brief
 Main initialization routine of the traffic control module.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
netmgr_tc_init( int nlink, netmgr_ctl_port_config_type links[] )
{
  struct tc_vtbl vtable;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT(nlink > 0);

  /* Set number of links in the configuration blob */
  netmgr_tc_cfg.nlink = nlink;
  netmgr_tc_cfg.link_array = links;   /* Needed for reset */

  /* Register with Platform layer */
  vtable.reset             = netmgr_tc_reset_link;
  vtable.flow_activate     = netmgr_tc_flow_activate_hdlr;
  vtable.flow_modify       = netmgr_tc_flow_modify_hdlr;
  vtable.flow_delete       = netmgr_tc_flow_delete_hdlr;
  vtable.flow_control      = netmgr_tc_flow_control_hdlr;
  vtable.flow_suspend      = netmgr_tc_flow_activate_hdlr;

  if( NETMGR_SUCCESS !=
      netmgr_platform_register_vtbl( NETMGR_PLATFORM_VTBL_TC,
                                     (void*)&vtable ) )
  {
    NETMGR_ABORT("cannot register vtable with platform layer");
  }

  /* Initialize link data structures */
  netmgr_tc_reset_link_all();

  netmgr_tc_cfg.postrouting_chain_available =
    netmgr_tc_create_postrouting_mangle_chain(NETMGR_IPV4_ADDR);
  netmgr_tc_cfg.postrouting_chain_available =
    netmgr_tc_create_postrouting_mangle_chain(NETMGR_IPV6_ADDR);

  /* Register process exit cleanup handler */
  atexit( netmgr_tc_reset_link_all );

  netmgr_tc_cfg.is_initialized = TRUE;

  NETMGR_LOG_FUNC_EXIT;
  return;
}

#endif /* NETMGR_QOS_ENABLED */

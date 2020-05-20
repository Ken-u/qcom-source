#ifndef PS_LOGGING_DEFS_H
#define PS_LOGGING_DEFS_H
/*===========================================================================

                        P S _ L O G G I N G _ D E F S . H

GENERAL DESCRIPTION
  This file defines the data sturctures, constants used by PS to
  frame diagnostic responses to the requests from the tool (DM).

Copyright (c) 2004-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
                        EDIT HISTORY FOR MODULE

 $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_logging_defs.h#2 $
 $Author: anupamad $ $DateTime: 2011/05/24 13:14:40 $

when        who    what, where, why
--------    ---    ---------------------------------------------------------- 
03/14/11    kk     Adding new RmNet ifaces to DPL list.
11/11/10    gn     Adding new WLAN ifaces to DPL list 
06/30/10    rt     Added DPL support on NAT iface.
03/15/10    hs     Added DPL support on two new LTE Ifaces
08/31/09   mga     Merged from eHRPD branch
12/14/08    pp     Common Modem Interface: Public/Private API split.
21/11/08    am     Macro name changes for DPL IID flags.
10/17/08    am     Fixed ANSI C warnings for bit-field usage.
09/04/08    ssh    Removed featurization from dpl_iid_ifname_enum_type
09/01/08    ssh    Added UICC to iface list
12/25/06    msr    Fixed broken secondary link logging
08/31/06    mjp    Added IPHC and ROHC Logging
06/06/06    ss     Renamed DVBH_IFACE to DVBH_V4_IFACE, added DVBH_V6_IFACE
04/14/06    ss     Added DVBH to ifname list
05/31/05   aku/jd  Added WLAN iface to ifname list
03/15/05    ks     changed name of RMNET iface in dpl_iid_ifname_enum_type
01/12/05    msr    Added code review comments.
01/10/05    msr    Added support for flow and link logging.
11/08/04    msr    Moved the condition, which checks if DPL_IID_IFNAME_MAX is
                   greater than MAX_SYSTEM_IFACES, to ps_logging_diag.c
10/31/04   msr/ks  Changed the name of this file from ps_loggingi.h to
                   ps_logging_defs.h.
09/15/04   ks/msr  Major cleanup.
08/09/04    vd     Created the file.
===========================================================================*/


/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/

#include "comdef.h"
#include "ps_iface_defs.h"


/*===========================================================================

                            CONSTANTS

===========================================================================*/
/*---------------------------------------------------------------------------
  Maximum length of ps_iface's and phys link's description
---------------------------------------------------------------------------*/
#define DPL_IFNAME_DESC_S_LEN  32
#define DPL_LINK_DESC_S_LEN    DPL_IFNAME_DESC_S_LEN
#define DPL_FLOW_DESC_S_LEN    DPL_IFNAME_DESC_S_LEN

/*---------------------------------------------------------------------------
  Snaplen value special case to indicate that whole packet needs to be
  logged
---------------------------------------------------------------------------*/
#define SNAPLEN_ALL  0

/*---------------------------------------------------------------------------
  Snaplen value special case to indicate the maximum partial logging length,
  that is supporte
---------------------------------------------------------------------------*/
#define SNAPLEN_MAX  65535

/*---------------------------------------------------------------------------
  DPL flag masks and access macros.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------

Interface Identifier (IID) fields:

  + - - - - - - - - + - - - - - - - - + - - - - - - - - + - - - - - - - - +
  | Flags           | Ifname          | Protocol        | Link instance   |
  + - - - - - - - - + - - - - - - - - + - - - - - - - - + - - - - - - - - +
  |                  \
  |                   \
  + - - - - - - - - -- +
  |L | D| F|  Reserved |
  + - - - - - - - - -  +

---------------------------------------------------------------------------*/

#define DPL_IID_RESERVED_MASK (0x1F)

#define IS_DPL_IID_RESERVED_NONZERO(iid)         \
  ((iid).dpl_flags & (DPL_IID_RESERVED_MASK))

/*---------------------------------------------------------------------------
  Flow bit (Which of flow and network logging is enabled?)

typedef enum
{
  DPL_IID_FLOW_BIT_MIN      = 0,
  DPL_IID_FLOW_BIT_NONFLOW  = 0,
  DPL_IID_FLOW_BIT_FLOW     = 1,
  DPL_IID_FLOW_BIT_WIDTH    = 1        Ensures that enum occupies 1 bit
} dpl_iid_flow_bit_enum_type;
---------------------------------------------------------------------------*/
#define DPL_IID_FLOW_MASK (0x20)

#define SET_DPL_IID_F_BIT_FLOW(iid)              \
  ((iid).dpl_flags |= (DPL_IID_FLOW_MASK))

#define SET_DPL_IID_F_BIT_NONFLOW(iid)           \
  ((iid).dpl_flags &= ~(DPL_IID_FLOW_MASK))

#define IS_DPL_IID_F_BIT_FLOW(iid)               \
  ((iid).dpl_flags & (DPL_IID_FLOW_MASK))

#define IS_DPL_IID_F_BIT_NONFLOW(iid)            \
  (!IS_DPL_IID_F_BIT_FLOW(iid))

/*---------------------------------------------------------------------------
  D bit (Direction)

typedef enum
{
  DPL_IID_DIR_MIN   = 0,
  DPL_IID_DIR_RX    = 0,
  DPL_IID_DIR_TX    = 1,
  DPL_IID_DIR_WIDTH = 1                Ensures that enum occupies 1 bit
} dpl_iid_d_bit_enum_type;
---------------------------------------------------------------------------*/
#define DPL_IID_DIRECTION_MASK (0x40)

#define SET_DPL_IID_DIR_TX(iid)                  \
  ((iid).dpl_flags |= (DPL_IID_DIRECTION_MASK))

#define SET_DPL_IID_DIR_RX(iid)                  \
  ((iid).dpl_flags &= ~(DPL_IID_DIRECTION_MASK))

#define IS_DPL_IID_DIR_TX(iid)                   \
  ((iid).dpl_flags & (DPL_IID_DIRECTION_MASK))

#define IS_DPL_IID_DIR_RX(iid)                   \
  (!IS_DPL_IID_DIR_TX(iid))


/*---------------------------------------------------------------------------
  L bit (Is link level logging enabled?)

typedef enum
{
  DPL_IID_L_BIT_MIN      = 0,
  DPL_IID_L_BIT_NETWORK  = 0,
  DPL_IID_L_BIT_LINK     = 1,
  DPL_IID_L_BIT_WIDTH    = 1           Ensures that enum occupies 1 bit
} dpl_iid_l_bit_enum_type;
---------------------------------------------------------------------------*/
#define DPL_IID_LINK_MASK (0x80)

#define SET_DPL_IID_L_BIT_LINK(iid)              \
  ((iid).dpl_flags |= (DPL_IID_LINK_MASK))

#define SET_DPL_IID_L_BIT_NETWORK(iid)           \
  ((iid).dpl_flags &= ~(DPL_IID_LINK_MASK))

#define IS_DPL_IID_L_BIT_LINK(iid)               \
  ((iid).dpl_flags & (DPL_IID_LINK_MASK))

#define IS_DPL_IID_L_BIT_NETWORK(iid)            \
  (!IS_DPL_IID_L_BIT_LINK(iid))
  
/*===========================================================================

                                   VARIABLES

===========================================================================*/
/*---------------------------------------------------------------------------
  Enum used for IID Interface names
---------------------------------------------------------------------------*/
typedef enum
{
  /* Value 0 is reserved and must not be defined or used
  DPL_IID_IFNAME_RESERVED               = 0                                */
  DPL_IID_IFNAME_MIN                    = 1,
  DPL_IID_IFNAME_CDMA_SN_IFACE_V4_PKT   = 1,
  DPL_IID_IFNAME_CDMA_SN_IFACE_V6_PKT   = 2,
  DPL_IID_IFNAME_CDMA_SN_IFACE_ANY_PKT  = 3,
  DPL_IID_IFNAME_CDMA_SN_IFACE_ASYNC    = 4,
  DPL_IID_IFNAME_CDMA_AN_IFACE          = 5,
  DPL_IID_IFNAME_UMTS_PDP_CONTEXT_0     = 6,
  DPL_IID_IFNAME_UMTS_PDP_CONTEXT_1     = 7,
  DPL_IID_IFNAME_UMTS_PDP_CONTEXT_2     = 8,
  DPL_IID_IFNAME_SIO_IFACE_UMTS         = 9,
  DPL_IID_IFNAME_SIO_IFACE_CDMA         = 10,
  DPL_IID_IFNAME_CDMA_BCAST_IFACE       = 11,
  DPL_IID_IFNAME_IPSEC_IFACE            = 12,
  DPL_IID_IFNAME_LO_V4_IFACE            = 13,
  DPL_IID_IFNAME_LO_V6_IFACE            = 14,
  DPL_IID_IFNAME_SIO_IFACE_RMNET        = 15,
  DPL_IID_IFNAME_WLAN_IFACE             = 16,
  DPL_IID_IFNAME_DVBH_V4_IFACE          = 17,
  DPL_IID_IFNAME_DVBH_V6_IFACE          = 18,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_2      = 19,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_3      = 20,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_4      = 21,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_5      = 22,
  DPL_IID_IFNAME_UICC_IFACE             = 23,
  DPL_IID_IFNAME_IWLAN_3GPP2_IFACE      = 24,
  DPL_IID_IFNAME_IWLAN_3GPP_IFACE_PDP_0 = 25,
  DPL_IID_IFNAME_IWLAN_3GPP_IFACE_PDP_1 = 26,
  DPL_IID_IFNAME_IWLAN_3GPP_IFACE_PDP_2 = 27,
  DPL_IID_IFNAME_STA_IFACE              = 28,
  DPL_IID_IFNAME_UW_FMC_IFACE           = 29,
  DPL_IID_IFNAME_PROXY_IFACE_0          = 30,
  DPL_IID_IFNAME_PROXY_IFACE_1          = 31,
  DPL_IID_IFNAME_PROXY_IFACE_2          = 32,
  DPL_IID_IFNAME_PROXY_IFACE_3          = 33,
  DPL_IID_IFNAME_PROXY_IFACE_4          = 34,
  DPL_IID_IFNAME_3GPP_CONTEXT_0_IPV4    = 35,
  DPL_IID_IFNAME_3GPP_CONTEXT_0_IPV6    = 36, 
  DPL_IID_IFNAME_3GPP_CONTEXT_1_ANY     = 37,
  DPL_IID_IFNAME_3GPP_CONTEXT_2_ANY     = 38,
  DPL_IID_IFNAME_3GPP_CONTEXT_3_ANY     = 39,
  DPL_IID_IFNAME_3GPP_CONTEXT_4_ANY     = 40,
  DPL_IID_IFNAME_3GPP_CONTEXT_5_ANY     = 41,
  DPL_IID_IFNAME_3GPP_CONTEXT_6_ANY     = 42,
  DPL_IID_IFNAME_CDMA_SN_IFACE_3        = 43,
  DPL_IID_IFNAME_CDMA_SN_IFACE_4        = 44,
  DPL_IID_IFNAME_CDMA_SN_IFACE_5        = 45,
  DPL_IID_IFNAME_CDMA_SN_IFACE_6        = 46,
  DPL_IID_IFNAME_CDMA_SN_IFACE_7        = 47,
  DPL_IID_IFNAME_CDMA_SN_IFACE_8        = 48,
  DPL_IID_IFNAME_NAT_IFACE_1            = 49,
  DPL_IID_IFNAME_WLAN_IFACE_ADHOC       = 50,
  DPL_IID_IFNAME_WLAN_IFACE_SOFTAP      = 51,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_6      = 52,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_7      = 53,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_8      = 54,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_9      = 55,
  DPL_IID_IFNAME_SIO_IFACE_RMNET_10     = 56,
  DPL_IID_IFNAME_MAX,
  DPL_IID_IFNAME_WIDTH                  = 0xFF /* Ensures that enum
                                                  occupies 8 bits          */
} dpl_iid_ifname_enum_type;

/*---------------------------------------------------------------------------
  When link bit is DPL_IID_L_BIT_NETWORK, the protocol field uses this enum
---------------------------------------------------------------------------*/
typedef enum
{
  /* Value 0 is reserved and must not be used
  DPL_IID_NETPROT_RESERVED      = 0                                         */
  DPL_IID_NETPROT_MIN           = 1,
  DPL_IID_NETPROT_IP            = 1,
  DPL_IID_NETPROT_HDLC_UNFRAMED = 2,
  DPL_IID_NETPROT_MAX,
  DPL_IID_NETPROT_WIDTH         = 0xFF /* Ensures that enum occupies 8 bits */
} dpl_iid_netprot_enum_type;

/*---------------------------------------------------------------------------
  When link bit is DPL_IID_L_BIT_LINK, the protocol field uses this enum
---------------------------------------------------------------------------*/
typedef enum
{
  DPL_IID_LINKPROT_MIN          = 0,
  DPL_IID_LINKPROT_WILDCARD     = 0,
  DPL_IID_LINKPROT_ETHERNET     = 1,
  DPL_IID_LINKPROT_PPP_IN_HDLC  = 2,
  DPL_IID_LINKPROT_ROHC_COMP_IP = 3,
  DPL_IID_LINKPROT_IPHC_COMP_IP = 4,
  DPL_IID_LINKPROT_MAX,
  DPL_IID_LINKPROT_WIDTH        = 0xFF /* Ensures that enum occupies 8 bits */
} dpl_iid_linkprot_enum_type;

/*---------------------------------------------------------------------------
  IID data structure.
---------------------------------------------------------------------------*/
typedef struct
{
  unsigned int                   dpl_flags:8;
  unsigned int                   ifname:8;
  unsigned int                   protocol:8;
  unsigned int                   link_instance:8;
} dpl_iid_type;

/*---------------------------------------------------------------------------
  ROHC and IPHC Headers for DPL logging
---------------------------------------------------------------------------*/
typedef struct
{
  byte   ppp_flag;
  byte   ppp_prot[2];
} dpl_comp_header_type;

/*---------------------------------------------------------------------------
  Link logging control block for Tx and Rx directions

    - partial_bytes_logged : records the number of bytes in a packet logged
                             so far
    - snaplen              : partial logging length for link protocols
    - mask                 : A set bit implies that logging is enabled for
                             corresponding protocol
    - seq_num              : Sequence number sent in last log packet
    - dpl_id               : DPL IID of this Physlink. Protocol field must
                             always be 0
    - is_logged            : Indicates if logging is enables or not

  There are two iid's in the link control block so resets can be done easily
  and are consistent over time.

  This is not featurized so ROHC and IPHC libraries will build if logging is
  turned off
---------------------------------------------------------------------------*/
typedef struct
{
  uint32   partial_bytes_logged;
  uint32   snaplen;
  uint16   seq_num[DPL_IID_LINKPROT_MAX];
  uint16   seg_num[DPL_IID_LINKPROT_MAX];
  boolean  is_logged;
} dpl_link_dir_cb_type;

typedef struct
{
  dpl_link_dir_cb_type  tx_cb;
  dpl_link_dir_cb_type  recv_cb;
  dpl_iid_type          tx_dpl_id;
  dpl_iid_type          recv_dpl_id;
  char                  desc[DPL_LINK_DESC_S_LEN];
} dpl_link_cb_type;

/*---------------------------------------------------------------------------
  Network logging control block for Tx and Rx directions
    - snaplen  : partial logging length for network protocols
    - mask     : A set bit implies that logging is enabled for
                 corresponding protocol (see netprot enum)
    - seq_num  : Sequence number sent in last log packet
---------------------------------------------------------------------------*/
typedef struct
{
  uint32        mask;
  uint32        snaplen[DPL_IID_NETPROT_MAX];
  uint16        seq_num[DPL_IID_NETPROT_MAX];
} dpl_net_dir_cb_type;

#endif /* PS_LOGGING_DEFS_H */

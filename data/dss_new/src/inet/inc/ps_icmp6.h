/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                                 I C M P 6. h

GENERAL DESCRIPTION
 Internet Control Message Protocol Version 6(ICMPv6)


EXTERNALIZED FUNCTIONS
  icmp6_input()
   This function Process an incoming ICMPv6 packet.
   The only ICMPv6 messages that are currently processed are 1) Echo Request.
   The Echo Response will be echoed back to the ICMPv6 peer in the base
   station.



INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 1995-2009 by Qualcomm Technologies, Inc.  All Rights Reserved.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE
$Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_icmp6.h#1 $
$Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/02/05    vp     Renamed icmp6i_rate_limit_on_type() to
                   icmp6_rate_limit_on_type().
07/16/04    vp     Declaration of icmpv6_token_bucket related constants.
                   Declaration of function icmp6_init_token_bucket().
06/11/04    vp     Removal of inclusion of psglobal.h and netuser.h
04/05/04    sv     Featurized the code.
03/16/04    mct    Renamed acl_ipfilter_info_type to ip_pkt_info_type.
03/02/04    rc     Added function prototype for icmp6_output().
11/11/03    rc     Initial Version

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "customer.h"       /* Customer Specific Features */
#if defined (FEATURE_DATA_PS) && defined (FEATURE_DATA_PS_IPV6)
#include "dsm.h"
#include "ps_iface_defs.h"
#include "ps_ip.h"
#include "ps_iputil.h"
#include "ps_icmp6_msg.h"
#include "token_bucket.h"
#include "msg.h"
#include "err.h"
#include "dssocket.h"
#include "amssassert.h"
#include <memory.h>
#include "ps_icmp.h"

/*===========================================================================

                          PUBLIC DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Token bucket parameters to rate limit ICMPv6 messages.
---------------------------------------------------------------------------*/
#define ICMP6_TOKENS_PER_SECOND 10/* # of milliseconds to generate 1 token */
#define ICMP6_MAX_TOKENS 10      /* Max # of tokens in icmpv6_token_bucket */

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                      PUBLIC FUNCTION DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================

FUNCTION ICMP6_RATE_LIMIT_ON_TYPE()

DESCRIPTION
  This function rate limits outgoing ICMPv6 messages based on type of message.

DEPENDENCIES
  icmpv6_token_bucket is initialized.

RETURN VALUE
  0 - Rate limit the packet (Drop the packet - no token)
  1 - Message can be sent.

SIDE EFFECTS
  None
===========================================================================*/
boolean icmp6_rate_limit_on_type
(
  ps_icmp6_msg_enum_type type                        /* ICMP6 message type */
);

/*===========================================================================

FUNCTION ICMP6_INPUT()

DESCRIPTION
   This function Process an incoming ICMPv6 packet.
   The only ICMPv6 messages that are currently processed are 1) Echo Request.
   The Echo Response will be echoed back to the ICMPv6 peer in the base
   station.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void icmp6_input
(
  ip_pkt_info_type*        ip_pkt_info_ptr,   /* IP Packet Info            */
  struct dsm_item_s        **item_ptr,        /* Pointer to IPv6 Packet    */
  uint16                   offset             /* Offset to the ICMP packet */
);

/*===========================================================================

FUNCTION ICMP6_OUTPUT()

DESCRIPTION
  This function does the following:

  1. Calls function to insert the ICMP header in the ICMP packet,
     create the pseudo-header and compute the ICMP6 checksum.
  3. Sends ICMP packet down to IP.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
extern void icmp6_output
(
  struct ps_in6_addr *dest_ip_ptr,          /* Destination of the delivery */
  ps_icmp6_msg_type  *icmp6_msg,                           /* ICMP Message */
  struct dsm_item_s  *item_ptr,                       /* Item to be output */
  struct icmp_cb     *icmp_cb_ptr,                  /* Ptr to ICMP ctl blk */
  uint16             length,            /* Optional Length of Data Portion */
  ps_iface_type     *tx_if_cache                          /* Routing Cache */
);

/*===========================================================================

FUNCTION ICMP6_INIT_TOKEN_BUCKET()

DESCRIPTION
  This function initializes the icmpv6_token_bucket.

DEPENDENCIES
  None

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
extern void icmp6_init_token_bucket(void);
#endif /* FEATURE_DATA_PS && FEATURE_DATA_PS_IPV6 */


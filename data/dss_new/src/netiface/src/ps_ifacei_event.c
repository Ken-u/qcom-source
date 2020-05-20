/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             P S _ I F A C E I _ E V E N T . C


GENERAL DESCRIPTION
  This is the implementation of the PS interface event functions. This
  file implement functions used for both ps_iface and phys_link events.

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2002-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_ifacei_event.c#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/08/10    rp     RAM optimization changes.
11/18/10    sy     Removed ps_ifacei_event_cback()
08/05/09    jee    Added a notification for DIAG upon IFACE UP event
06/30/10    ea     Clear the cache when the iface is down.
06/19/10    vs     Increased PS_IFACE_EVT_BUF_NUM due to QMI Dual IP support
09/21/09   mga     Merged from eHRPD branch
09/09/09    ss     Critical section released in error cases.
08/29/08    am     Increased PS_IFACE_EVT_BUF_NUM and high WM for low-tiers.
08/19/08    dm     Added support to post EXTENDED_IP_CONFIG_EV when DHCP
                   handle is not valid at the time of iface bring up
04/01/08    ssh    Adjusted iface event bufs to allow for MIPv6
04/03/08    am     BCMCS 2.0 Support.
06/26/07    rs     Added code to stop the Querier V1 present timer if the
                   interface is getting disabled.
05/07/07    mct    IPv6 Neighbor Discovery
04/17/07    es     Added support for EXTENDED_IP_CONFIG_EV.
04/09/07    vp     Additions for mip6_bootstrap_info
03/27/07    msr    Added support for RF_CONDITIONS_CHANGED_EV
02/08/07    scb    Fixed Klocwork High errors
01/23/07    msr    EMPA enhancements
11/29/06    msr    Fixed bug in ps_iface_change_pri_phys_link()
11/06/06    hm     Using a single ps_iface_free_domain_name_list function
                   instead of two different versions for SIP and DNS.
11/04/06    hm     Freeing domain name search on iface down indication.
11/02/06    mct    Added support for RFC3041: IPv6 Privacy Extensions.
11/01/06    rt     Free SIP server and domain name fn's used during IFACE
                   DOWN indication to free the lists.
11/01/06    rt     Merged EIDLE_SCI feature changes.
09/12/06    msr    Removed redundant state field in event_info structure
09/05/06    ks     Include network_params_info_type struct in ps_iface
                   instead of only auth_info to hold network config params.
08/14/06    sv     Merged slotted/Mode attribute ioctl.
08/15/06    rt     Resetting the SIP server list during IFACE down indication
04/25/06    msr    L4/Tasklock code review changes
02/22/06    msr    Using single critical section
02/13/06    msr    Fixed compilation errors
02/06/06    msr    Updated for L4 tasklock/crit sections.
10/18/05    msr    Removed support for FLOW_ACTIVATE_MODIFIED_EV and
                   PHYS_LINK_UP_MODIFIED_EV and changed
                   ps_iface_ipfltr_updated_ind() to conform to ipfltr_info
10/12/05    mct    Added support for new iface flow added/deleted events.
09/13/05    sv     Added support for new HDR rev0 rate inertia and HDR HPT mode
                   ioctls.
09/09/05    jd     On iface down indication, clear mode independent
                   authentication parameters
09/01/05    rt     Added TECH_CHANGED_EV in ps_iface_generic_ind
08/29/05    mct    Fixed missing taskfrees.
08/16/05    ks     Clear ps_iface stats from QMI on iface_down
08/16/05    msr    Fixed PS_BRANCH_TASKFREE()
08/15/05    mct    Fixed naming causing some issues w/C++ compilers.
08/15/05    mct    Added support for QOS_AWARE and QOS_UNAWARE events.
08/03/05    msr    Fixed a bug in ps_iface_change_pri_phys_link().
07/25/05    rt     Added function ps_iface_down_ind_ex()
06/20/05    mct    Added support for new multicast events.
05/13/05    ks     fixed lint errors
05/13/05    ks     fixed bug introduced in lint changes.
05/12/05    mct    Lint changes.
05/03/05    msr    Not enforcing any state checks
04/17/05    msr    Changed the behaviour of ps_iface_change_pri_phys_link().
                   Enforcing that iface is GOING_DOWN when phys link or flow
                   is GOING_NULL.
04/16/05    ks     Added validation of events before invoking them to avoid
                   duplicate events.
04/16/05    ks     Added PHYS_LINK_NULL state for PHYS_LINK_GONE_EV.
01/10/05    sv     Merged IPSEC changes.
01/08/05    vp     Flushing of net info (both v4/6) during ps_iface_down_ind()
11/19/04    msr    Using internal ps_iface macros instead of external ones.
11/19/04    msr    Added F3 messages to iface indications.
11/16/04    ks     Added check to ensure only valid phys_link events can be
                   registered with ps_iface_event_cback_reg(). Added
                   registering of phys link coming up, going down and gone
                   events.
11/08/04    mct    Changed TASKFREE to PS_BRANCH_TASKFREE to fix compilation
                   errors.
11/03/04    msr    Fixed missing brace and TASKFREE().
10/25/04    msr    Changed ps_iface_pri_phys_link_change_ind_ex() to
                   ps_iface_change_pri_phys_link() and changed the
                   implementation. Freeing buffers if event registration
                   fails in ps_iface_event_init().
10/29/04    mct    Fixed QOS merge issue where iface was being flow controlld
                   when multiple phys links were present.
10/12/04    mct    Call cbacks associated with the MT event. Added new
                   generic indication, ps_iface_generic_ind for new events.
10/05/04    ks     Set the bridge iface ptr to NULL in ps_iface_down_ind().
08/12/04    sv     Added support to distinguish IFACE_FLOW_ENABLED event
                   from PHYS_LINK_FLOW_ENABLED event.
08/09/04    mct    Fixed ps_iface_down_ind for logical ifaces to not call
                   ref_cnt_dec if phys_link has already been disassociated.
08/06/04    sv     Invoke iface flow enabled event when primary phys link
                   is flow enabled.
08/02/04    mct    Changed macros to internal ps_iface macros and included
                   ps_ifacei_utils.h.
07/30/04    ifk    Added ps_iface_valid_ra_ind()
07/30/04    vp     Changes due to consolidation of MTU and net_info structure
                   in ps_iface.
07/16/04    ak     On down_ind(), clear ps iface ref cnt.
06/11/04    vp     Changes for representation of IP addresses as struct
                   ps_in_addr or struct ps_in6_addr.
03/24/04    ak     Handle DOWN and DISABLED case in dorm_action().
03/22/04    ak     Changed dorm_action_func_ptr to ...f_ptr.
03/10/04    ak     Inc ref_cnt on routeable indiciation.
02/18/04    usb    Client id support for ps iface IP filtering.
02/04/04    ak     Updated for phys link and iface separation.  Various
                   fucntions have now moved to ps_phys_linki_event.c.  Also
                   added in events for transient states.
11/03/03    aku    Added Null check in ps_iface_alloc_event_cback_buf() after
                   call to ps_mem_get_buf().
10/21/03    usb    Added required included files.
10/14/03    mct    Added callbacks for the coming_up, going_down, and
                   phys_link equivalent events.
02/03/04    aku    Added support for logical ifaces
01/30/04    usb    Reset in_use, shreable and looked_up flags in iface down.
12/26/03    usb    Handling for new ps_iface state CONFIGURING.
11/03/03    aku    Added Null check in ps_iface_alloc_event_cback_buf() after
                   call to ps_mem_get_buf().
10/21/03    usb    Added required included files.
10/14/03    mct    Added callbacks for the coming_up, going_down, and
                   phys_link equivalent events.
08/25/03    aku    ps_mem_free() takes pointer-to-pointer as arg.
08/15/03    aku    created file.
===========================================================================*/
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                INCLUDE FILES

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "msg.h"
#include "queue.h"
#include "ps_ifacei_event.h"
#include "ps_ifacei_utils.h"
#include "ps_iface.h"
#include "ps_ifacei.h"
#include "ds_flow_control.h"
#include "ps_phys_link.h"
#include "ps_phys_linki_event.h"
#include "ps_mem.h"
#include "ps_utils.h"
#include "ps_flowi.h"
#include "ps_flowi_event.h"
#include "ps_iface_dns_cache.h"

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#include "ps_ip6_addr.h"
#endif /* FEATURE_DATA_PS_IPV6 */

#ifdef FEATURE_DATA_PS_DHCP
#include "dhcp_client.h"
#endif /* FEATURE_DATA_PS_DHCP */

#ifdef FEATURE_DATA_PS_DHCPV6
#include "dhcp6_client.h"
#endif /* FEATURE_DATA_PS_DHCPV6 */

#include "ds_Utils_DebugMsg.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           LOCAL DATA DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*---------------------------------------------------------------------------
  Tuning the number of ps iface event buffers needed by this module
---------------------------------------------------------------------------*/
#define PS_IFACE_EVT_BUF_SIZE  ((sizeof(ps_iface_event_buf_type) + 3) & ~3)

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET

#ifdef FEATURE_DATA_FUSION_MDM
/* We need 25 iface evt buffers for each additional QMI instance.
 * For Fusion, 8 additional QMI instances have been added to for SDIO
 * Hence increasing iface event buffers by 220  */
#define PS_IFACE_EVT_BUF_NUM        1200
#define PS_IFACE_EVT_BUF_HIGH_WM    800

#else

#ifdef FEATURE_DATA_RM_NET_USES_SM_LAPTOP_INST
#ifdef FEATURE_DATA_PS_IPV6
#ifdef FEATURE_DATAMODEM_DUAL_PROC_MODEM_EHRPD
#define PS_IFACE_EVT_BUF_NUM        720
#define PS_IFACE_EVT_BUF_HIGH_WM    680
#else
#define PS_IFACE_EVT_BUF_NUM        620
#define PS_IFACE_EVT_BUF_HIGH_WM    580
#endif /* FEATURE_DATAMODEM_DUAL_PROC_MODEM_EHRPD */
#else
#ifdef FEATURE_DATAMODEM_DUAL_PROC_MODEM_EHRPD
#define PS_IFACE_EVT_BUF_NUM        720
#define PS_IFACE_EVT_BUF_HIGH_WM    580
#else
#define PS_IFACE_EVT_BUF_NUM        520
#define PS_IFACE_EVT_BUF_HIGH_WM    480
#endif /* FEATURE_DATAMODEM_DUAL_PROC_MODEM_EHRPD */
#endif /* FEATURE_DATA_PS_IPV6 */
#else
#define PS_IFACE_EVT_BUF_NUM        810
#define PS_IFACE_EVT_BUF_HIGH_WM    600
#endif /* FEATURE_DATA_RM_NET_USES_SM_LAPTOP_INST */

#endif /* FEATURE_DATA_FUSION_MDM */

#define PS_IFACE_EVT_BUF_LOW_WM      35

#else

#define PS_IFACE_EVT_BUF_NUM         180
#define PS_IFACE_EVT_BUF_HIGH_WM     144
#define PS_IFACE_EVT_BUF_LOW_WM        5
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

#ifdef FEATURE_DSS_LINUX
#undef PS_IFACE_EVT_BUF_NUM
#define PS_IFACE_EVT_BUF_NUM 2000
#endif
/*---------------------------------------------------------------------------
  Allocate memory to hold ps_iface_event_buf along with ps_mem header
---------------------------------------------------------------------------*/
static int ps_iface_evt_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                (
                                  PS_IFACE_EVT_BUF_NUM,
                                  PS_IFACE_EVT_BUF_SIZE
                                )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*---------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging. The first one points
  to ps_mem header and the latter points to ps_iface_event_buf
---------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type      * ps_iface_evt_buf_hdr[PS_IFACE_EVT_BUF_NUM];
static ps_iface_event_buf_type  * ps_iface_evt_buf_ptr[PS_IFACE_EVT_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */


/*---------------------------------------------------------------------------
  GLOBAL_IFACE_EVENT_Q_ARRAY - this is used to store callbacks that are
    intended to be called when events happen onANY interface.
  GLOBAL_IFACE_EVENT_Q_INITIALIZED - has the global queue been initialized?

  NOTE: these are separate (not in a struct) to insure that the
        initialization variables are set correctly.
---------------------------------------------------------------------------*/
static q_type  global_iface_event_q_array[IFACE_EVENT_MAX];

/*---------------------------------------------------------------------------
  Event buffers used by ps_iface to register for global phys link events
---------------------------------------------------------------------------*/
static void *ps_iface_phys_link_up_buf;
static void *ps_iface_phys_link_down_buf;
static void *ps_iface_phys_link_gone_buf;
static void *ps_iface_phys_link_coming_up_buf;
static void *ps_iface_phys_link_going_down_buf;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                       INTERNAL FORWARD DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
static void ps_iface_update_stats_on_down_state
(
  ps_iface_type  *this_iface_ptr
);

static void ps_iface_update_stats_on_up_state
(
  ps_iface_type  *this_iface_ptr
);


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                             INTERNAL FUNCTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION PS_IFACEI_PHYS_LINK_EVENT_CBACK()

DESCRIPTION
  This event callback will be registered by ps_iface as a global event
  callback with phys link. This function will invoke all the global phys link
  event callbacks registed via ps_iface and also the iface specific phys link
  event callbacks.

  Uses ps_ifacei_inoke_event_cback().  Each call to that function results in
  two sets of callbacks:

      callbacks registered with that particular iface
      callbacks registered on the global iface event queue.

  Note that if two iface's point to the same phys_link.  In that case, the
  callbacks registered with the global iface event queue will be called
  twice.  However, each call will be called with a different IFACE ptr.  Up
  to client to distinguish correctly.

PARAMETERS
  this_phys_link_ptr: ptr to the phys link on which we are operating
  this_iface_ptr: ptr to the interface on which we are operating
  event: the event for which callbacks need to be called
  event_info: event information that needs to be passed into callback.
  user_data_ptr: Pointer to user data info

RETURN VALUE
  None

DEPENDENCIES
  Must be called in a TASKLOCK

SIDE EFFECTS
  None
===========================================================================*/
static void ps_ifacei_phys_link_event_cback
(
  ps_phys_link_type           * this_phys_link_ptr,
  ps_iface_event_enum_type      event,
  ps_iface_event_info_u_type    event_info,
  void                        * user_data_ptr
)
{
  ps_iface_type  *iface_ptr = NULL;
  int             iface_index;
  boolean         invoke_cbacks = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Determine if the phys link on which this event occurred is the primary
    phys link of one the created ifaces.
  -------------------------------------------------------------------------*/
  for (iface_index = 0; iface_index < MAX_SYSTEM_IFACES; iface_index++)
  {
    iface_ptr = global_iface_ptr_array[iface_index];

    /*-----------------------------------------------------------------------
      Since ifaces are added sequentially and they are never deleted, as
      soon as we get a NULL ptr there is no more iface pointers in the list.
    -----------------------------------------------------------------------*/
    if(iface_ptr == NULL)
    {
      break;
    }

    /*-----------------------------------------------------------------------
      Call callbacks only if the event occured on primary phys link of the
      iface
    -----------------------------------------------------------------------*/
    if(PS_IFACEI_GET_PHYS_LINK(iface_ptr) == this_phys_link_ptr)
    {
      LOG_MSG_INFO2("Phys link evt %d on iface=0x%x:%d",
              event, iface_ptr->name, iface_ptr->instance);

      switch(event)
      {
        case PHYS_LINK_UP_EV:
          invoke_cbacks = TRUE;
          break;

        case PHYS_LINK_DOWN_EV:
          invoke_cbacks = TRUE;
          break;

        case PHYS_LINK_GONE_EV:
          invoke_cbacks = TRUE;
          break;

        case PHYS_LINK_COMING_UP_EV:
          invoke_cbacks = TRUE;
          break;

        case PHYS_LINK_GOING_DOWN_EV:
          invoke_cbacks = TRUE;
          break;

        default:
          break;
      }

      if (invoke_cbacks == TRUE)
      {
        /*-------------------------------------------------------------------
          Not always invoking callbacks.  Flow-control events will be invoked
          automatically by enable/disable_flow calls above.  Don't want to
          duplicate here.
        -------------------------------------------------------------------*/
        ps_ifacei_invoke_event_cbacks(iface_ptr,
                                      NULL,
                                      event,
                                      event_info);

      } /* invoke cbacks? */
    } /* iface has this phys link */
  } /* for all ifaces... */

} /* ps_ifacei_phys_link_event_cback() */

/*===========================================================================
FUNCTION PS_IFACEI_VALIDATE_EVENT_CBACKS()

DESCRIPTION
  This function will validate all of the event callbacks for a given
  interface or phys link. The validation is successful if the ps_iface or
  the phys_link is not already in the same state as the occuring event.

PARAMETERS
  this_iface_ptr: ptr to the interface on which we are operating
  this_phys_link_ptr: ptr to the phys_link on which we are operating
  event: the event for which callbacks need to be called
  event_info: event information that needs to be passed into callback.

RETURN VALUE
  TRUE: callback must be invoked
  FALSE: don't invoke callback

DEPENDENCIES
  Must be called in a iface critical section

SIDE EFFECTS
  None
===========================================================================*/
static boolean ps_ifacei_validate_event_cbacks
(
  const ps_iface_type             *this_iface_ptr,
  const ps_phys_link_type         *this_phys_link_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info
)
{
  boolean invoke_cbacks = TRUE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Note - Not validating iface and phys_link ptrs as this function is
    called only from ps_ifacei_invoke_event_cbacks() which validates them
    just before calling this
  -------------------------------------------------------------------------*/

  if(NULL != this_iface_ptr)
  {
    switch (event)
    {
      case IFACE_UP_EV :
        if(event_info.state == IFACE_UP)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_DOWN_EV :
        if(event_info.state == IFACE_DOWN)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_ROUTEABLE_EV :
        if(event_info.state == IFACE_ROUTEABLE)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_CONFIGURING_EV :
        if(event_info.state == IFACE_CONFIGURING)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_COMING_UP_EV :
        if(event_info.state == IFACE_COMING_UP)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_GOING_DOWN_EV :
        if(event_info.state == IFACE_GOING_DOWN)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_PHYS_LINK_COMING_UP_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_COMING_UP)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_PHYS_LINK_GOING_DOWN_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_GOING_DOWN)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_PHYS_LINK_DOWN_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_DOWN)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_PHYS_LINK_GONE_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_NULL)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case IFACE_PHYS_LINK_UP_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_UP)
        {
          invoke_cbacks = FALSE;
        }
        break;

      default:
        /* do nothing */
        break;
    } /* switch */

    return invoke_cbacks;
  }

  if(NULL != this_phys_link_ptr)
  {
    switch (event)
    {
      /*---------------------------------------------------------------------
        Don't check if duplicate PHYS_LINK_UP_EV is generated as UMTS calls
        ps_phys_link_up_ind() to indicate that the network modified QoS
        autonomously
      ---------------------------------------------------------------------*/
      case PHYS_LINK_UP_EV :
        break;

      case PHYS_LINK_COMING_UP_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_COMING_UP)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case PHYS_LINK_GOING_DOWN_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_GOING_DOWN)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case PHYS_LINK_DOWN_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_DOWN)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case PHYS_LINK_GONE_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_NULL)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case PHYS_LINK_GOING_NULL_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_GOING_NULL)
        {
          invoke_cbacks = FALSE;
        }
        break;

      case PHYS_LINK_RESUMING_EV :
        if(event_info.phys_link_event_info.state == PHYS_LINK_RESUMING)
        {
          invoke_cbacks = FALSE;
        }
        break;

      default:
        /* do nothing */
        break;

    } /* switch */

    return invoke_cbacks;
  }

  return invoke_cbacks;

} /* ps_ifacei_validate_event_cbacks() */



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                             EXTERNAL FUNCTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION PS_IFACE_EVENT_INIT()

DESCRIPTION
  This function initializes the global queues for both ps_iface and
  ps_phys_link.  It also registers the ps_iface phys_link function on the
  global phys_link queue.

  Note: Currently, only the events in ps_iface need initialization. Hence,
  the implementation of this function resides in ps_ifacei_event.c.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_event_init
(
  void
)
{
  int loop;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Initialize pool
  -------------------------------------------------------------------------*/
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_IFACE_EVT_CB_TYPE,
                           ps_iface_evt_buf_mem,
                           PS_IFACE_EVT_BUF_SIZE,
                           PS_IFACE_EVT_BUF_NUM,
                           PS_IFACE_EVT_BUF_HIGH_WM,
                           PS_IFACE_EVT_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_iface_evt_buf_hdr,
                           (int *) ps_iface_evt_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                            ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

  /*-------------------------------------------------------------------------
    Initialize global event queues
  -------------------------------------------------------------------------*/
  for(loop = 0; loop < IFACE_EVENT_MAX; loop++)
  {
    (void) q_init(global_iface_event_q_array + loop);
  }

  /*-------------------------------------------------------------------------
    Allocate buffers and register the global ps_iface function for phys link
    events.
  -------------------------------------------------------------------------*/
  ps_iface_phys_link_up_buf =
    ps_phys_link_alloc_event_cback_buf(ps_ifacei_phys_link_event_cback, NULL);

  ASSERT(ps_iface_phys_link_up_buf != NULL);
  if(0 != ps_phys_link_event_cback_reg(NULL,
                                       PHYS_LINK_UP_EV,
                                       ps_iface_phys_link_up_buf))
  {
    LOG_MSG_ERROR("Could not reg global cback for evt %d",
              PHYS_LINK_UP_EV,0,0);
    ps_phys_link_free_event_cback_buf(ps_iface_phys_link_up_buf);
    ASSERT(0);
  }

  ps_iface_phys_link_down_buf =
    ps_phys_link_alloc_event_cback_buf(ps_ifacei_phys_link_event_cback, NULL);

  ASSERT(ps_iface_phys_link_down_buf != NULL);
  if(0 != ps_phys_link_event_cback_reg(NULL,
                                       PHYS_LINK_DOWN_EV,
                                       ps_iface_phys_link_down_buf))
  {
    LOG_MSG_ERROR("Could not reg global cback for evt %d",
              PHYS_LINK_DOWN_EV,0,0);
    ps_phys_link_free_event_cback_buf(ps_iface_phys_link_down_buf);
    ASSERT(0);
  }

  ps_iface_phys_link_gone_buf =
    ps_phys_link_alloc_event_cback_buf(ps_ifacei_phys_link_event_cback, NULL);

  ASSERT(ps_iface_phys_link_gone_buf != NULL);
  if(0 != ps_phys_link_event_cback_reg(NULL,
                                       PHYS_LINK_GONE_EV,
                                       ps_iface_phys_link_gone_buf))
  {
    LOG_MSG_ERROR("Could not reg global cback for evt %d",
              PHYS_LINK_GONE_EV,0,0);
    ps_phys_link_free_event_cback_buf(ps_iface_phys_link_gone_buf);
    ASSERT(0);
  }

  ps_iface_phys_link_coming_up_buf =
    ps_phys_link_alloc_event_cback_buf(ps_ifacei_phys_link_event_cback, NULL);

  ASSERT(ps_iface_phys_link_coming_up_buf != NULL);
  if(0 != ps_phys_link_event_cback_reg(NULL,
                                       PHYS_LINK_COMING_UP_EV,
                                       ps_iface_phys_link_coming_up_buf))
  {
    LOG_MSG_ERROR("Could not reg global cback for evt %d",
              PHYS_LINK_COMING_UP_EV,0,0);
    ps_phys_link_free_event_cback_buf(ps_iface_phys_link_coming_up_buf);
    ASSERT(0);
  }

  ps_iface_phys_link_going_down_buf =
    ps_phys_link_alloc_event_cback_buf(ps_ifacei_phys_link_event_cback, NULL);

  ASSERT(ps_iface_phys_link_going_down_buf != NULL);
  if(0 != ps_phys_link_event_cback_reg(NULL,
                                       PHYS_LINK_GOING_DOWN_EV,
                                       ps_iface_phys_link_going_down_buf))
  {
    LOG_MSG_ERROR("Could not reg global cback for evt %d",
              PHYS_LINK_GOING_DOWN_EV,0,0);
    ps_phys_link_free_event_cback_buf(ps_iface_phys_link_going_down_buf);
    ASSERT(0);
  }

} /* ps_iface_event_init() */

/*===========================================================================
FUNCTION PS_IFACE_ALLOC_EVENT_CBACK_BUF()

DESCRIPTION
  This function is used to allocate the memory used to register for ps_iface
  event callbacks.

PARAMETERS
  event_cback_f_ptr: the callback to be registered
  user_data_ptr: data to be called with the callback.

RETURN VALUE
  NULL: if allocation failed
  ptr to memory being allocated.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void *ps_iface_alloc_event_cback_buf
(
  ps_iface_event_cback_type event_cback_f_ptr,
  void                     *user_data_ptr
)
{
  ps_iface_event_buf_type *buf_ptr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(event_cback_f_ptr == NULL)
  {
    return NULL;
  }

  /*-------------------------------------------------------------------------
    Allocate a new event callback buffer.
  -------------------------------------------------------------------------*/
  buf_ptr =
    (ps_iface_event_buf_type *) ps_mem_get_buf(PS_MEM_PS_IFACE_EVT_CB_TYPE);

  if(buf_ptr == NULL)
  {
    LOG_MSG_FATAL_ERROR("No more items in ps_iface event pool",0,0,0);
    return buf_ptr;
  }

  /*-------------------------------------------------------------------------
    Initialize the event buffer
  -------------------------------------------------------------------------*/
  (void) q_link(buf_ptr, &(buf_ptr->link));
  buf_ptr->event_cback_f_ptr = event_cback_f_ptr;
  buf_ptr->user_data_ptr     = user_data_ptr;
  buf_ptr->event_q_ptr       = NULL;

  return buf_ptr;

} /* ps_iface_alloc_event_cback_buf() */


/*===========================================================================
FUNCTION PS_IFACE_FREE_EVENT_CBACK_BUF()

DESCRIPTION
  This function is used to free the memory used to register for ps_iface
  event callbacks.

PARAMETERS
  buf: the previously allocated buffer.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_free_event_cback_buf
(
  void *buf
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if(buf == NULL)
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Validate the buffer that is passed in not already on a queue
  -------------------------------------------------------------------------*/
  if(Q_ALREADY_QUEUED(&(((ps_iface_event_buf_type*)buf)->link)))
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Invalidate the buffer so that the caller cannot use it without
    explicitly allcating it again.
  -------------------------------------------------------------------------*/
  ((ps_iface_event_buf_type *) buf)->event_cback_f_ptr = NULL;

  PS_MEM_FREE(buf);

} /* ps_iface_free_event_cback_buf() */


/*===========================================================================
FUNCTION PS_IFACE_EVENT_CBACK_REG()

DESCRIPTION
  Registers callbacks for events on an interface.  Whenever an event occurs,
  the callback for that event is called.  If the event is true when
  registration happens the callback is called immediately.  This will fail if
  either the interface is invalid, or the buffer to queue (with callback and
  user data pointer) is NULL.  A callback will only be queued if the event
  has not previously been queued.

  Memory is owned by the caller and it is put into the queue for that
  event.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  event:  Event that triggers the callbacks
  buf:    Buffer that holds the callback pointer. Should not be NULL.

RETURN VALUE
  0: on success (include
 -1: on failure (any of the passed in parameters is bad)

DEPENDENCIES
  The link field must have been initialized with q_link().

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_event_cback_reg
(
  ps_iface_type           *this_iface_ptr,
  ps_iface_event_enum_type event,
  void                    *buf
)
{
  ps_phys_link_type           * phys_link_ptr;
  ps_iface_event_buf_type     * buf_ptr;
  ps_iface_event_info_u_type    event_info;
  boolean                       need_to_invoke_cback = FALSE;
  void                        * dhcp_client_handle = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(buf == NULL || event >= IFACE_EVENT_MAX)
  {
    ASSERT(0);
    return -1;
  }

  buf_ptr = (ps_iface_event_buf_type*)buf;

  /*-------------------------------------------------------------------------
    Check if the event to be registered is valid
  -------------------------------------------------------------------------*/

  if(event >= IFACE_PHYS_LINK_MAX_EV && event < PHYS_LINK_MAX_EV)
  {
    LOG_MSG_FATAL_ERROR("Invalid Phys Link event %d to register for ps_iface",event, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Validate the buffer that is passed in
  -------------------------------------------------------------------------*/
  if(ps_mem_is_valid(buf, PS_MEM_PS_IFACE_EVT_CB_TYPE) == FALSE)
  {
    LOG_MSG_INFO1("Invalid ps_iface buf 0x%p to reg evt %d", buf, event, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    make sure that this is not already queued - if not mark it as such
  -------------------------------------------------------------------------*/
  if(Q_ALREADY_QUEUED(&(buf_ptr->link)))
  {
    LOG_MSG_INFO1("Not re-q'ing ps_iface buf 0x%p for evt %d", buf, event, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    As the contents of the buffer are supposed to be set by ps_iface
    functions it should NEVER have a NULL callback
  -------------------------------------------------------------------------*/
  if(buf_ptr->event_cback_f_ptr == NULL)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    is this for the global queue?
  -------------------------------------------------------------------------*/
  if (this_iface_ptr == NULL)
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
    buf_ptr->event_q_ptr = global_iface_event_q_array + event;
    q_put(global_iface_event_q_array + event, &(buf_ptr->link));
    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    return 0;
  } /* if(global queue) */

  /*-------------------------------------------------------------------------
    if not the global queue then make sure that the interface is valid
  -------------------------------------------------------------------------*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  buf_ptr->event_q_ptr =
    (this_iface_ptr->iface_private.event_q_array) + event;
  q_put((this_iface_ptr->iface_private.event_q_array) + event,
        &(buf_ptr->link));

  /*-------------------------------------------------------------------------
    Initializing "event_info" to invalid value so that lint won't crib
  -------------------------------------------------------------------------*/
  event_info.state = IFACE_STATE_INVALID;

  /*-------------------------------------------------------------------------
    check if the event is true - if so call the callback immediately
  -------------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(this_iface_ptr);
  switch (event)
  {
    /*-----------------------------------------------------------------------
                            IFACE_ENABLED_EV
    -----------------------------------------------------------------------*/
    case IFACE_ENABLED_EV:
      if(this_iface_ptr->iface_private.state != IFACE_DISABLED)
      {
        event_info.state = this_iface_ptr->iface_private.state;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_DISABLED_EV
    -----------------------------------------------------------------------*/
    case IFACE_DISABLED_EV:
      if(this_iface_ptr->iface_private.state == IFACE_DISABLED)
      {
        event_info.state = IFACE_DISABLED;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_DOWN_EV
    -----------------------------------------------------------------------*/
    case IFACE_DOWN_EV:
      if(this_iface_ptr->iface_private.state == IFACE_DOWN)
      {
        event_info.state = IFACE_DOWN;
        event_info.iface_down_info.netdown_reason =
          this_iface_ptr->event_info_cache.iface_down_info.netdown_reason;

        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_COMING_UP_EV
    -----------------------------------------------------------------------*/
    case IFACE_COMING_UP_EV:
      if(this_iface_ptr->iface_private.state == IFACE_COMING_UP)
      {
        event_info.state = IFACE_COMING_UP;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_CONFIGURING_EV
    -----------------------------------------------------------------------*/
    case IFACE_CONFIGURING_EV:
      if(this_iface_ptr->iface_private.state == IFACE_CONFIGURING)
      {
        event_info.state = IFACE_CONFIGURING;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_ROUTEABLE_EV
    -----------------------------------------------------------------------*/
    case IFACE_ROUTEABLE_EV:
      if(this_iface_ptr->iface_private.state == IFACE_ROUTEABLE)
      {
        event_info.state = IFACE_ROUTEABLE;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                               IFACE_UP_EV
    -----------------------------------------------------------------------*/
    case IFACE_UP_EV:
      if(this_iface_ptr->iface_private.state == IFACE_UP)
      {
        event_info.state = IFACE_UP;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_GOING_DOWN_EV
    -----------------------------------------------------------------------*/
    case IFACE_GOING_DOWN_EV:
      if(this_iface_ptr->iface_private.state == IFACE_GOING_DOWN)
      {
        event_info.state = IFACE_GOING_DOWN;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                            IFACE_EXTENDED_IP_CONFIG_EV
    -----------------------------------------------------------------------*/
    case IFACE_EXTENDED_IP_CONFIG_EV:
      if(this_iface_ptr->iface_private.state == IFACE_UP)
      {
        dhcp_client_handle = ps_iface_get_dhcp_client_handle(this_iface_ptr);
        if (NULL == dhcp_client_handle)
        {
          event_info.extended_ip_config_succeeded = TRUE;
          need_to_invoke_cback = TRUE;
        }
#ifdef FEATURE_DATA_PS_DHCP
        else if (TRUE == ps_iface_addr_family_is_v4(this_iface_ptr))
        {
          if(!dhcp_client_is_dhcp_in_progress(dhcp_client_handle))
          {
            event_info.extended_ip_config_succeeded = TRUE;
            need_to_invoke_cback = TRUE;
          }
        }
#endif /* FEATURE_DATA_PS_DHCP */
#ifdef FEATURE_DATA_PS_DHCPV6
        else if (TRUE == ps_iface_addr_family_is_v6(this_iface_ptr))
        {
          if(!dhcp6_client_is_dhcp_in_progress(dhcp_client_handle))
          {
            event_info.extended_ip_config_succeeded = TRUE;
            need_to_invoke_cback = TRUE;
          }
        }
#endif /* FEATURE_DATA_PS_DHCPV6 */
        else
        {
          LOG_MSG_ERROR("iface addr family does not match dhcp clients", 0, 0, 0);
        }
      }
      break;

    /*-----------------------------------------------------------------------
                         IFACE_PHYS_LINK_DOWN_EV
    -----------------------------------------------------------------------*/
    case IFACE_PHYS_LINK_DOWN_EV:
      if (PS_PHYS_LINK_GET_STATE(phys_link_ptr) == PHYS_LINK_DOWN)
      {
        event_info.phys_link_event_info.state = PHYS_LINK_DOWN;
        event_info.phys_link_event_info.info_code = PS_EIC_NOT_SPECIFIED;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                         IFACE_PHYS_LINK_COMING_UP_EV
    -----------------------------------------------------------------------*/
    case IFACE_PHYS_LINK_COMING_UP_EV:
      if (PS_PHYS_LINK_GET_STATE(phys_link_ptr) == PHYS_LINK_COMING_UP)
      {
        event_info.phys_link_event_info.state = PHYS_LINK_COMING_UP;
        event_info.phys_link_event_info.info_code = PS_EIC_NOT_SPECIFIED;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                          IFACE_PHYS_LINK_UP_EV
    -----------------------------------------------------------------------*/
    case IFACE_PHYS_LINK_UP_EV:
      if (PS_PHYS_LINK_GET_STATE(phys_link_ptr) == PHYS_LINK_UP)
      {
        event_info.phys_link_event_info.state = PHYS_LINK_UP;
        event_info.phys_link_event_info.info_code = PS_EIC_NOT_SPECIFIED;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                         IFACE_PHYS_LINK_GOING_DOWN_EV
    -----------------------------------------------------------------------*/
    case IFACE_PHYS_LINK_GOING_DOWN_EV:
      if (PS_PHYS_LINK_GET_STATE(phys_link_ptr) == PHYS_LINK_GOING_DOWN)
      {
        event_info.phys_link_event_info.state = PHYS_LINK_GOING_DOWN;
        event_info.phys_link_event_info.info_code = PS_EIC_NOT_SPECIFIED;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                         IFACE_PHYS_LINK_GONE_EV
    -----------------------------------------------------------------------*/
    case IFACE_PHYS_LINK_GONE_EV:
      if (PS_PHYS_LINK_GET_STATE(phys_link_ptr) == PHYS_LINK_NULL)
      {
        event_info.phys_link_event_info.state = PHYS_LINK_NULL;
        event_info.phys_link_event_info.info_code = PS_EIC_NOT_SPECIFIED;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                          IFACE_FLOW_ENABLED_EV
    -----------------------------------------------------------------------*/
    case IFACE_FLOW_ENABLED_EV:
      if(this_iface_ptr->iface_private.tx_flow_mask == 0)
      {
        event_info.flow_mask = 0;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                         IFACE_FLOW_DISABLED_EV
    -----------------------------------------------------------------------*/
    case IFACE_FLOW_DISABLED_EV:
      if(this_iface_ptr->iface_private.tx_flow_mask != 0)
      {
        event_info.flow_mask = this_iface_ptr->iface_private.tx_flow_mask;
        need_to_invoke_cback = TRUE;
      }
      break;

    /*-----------------------------------------------------------------------
                         IFACE_IPFLTR_UPDATED_EV

      Don't call this event callback during registration.
    -----------------------------------------------------------------------*/
    case IFACE_IPFLTR_UPDATED_EV:
      break;

    /*-----------------------------------------------------------------------
                           IFACE_MT_REQUEST_EV

        Don't call this event callback during registration.
    -----------------------------------------------------------------------*/
    case IFACE_MT_REQUEST_EV:
      break;

    /*-----------------------------------------------------------------------
                         IFACE_DELETE_EV and default

      It does not make sence to call the delete event here because the if
      interface has been deleted, then it is not in use and can't have
      callback called on/by it.
    -----------------------------------------------------------------------*/
    case IFACE_DELETE_EV:
      ASSERT(0);
      break;

    default:
      /* do nothing */;
      break;

  } /* switch(event) */

  /*-------------------------------------------------------------------------
    If the callback needs to be called, do so
  -------------------------------------------------------------------------*/
  if (need_to_invoke_cback)
  {
    buf_ptr->event_cback_f_ptr(this_iface_ptr,
                               event,
                               event_info,
                               buf_ptr->user_data_ptr);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_event_cback_reg() */



/*===========================================================================
FUNCTION PS_IFACE_EVENT_CBACK_DEREG()

DESCRIPTION
  DeRegisters callbacks for a specific event on an interface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  event:  Event that triggers the callbacks
  buf:    Buffer that holds the callback pointer. This buffer is removed
          from the queue of callbacks. Should not be NULL.

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_event_cback_dereg
(
  ps_iface_type           *this_iface_ptr,
  ps_iface_event_enum_type event,
  void                    *buf
)
{
  q_type                   * working_q_ptr = NULL;
  ps_iface_event_buf_type  * buf_ptr;
  boolean                    quit = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(event >= IFACE_EVENT_MAX || buf == NULL)
  {
    ASSERT(0);
    return;
  }

  buf_ptr = (ps_iface_event_buf_type*)buf;

  /*-------------------------------------------------------------------------
    Validate the buffer that is passed in
  -------------------------------------------------------------------------*/
  if(ps_mem_is_valid(buf, PS_MEM_PS_IFACE_EVT_CB_TYPE) == FALSE)
  {
    LOG_MSG_INFO1("Invalid buf 0x%p to dereg evt %d", buf, event, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    make sure that this event is on a queue - if not mark it as such
  -------------------------------------------------------------------------*/
  if(!Q_ALREADY_QUEUED(&(buf_ptr->link)))
  {
    LOG_MSG_INFO2("Buf 0x%p for event %d not on a queue", buf, event, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    As the contents of the buffer are supposed to be set by ps_iface
    functions it should NEVER have a NULL callback
  -------------------------------------------------------------------------*/
  if(buf_ptr->event_cback_f_ptr == NULL)
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    If this registration is for global callbacks, then use the global
    variable as the working pointer, otherwise use the q from the iface
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if (this_iface_ptr == NULL)
  {
    working_q_ptr = global_iface_event_q_array + event;
  }
  else if (PS_IFACE_IS_VALID(this_iface_ptr))
  {
    working_q_ptr = (this_iface_ptr->iface_private.event_q_array) + event;
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    ASSERT(0);
    LOG_MSG_ERROR("Invalid iface 0x%p is passed", this_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Make sure the event buf is either on an event queue of the specified
    iface or on the gloabl event queue
  -------------------------------------------------------------------------*/
  if(working_q_ptr != buf_ptr->event_q_ptr)
  {
    ASSERT(0);
    LOG_MSG_ERROR("Buf 0x%p (ev %d) not reg 0x%p iface",
              buf, event, this_iface_ptr);
    quit = TRUE;
  }

  /*-------------------------------------------------------------------------
    make sure we have a valid queue to remove from - if so use the queue
    function to remove the item from the queue.
  -------------------------------------------------------------------------*/
  if(quit == FALSE)
  {
#ifdef FEATURE_Q_NO_SELF_QPTR
    q_delete(working_q_ptr, &(buf_ptr->link));
#else
    q_delete(&(buf_ptr->link));
#endif
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return;

} /* ps_iface_event_cback_dereg() */



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
                                 INDICATIONS

  These are intended to only be called by the interface clients/owners. While
  there is no way to enforce this, having someone else call the indications
  is not supported behavior.
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION PS_IFACE_ENABLE_IND()

DESCRIPTION
  The given interface has been enabled.  Call callbacks.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_enable_ind
(
  ps_iface_type *this_iface_ptr
)
{
  ps_iface_event_info_u_type event_info;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE ENABLE IND 0x%x:%d, state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr));

  event_info.state = this_iface_ptr->iface_private.state;
  if(event_info.state == IFACE_DISABLED)
  {
    this_iface_ptr->iface_private.state = IFACE_DOWN;

    /*-----------------------------------------------------------------------
      Invoke Call back only if the iface state is disabled, not otherwise.
    -----------------------------------------------------------------------*/
    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_ENABLED_EV,
                                  event_info);
  }
  else
  {
    LOG_MSG_INFO2("Interface 0x%x:%d already enabled, state %d",
            this_iface_ptr->name,
            this_iface_ptr->instance,
            PS_IFACEI_GET_STATE(this_iface_ptr));
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_enable_ind() */

/*===========================================================================
FUNCTION PS_IFACE_DISABLED_IND()

DESCRIPTION
  The given interface has been disabled, if the interface is in any state
  other than DOWN then force the state DOWN first.  Call callbacks.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_disabled_ind
(
  ps_iface_type *this_iface_ptr
)
{
  ps_iface_event_info_u_type event_info;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE DISABLED IND 0x%x:%d, state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    copy the current state information into the event_info varible, then
    process based on this state.
  -------------------------------------------------------------------------*/
  event_info.state = PS_IFACEI_GET_STATE(this_iface_ptr);
  switch(event_info.state)
  {
  /*-----------------------------------------------------------------------
      Don't invoke callback if iface is already in DISABLED state.
    -----------------------------------------------------------------------*/
  case IFACE_DISABLED:
    LOG_MSG_INFO2("Interface 0x%x:%d already disabled, state %d",
            this_iface_ptr->name,
            this_iface_ptr->instance,
            PS_IFACEI_GET_STATE(this_iface_ptr));
    break;

  case IFACE_CONFIGURING:
  case IFACE_ROUTEABLE:
  case IFACE_UP:
  case IFACE_COMING_UP:
  case IFACE_GOING_DOWN:
  case IFACE_LINGERING:
    /*-----------------------------------------------------------------------
      If we are in any state other than down we want to call the down
      indication - then change the state for the interface.
    -----------------------------------------------------------------------*/
    ps_iface_down_ind(this_iface_ptr);
    this_iface_ptr->iface_private.state = IFACE_DISABLED;

    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_DISABLED_EV,
                                  event_info);
    break;

  case IFACE_DOWN:
    this_iface_ptr->iface_private.state = IFACE_DISABLED;

    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_DISABLED_EV,
                                  event_info);
    break;

  default:
    ASSERT(0);
  } /* switch(state) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_disabled_ind() */



/*===========================================================================
FUNCTION PS_IFACE_DOWN_IND()

DESCRIPTION
  The given interface has been stopped.   Call callbacks.  We set the address
  to zero (this allows the address server to OK the use of this address by
  someone else) and inform the address server.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_down_ind
(
  ps_iface_type *this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

    ps_iface_down_ind_ex(this_iface_ptr, PS_NET_DOWN_REASON_NOT_SPECIFIED);
  return;

} /* ps_iface_down_ind() */


/*===========================================================================
FUNCTION PS_IFACE_DOWN_IND_EX()

DESCRIPTION
  The given interface has been stopped with the given down reason.
  Call callbacks.  We set the address to zero (this allows the address
  server to OK the use of this address by someone else) and inform the
  address server.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ps_iface_net_down_reason_type : network down reason.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_down_ind_ex
(
  ps_iface_type *this_iface_ptr,
  ps_iface_net_down_reason_type down_reason
)
{
  ps_iface_event_info_u_type event_info;
#ifdef FEATURE_DATA_PS_MCAST
  int32                        timer_ret_val = 0;
  struct ps_mcast_cb         * cur_grp_ptr;
#endif /* FEATURE_DATA_PS_MCAST */
  int16 ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-----------------------------------------------------------------------
     If IGMP V1 querier present timer is running, it needs to be stopped.
  -----------------------------------------------------------------------*/
#ifdef FEATURE_DATA_PS_MCAST
  if (this_iface_ptr->iface_private.querier_v1_timer != 0)
  {
    if (ps_timer_is_running(this_iface_ptr->iface_private.querier_v1_timer))
    {
      timer_ret_val = ps_timer_cancel(
                        this_iface_ptr->iface_private.querier_v1_timer);
      if (timer_ret_val != PS_TIMER_SUCCESS)
      {
        LOG_MSG_ERROR("Error cancelling the timer", 0, 0, 0);
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        return;
      }
    }
  }
  /*-----------------------------------------------------------------------
     Cancel all the running timers for each of the multicast groups
     associated with this interface.
  -----------------------------------------------------------------------*/
  cur_grp_ptr = this_iface_ptr->iface_private.if_mcast_grps;
  while (cur_grp_ptr != NULL)
  {
    if (ps_timer_is_running(cur_grp_ptr->mcast_report_timer))
    {
      timer_ret_val = ps_timer_cancel(
                          cur_grp_ptr->mcast_report_timer);
      if (timer_ret_val != PS_TIMER_SUCCESS)
      {
        LOG_MSG_ERROR("Error cancelling the timer", 0, 0, 0);
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        return;
      }
    }
    cur_grp_ptr = cur_grp_ptr->next_mcast_grp;
  } /* end of while */
#endif /* FEATURE_DATA_PS_MCAST */

  /*-------------------------------------------------------------------------
    Make sure that all the secondary flows that belong to this iface are
    deleted
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_GET_NUM_SEC_FLOWS(this_iface_ptr) != 0)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Flows are still present on iface, 0x%x:%d",
              this_iface_ptr->name, this_iface_ptr->instance, 0);
    ASSERT(0);
    return;
  }

  LOG_MSG_INFO2("IFACE DOWN IND 0x%x:%d, state %d",
                this_iface_ptr->name,
                this_iface_ptr->instance,
                PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    The ref cnt is forced to zero.
  -------------------------------------------------------------------------*/
  PS_IFACE_RESET_REF_CNT(this_iface_ptr);

  switch(PS_IFACEI_GET_STATE(this_iface_ptr))
  {
    case IFACE_DOWN:
    case IFACE_COMING_UP:
    case IFACE_CONFIGURING:
    case IFACE_ROUTEABLE:
    case IFACE_UP:
    case IFACE_GOING_DOWN:
    case IFACE_LINGERING:
    /*-----------------------------------------------------------------------
      reset all of the IP configuration
    -----------------------------------------------------------------------*/
      memset(&(this_iface_ptr->net_info), 0, sizeof(network_info_type));
      this_iface_ptr->net_info.mtu = DEFAULT_DATA_UNIT;

      ps_iface_free_sip_addr_list(&this_iface_ptr->sip_serv_addr_list);
      ps_iface_free_domain_name_list
      (
        &this_iface_ptr->sip_serv_domain_name_list
      );

      ps_iface_free_domain_name_list
      (
        &this_iface_ptr->domain_name_search_list
      );

      if(ps_iface_addr_family_is_v4(this_iface_ptr))
      {
        this_iface_ptr->iface_private.ipv4_addr = 0;
      }
#ifdef FEATURE_DATA_PS_IPV6
      else if(ps_iface_addr_family_is_v6(this_iface_ptr))
      {
        (void) ps_iface_delete_all_v6_prefixes(this_iface_ptr);
      }
#endif /* FEATURE_DATA_PS_IPV6 */

#ifdef FEATURE_DATA_PS_MIPV6
      if(this_iface_ptr->mip6_bootstrap_info)
      {
        PS_MEM_FREE(this_iface_ptr->mip6_bootstrap_info);
      }
#endif /* FEATURE_DATA_PS_MIPV6 */

      this_iface_ptr->iface_private.tx_bridge_ptr = NULL;

      PS_IFACEI_RESET_IN_USE(this_iface_ptr);
      PS_IFACEI_RESET_LOOKED_UP(this_iface_ptr);
      PS_IFACEI_RESET_PROC_ID(this_iface_ptr);

      PS_IFACE_SET_ASSOC_IFACE(this_iface_ptr, NULL);
      PS_IFACE_SET_TRAT_IFACE(this_iface_ptr, NULL);

      ps_iface_update_stats_on_down_state(this_iface_ptr);

      /*---------------------------------------------------------------------
        Reset flow control mask. Otherwise, iface will forever be flow
        controlled forever.
      ---------------------------------------------------------------------*/
      ps_iface_enable_flow(this_iface_ptr,
                           this_iface_ptr->iface_private.tx_flow_mask);

      /*---------------------------------------------------------------------
        Store the previous state, change the current state to DOWN and call
        the callbacks.
      ---------------------------------------------------------------------*/
      event_info.state = this_iface_ptr->iface_private.state;
      this_iface_ptr->iface_private.state = IFACE_DOWN;

      /*---------------------------------------------------------------------
        If this Iface is being arbitrated on, overwrite down reason code,
        this solution is hacky and needs to change. The reason code shld
        come from mode handler.
      ---------------------------------------------------------------------*/
      if( PS_IFACEI_IS_IN_ARBITRATION( this_iface_ptr ) )
      {
        event_info.iface_down_info.netdown_reason =
          PS_NET_DOWN_REASON_APP_PREEMPTED;
        PS_IFACEI_RESET_ARBITRATION_IN_PROGRESS( this_iface_ptr );
      }
      else
      {
        event_info.iface_down_info.netdown_reason = down_reason;
      }

      LOG_MSG_INFO2( "Invoke Iface down ev callbacks 0x%x:%d, down reason %d",
                     this_iface_ptr->name,
                     this_iface_ptr->instance,
                     event_info.iface_down_info.netdown_reason );

      ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                    NULL,
                                    IFACE_DOWN_EV,
                                    event_info);

      /*---------------------------------------------------------------------
        intialize statistics
      ---------------------------------------------------------------------*/
      memset(&this_iface_ptr->stats, 0, sizeof(ps_iface_stats_type));
      memset(&this_iface_ptr->iface_i_stats,
             0,
             sizeof(this_iface_ptr->iface_i_stats));

      /*---------------------------------------------------------------------
        clear network config information in iface
      ---------------------------------------------------------------------*/
      memset( &(this_iface_ptr->net_cfg_params),
              0,
              sizeof(network_params_info_type) );

      /*---------------------------------------------------------------------
        clear the cache queue of the iface
      ---------------------------------------------------------------------*/
#ifdef FEATURE_DATA_PS_DNS
      (void)ps_iface_dns_cache_flush(this_iface_ptr, &ps_errno);
#endif
      break;

    case IFACE_DISABLED:
      LOG_MSG_ERROR("Cannot move disabled iface %d:%d to down",
                 this_iface_ptr->name,
                 this_iface_ptr->instance, 0);

      break;

    default:
      ASSERT(0);
  } /* switch(iface_state) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_down_ind() */



/*===========================================================================
FUNCTION PS_IFACE_CONFIGURING_IND()

DESCRIPTION
  The given interface has been set to configuring.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

PARAMETERS
  this_iface_ptr: Ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
// do a switch on all states: if disabled fail
void ps_iface_configuring_ind
(
  ps_iface_type *this_iface_ptr
)
{
  ps_flow_type                * ps_flow_ptr;
  ps_iface_event_info_u_type    event_info;
  ps_phys_link_type           * ps_phys_link_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE CONFIGURING IND 0x%x:%d, state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    Need to bind here since ifaces like SIO could potentially call
    routeable_ind() without calling iface_up_ind() first
  -------------------------------------------------------------------------*/
  /*-------------------------------------------------------------------------
    Can't bind default flow to primary phys link for logical ifaces since
    they don't have phys links. Binding is done explictly when logical iface
    associates with a physical iface

    Binding default flow of an iface with phys-link-dynamic-binding should
    already have happened before the iface reaching configuring state. This
    can be indicated by the non-NULL phys link pointer in the default flow.
  -------------------------------------------------------------------------*/
  if (!PS_IFACEI_IS_LOGICAL(this_iface_ptr))
  {
    ps_flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(this_iface_ptr);
    ps_phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(this_iface_ptr);
    if(ps_flow_ptr == NULL || ps_phys_link_ptr == NULL)
    {

      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("ps_iface_configuring_ind:flow_ptr or phys_link_ptr is NULL"
                 ,0,0,0);
      return;
    }
    else
    {
      (void)ps_flowi_bind_phys_link(ps_flow_ptr,ps_phys_link_ptr);
    }
  }

  /*-------------------------------------------------------------------------
    Store the old state, set the new state to configuring and call the
    callbacks
  -------------------------------------------------------------------------*/
  event_info.state = this_iface_ptr->iface_private.state;
  this_iface_ptr->iface_private.state = IFACE_CONFIGURING;
  LOG_MSG_INFO2 ("Calling event cbacks",0,0,0);
  ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                NULL,
                                IFACE_CONFIGURING_EV,
                                event_info);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_configuring_ind() */



/*===========================================================================
FUNCTION PS_IFACE_ROUTEABLE_IND()

DESCRIPTION
  The given interface has been set to routeable. The interface needs to be in
  the UP state before this indication is called.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
// do a switch on all states: if disabled fail
void ps_iface_routeable_ind
(
  ps_iface_type *this_iface_ptr
)
{
  ps_flow_type                * ps_flow_ptr;
  ps_iface_event_info_u_type    event_info;
  ps_phys_link_type           * ps_phys_link_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE ROUTEABLE IND 0x%x:%d, state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    Need to bind here since ifaces like SIO could potentially call
    routeable_ind() without calling iface_up_ind() first
  -------------------------------------------------------------------------*/
  /*-------------------------------------------------------------------------
    Can't bind default flow to primary phys link for logical ifaces since
    they don't have phys links. Binding is done explictly when logical iface
    associates with a physical iface

    Binding default flow of an iface with phys-link-dynamic-binding should
    already have happened before the iface reaching routeable state. This
    can be indicated by the non-NULL phys link pointer in the default flow.
  -------------------------------------------------------------------------*/
  if (!PS_IFACEI_IS_LOGICAL(this_iface_ptr))
  {
    ps_flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(this_iface_ptr);
    ps_phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(this_iface_ptr);
    if(ps_flow_ptr == NULL || ps_phys_link_ptr == NULL)
    {

      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("ps_iface_configuring_ind:flow_ptr or phys_link_ptr is NULL"
                 ,0,0,0);
      return;
    }
    else
    {
      (void)ps_flowi_bind_phys_link(ps_flow_ptr,ps_phys_link_ptr);
    }
  }

  /*-------------------------------------------------------------------------
    Store the old state, set the new state to routeable and call the callbacks
  -------------------------------------------------------------------------*/
  event_info.state = this_iface_ptr->iface_private.state;
  this_iface_ptr->iface_private.state = IFACE_ROUTEABLE;
  LOG_MSG_INFO2 ("Calling event cbacks",0,0,0);
  ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                NULL,
                                IFACE_ROUTEABLE_EV,
                                event_info);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_routeable_ind() */



/*===========================================================================
FUNCTION PS_IFACE_UP_IND()

DESCRIPTION
  The given interface is up.  Call callbacks.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
// do a switch on all states: if disabled fail
void ps_iface_up_ind
(
  ps_iface_type *this_iface_ptr
)
{
  ps_flow_type                * ps_flow_ptr;
  ps_iface_event_info_u_type    event_info;
  ps_phys_link_type           * ps_phys_link_ptr;
  void                        * dhcp_client_handle = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE UP IND 0x%x:%d, state %d",
                this_iface_ptr->name,
                this_iface_ptr->instance,
                PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    If the ref count is zero then move IFACE to LINGERING, else move to UP
  -------------------------------------------------------------------------*/
  if (this_iface_ptr->iface_private.ref_cnt == 0 &&
      this_iface_ptr->linger_cmd_f_ptr != NULL &&
      this_iface_ptr->iface_private.linger_timeout_val != 0)
  {
    event_info.state = this_iface_ptr->iface_private.state;
    this_iface_ptr->iface_private.state = IFACE_LINGERING;
    ps_ifacei_invoke_event_cbacks( this_iface_ptr,
                                   NULL,
                                   IFACE_LINGERING_EV,
                                   event_info);

    (void) this_iface_ptr->linger_cmd_f_ptr( this_iface_ptr, NULL);
  }
  else
  {
    /*-----------------------------------------------------------------------
      Need to bind here since ifaces like SIO post ps_iface_up_ind() directly
      without calling ps_iface_bring_up_cmd() first
    -----------------------------------------------------------------------*/
    /*-----------------------------------------------------------------------
      Can't bind default flow to primary phys link for logical ifaces since
      they don't have phys links. Binding is done explictly when logical iface
      associates with a physical iface

      Binding default flow of an iface with phys-link-dynamic-binding should
      already have happened before the iface reaching up state. This
      can be indicated by the non-NULL phys link pointer in the default flow.
    -----------------------------------------------------------------------*/
    if (!PS_IFACEI_IS_LOGICAL(this_iface_ptr))
    {
      ps_flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(this_iface_ptr);
      ps_phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(this_iface_ptr);
      if(ps_flow_ptr == NULL || ps_phys_link_ptr == NULL)
      {

        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        LOG_MSG_ERROR("ps_iface_up_ind():flow_ptr or phys_link_ptr is NULL",
                      0, 0, 0);
        return;
      }
      else
      {
        (void)ps_flowi_bind_phys_link(ps_flow_ptr,ps_phys_link_ptr);
      }
    }

    /*-------------------------------------------------------------------------
      update ps_iface statistics when moving to UP state.
    -------------------------------------------------------------------------*/
    ps_iface_update_stats_on_up_state(this_iface_ptr);

    /*-----------------------------------------------------------------------
      store the previous state, change the current state to UP and call the
      callbacks.
    -----------------------------------------------------------------------*/
    event_info.state = this_iface_ptr->iface_private.state;
    this_iface_ptr->iface_private.state = IFACE_UP;
    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_UP_EV,
                                  event_info);

    dhcp_client_handle = ps_iface_get_dhcp_client_handle(this_iface_ptr);

    if (NULL == dhcp_client_handle)
    {
      event_info.extended_ip_config_succeeded = TRUE;
      ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                    NULL,
                                    IFACE_EXTENDED_IP_CONFIG_EV,
                                    event_info);
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_up_ind() */



/*===========================================================================
FUNCTION PS_IFACE_PHYS_LINK_DOWN_IND()

DESCRIPTION
  The physical link of the given interface is down (707 dormancy).  Call
  callbacks.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
// do a switch on all phys link states
void ps_iface_phys_link_down_ind
(
  const ps_iface_type *this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE PHYS LINK DOWN IND 0x%x:%d, state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    Call the underlying phys link down ind.
  -------------------------------------------------------------------------*/
  ps_phys_link_down_ind(PS_IFACEI_GET_PHYS_LINK(this_iface_ptr));

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_phys_link_down_ind() */



/*===========================================================================
FUNCTION PS_IFACE_PHYS_LINK_UP_IND()

DESCRIPTION
  The physical link of the given interface is up.  Call callbacks.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
// do a switch on all phys link states
void ps_iface_phys_link_up_ind
(
  const ps_iface_type *this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE PHYS LINK UP IND 0x%x:%d, state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr));

  /*-------------------------------------------------------------------------
    Call the underlying phys link up ind.
  -------------------------------------------------------------------------*/
  ps_phys_link_up_ind(PS_IFACEI_GET_PHYS_LINK(this_iface_ptr));

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_phys_link_up_ind() */


#ifdef FEATURE_DATA_PS_IPV6
/*===========================================================================
FUNCTION PS_IFACE_VALID_RA_IND()

DESCRIPTION
  This fucntion is called to indicate that a valid RA is received
  Appropriate event callback is posted.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on
  ra_life_time: lifetime of the RA

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_valid_ra_ind
(
  ps_iface_type                       *this_iface_ptr,
  uint16                               ra_life_time
)
{
  ps_iface_event_info_u_type event_info;
  ps_ipv6_iid_params_type    iid_param;
  ps_ip_addr_type            ip_addr;
  int16                      ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr) || ra_life_time == 0)
  {
    ASSERT(0);
    return;
  }

  LOG_MSG_INFO2("IFACE VALID RA IND 0x%x:%d, lifetime %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          ra_life_time);

  /*-------------------------------------------------------------------------
    Although iface contents are not changed, need to enter critical section
    as events are posted
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Valid Prefix was received, if IPv6 privacy extensions are enabled
    generate a new private shareable address.
  -------------------------------------------------------------------------*/
    iid_param.is_unique   = FALSE;
    iid_param.app_request = FALSE;

    if( -1 == ps_iface_generate_priv_ipv6_addr(this_iface_ptr,
                                               &ip_addr,
                                               &iid_param,
                                               &ps_errno))
    {
      if(ps_errno != DS_EWOULDBLOCK)
      {
        LOG_MSG_ERROR("Failed to create default IPv6 private shareable addr",0,0,0);
      }
    }

  /*-------------------------------------------------------------------------
    Call the event callback passing the RA life time
  -------------------------------------------------------------------------*/
  event_info.ra_lifetime = ra_life_time;

  ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                NULL,
                                IFACE_VALID_RA_EV,
                                event_info);

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return;

} /* ps_iface_valid_ra_ind() */
#endif /* FEATURE_DATA_PS_IPV6 */


/*===========================================================================
FUNCTION PS_IFACE_IPFLTR_UPDATED_IND()

DESCRIPTION
  This fucntion is called to indicate that the number of IP filters
  installed in this iface have changed. Appropriate event callback
  is posted.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on
  client: filtering client id for which the filters changed
  n_added: number of new filters added
  n_deleted: number of existing filters deleted

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_ipfltr_updated_ind
(
  ps_iface_type                       *this_iface_ptr,
  ps_iface_ipfltr_client_id_enum_type client,
  int                                 prev_cnt,
  int                                 new_cnt
)
{
  ps_iface_event_info_u_type    event_info;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (this_iface_ptr != NULL && !PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Although iface contents are not changed, need to enter critical section
    as events are posted
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Call the event callback passing the currently installed num ip filters
  -------------------------------------------------------------------------*/
  event_info.ipfltr_info.client        = client;
  event_info.ipfltr_info.curr_fltr_cnt = new_cnt;

  MSG_4(MSG_SSID_DFLT,
        MSG_LEGACY_MED,
        "If 0x%p, client %d, n_filters %d -> %d",
        this_iface_ptr,
        client,
        prev_cnt,
        new_cnt);

  ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                NULL,
                                IFACE_IPFLTR_UPDATED_EV,
                                event_info);

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_ipfltr_updated_ind() */



/*===========================================================================
FUNCTION PS_IFACE_GENERIC_IND()

DESCRIPTION
  This function is called to indicate that an iface event has occurred. This
  function is to be used for iface events that are not directly applicable
  to the iface itself. (Ie. generic iface events)

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on
  event: event that has occurred.
  usr_data_ptr: data associated with the event.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_generic_ind
(
  ps_iface_type                       *this_iface_ptr,
  ps_iface_event_enum_type             event,
  void                                *event_data_ptr
)
{
  ps_iface_event_info_u_type event_info;
  ps_iface_mcast_event_info_type *mcast_info = NULL;
  ps_iface_ioctl_bearer_tech_changed_type   *tech_change = NULL;
  ps_iface_ioctl_extended_ip_config_type *extended_ip_config_info = NULL;
#ifdef FEATURE_HDR_BCMCS_2p0
  ps_iface_event_info_u_type dup_event_info;
  ps_iface_event_enum_type dup_event = IFACE_MIN_EV;
#endif /* FEATURE_HDR_BCMCS_2p0 */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  LOG_MSG_INFO2("IFACE GENERIC EVENT IND 0x%x:%d, event %d",
          this_iface_ptr->name, this_iface_ptr->instance, event);

  /*-------------------------------------------------------------------------
    Initializing event_info to shut up lint
  -------------------------------------------------------------------------*/
  event_info.n_filters = 0;

  /*-------------------------------------------------------------------------
    Although iface contents are not changed, need to enter critical section
    as events are posted
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  switch (event)
  {
    /*-----------------------------------------------------------------------
      IFACE_MT_REQUEST_EV takes an event_data_ptr of type
      ps_iface_mt_handle_type *
    -----------------------------------------------------------------------*/
    case IFACE_MT_REQUEST_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      event_info.mt_handle = *(ps_iface_mt_handle_type *)(event_data_ptr);
      break;

    /*-----------------------------------------------------------------------
      Multicast events take an event_data_ptr of type
      ps_iface_mcast_event_info_type
    -----------------------------------------------------------------------*/
    case IFACE_MCAST_REGISTER_SUCCESS_EV:
    case IFACE_MCAST_REGISTER_FAILURE_EV:
    case IFACE_MCAST_DEREGISTERED_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      mcast_info                       = (ps_iface_mcast_event_info_type *)
                                         event_data_ptr;
      event_info.mcast_info.handle     = mcast_info->handle;
      event_info.mcast_info.info_code  = mcast_info->info_code;

#ifdef FEATURE_HDR_BCMCS_2p0
      dup_event_info.mcast_info.handle = mcast_info->handle;

      /*---------------------------------------------------------------------
        force_dereg_cbacks is set to TRUE for the original events and FALSE
        in the duplicate events. This ensures that if a dupilcate DEREG_EV
        is generated, it will not clear the application's event callback
        buffer.
        -------------------------------------------------------------------*/
      event_info.mcast_info.force_dereg_cbacks     = TRUE;
      dup_event_info.mcast_info.force_dereg_cbacks = FALSE;

      /*---------------------------------------------------------------------
          Specific info code handling:
            1. IC < PS_IFACE_MCAST_BCMCS_MAX_DEPRECATED_INFO_CODE:
              All such info codes will be duplicated into their 2p0
              counterparts and sent with a duplicated event
              (for e.g., MCAST_STATUS_EV for transient errors and REG_FAIL
              for permanent errors)
            2. IC > PS_IFACE_MCAST_BCMCS_MAX_DEPRECATED_INFO_CODE:
              Only specififc new BCMCS 2.0 info codes will be duplicated to
              PS_IFACE_MCAST_IC_NOT_SPECIFIED  to be backward compatible
              with older apps.
        -------------------------------------------------------------------*/
      switch (mcast_info->info_code)
      {
        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_CANCELLED:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_CANCELLED;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNABLE_TO_MONITOR:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_UNABLE_TO_MONITOR;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_REQUESTED:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_REQUESTED;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_TIMEOUT:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_TIMEOUT;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_LOST:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_LOST;
          if (event == IFACE_MCAST_REGISTER_FAILURE_EV)
          {
            dup_event = IFACE_MCAST_STATUS_EV;
          }
          else if (event == IFACE_MCAST_DEREGISTERED_EV)
          {
            dup_event = IFACE_MCAST_DEREGISTERED_EV;
          }
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_SYS_UNAVAILABLE:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_SYS_UNAVAILABLE;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE;
          dup_event = IFACE_MCAST_REGISTER_FAILURE_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED;
          dup_event = IFACE_MCAST_REGISTER_FAILURE_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG;
          dup_event = IFACE_MCAST_REGISTER_FAILURE_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNAVAILABLE:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_UNAVAILABLE;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_NO_MAPPING:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_NO_MAPPING;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_ID_NOT_FOUND_FOR_GIVEN_MULTICAST_IP:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_ID_NOT_FOUND_FOR_GIVEN_MULTICAST_IP;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_MAX_FLOWS_REACHED:
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS2p0_MAX_FLOWS_REACHED;
          dup_event = IFACE_MCAST_REGISTER_FAILURE_EV;
          break;

        /*-------------------------------------------------------------
          Special cases: These are the new BCMCS info codes meant to be
          sent to newer apps only using BCMCS2.0. All these info codes
          are duplicated using older info codes for older apps.
        -------------------------------------------------------------*/
        case PS_IFACE_MCAST_BCMCS_JOIN_REQ_IN_PROGRESS:
          /* STATUS event gets the new code. But, for backward compatibility:
             set the info code to NOT SPECIFIED for REG SUCC event */
          event_info.mcast_info.info_code = PS_IFACE_MCAST_IC_NOT_SPECIFIED;
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS_JOIN_REQ_IN_PROGRESS;
          dup_event = IFACE_MCAST_STATUS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_STATUS_MONITORED:
          /* For backward compatibility, duplicate REG SUCC event
             with the new info code and original with old info code */
          event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_IC_NOT_SPECIFIED;
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS_FLOW_STATUS_MONITORED;
          dup_event = IFACE_MCAST_REGISTER_SUCCESS_EV;
          break;

        case PS_IFACE_MCAST_BCMCS_FLOW_DEREGISTERED:
          /* For backward compatibility, duplicate DEREG event
             with the new info code and original with old info code */
          event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_IC_NOT_SPECIFIED;
          dup_event_info.mcast_info.info_code =
                   PS_IFACE_MCAST_BCMCS_FLOW_DEREGISTERED;
          dup_event = IFACE_MCAST_DEREGISTERED_EV;
          break;

        default:
          break;
      }

      /* Post duplicate event here, if it is set */
      if (dup_event > IFACE_MIN_EV)
      {
          ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                        NULL,
                                        dup_event,
                                        dup_event_info);
      }
      break;

    case IFACE_MCAST_STATUS_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      mcast_info = (ps_iface_mcast_event_info_type *)event_data_ptr;
      event_info.mcast_info.handle    = mcast_info->handle;
      event_info.mcast_info.info_code = mcast_info->info_code;
#endif /* FEATURE_HDR_BCMCS_2p0 */
      break;

    /*-----------------------------------------------------------------------
      IFACE_BEARER_TECH_CHANGED_EV takes an event_data_ptr of type
      ps_iface_ioctl_bearer_tech_changed_type *
    -----------------------------------------------------------------------*/
    case IFACE_BEARER_TECH_CHANGED_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      tech_change = (ps_iface_ioctl_bearer_tech_changed_type *)event_data_ptr;
      memcpy (&(event_info.bearer_tech_changed_info.old_bearer_tech),
              &(tech_change->old_bearer_tech),
              sizeof (ps_iface_bearer_technology_type));
      memcpy (&(event_info.bearer_tech_changed_info.new_bearer_tech),
              &(tech_change->new_bearer_tech),
              sizeof (ps_iface_bearer_technology_type));
      break;
    case IFACE_EXTENDED_IP_CONFIG_EV:
      if(event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR,
             "Invalid EXTENDED_IP_CONFIG event data ptr %d", event);
        ASSERT(0);
        return;
      }
      extended_ip_config_info =
        (ps_iface_ioctl_extended_ip_config_type *) event_data_ptr;
      event_info.extended_ip_config_succeeded = *extended_ip_config_info;
      break;
    /*-----------------------------------------------------------------------
      IFACE_QOS_AWARE/UNAWARE_SYSTEM_EV takes an event_data_ptr of type
      ps_extended_info_code_enum_type *
    -----------------------------------------------------------------------*/
    case IFACE_QOS_AWARE_SYSTEM_EV:
    case IFACE_QOS_UNAWARE_SYSTEM_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      event_info.qos_aware_info_code =
        *(ps_extended_info_code_enum_type *)(event_data_ptr);
      break;

    case IFACE_FLOW_ADDED_EV:
      event_info.flow_ptr = (ps_flow_type *)(event_data_ptr);
      break;

    case IFACE_FLOW_DELETED_EV:
    case IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV:
      break;

    case IFACE_OUTAGE_NOTIFICATION_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      event_info.outage_notification_info =
        *( (ps_iface_outage_notification_event_info_type *) event_data_ptr);
      event_info.outage_notification_info.time_to_outage += (uint32) msclock();
      break;

    case IFACE_RF_CONDITIONS_CHANGED_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      event_info.rf_conditions_change_info =
        *( (ps_iface_rf_conditions_info_type *) event_data_ptr);
      break;

    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV:
      break;

    case IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }
      event_info.hdr_rev0_rate_inertia_failure_code =
        *(ps_hdr_rev0_rate_inertia_failure_code_enum_type*)(event_data_ptr);
      break;

    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV:
      break;

    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      event_info.slotted_mode_info.hdr_slotted_mode_failure_code =
        *(ps_hdr_slotted_mode_failure_code_enum_type*)(event_data_ptr);
      break;

    case IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV:
       if (event_data_ptr == NULL)
       {
         PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
         MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
         ASSERT(0);
         return;
       }

       event_info.slotted_mode_info.sm_current_sci = *(uint8*)(event_data_ptr);
       break;

    case IFACE_MBMS_CONTEXT_ACT_SUCCESS_EV:
    case IFACE_MBMS_CONTEXT_ACT_FAILURE_EV:
    case IFACE_MBMS_CONTEXT_DEACT_SUCCESS_EV:
    case IFACE_MBMS_CONTEXT_DEACT_FAILURE_EV:
      if (event_data_ptr == NULL)
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        MSG_1(MSG_SSID_DS, MSG_LEGACY_ERROR, "NULL event data ptr %d", event);
        ASSERT(0);
        return;
      }

      mcast_info = (ps_iface_mcast_event_info_type *)event_data_ptr;
      event_info.mcast_info.handle    = mcast_info->handle;
      break;

    case IFACE_AUTHENTICATING_EV:
    case IFACE_APP_PREEMPTED_EV:
      break;

    default:
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_INFO2("Invalid event %d on iface 0x%x:%d",
              event, this_iface_ptr->name, this_iface_ptr->instance);
      ASSERT(0);
      return;
  }

  ps_ifacei_invoke_event_cbacks(this_iface_ptr, NULL, event, event_info);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_generic_ind() */



#ifdef FEATURE_DATA_PS_QOS
/*===========================================================================
FUNCTION PS_IFACE_CHANGE_PRI_PHYS_LINK()

DESCRIPTION
  This function is called to indicate that the primary phys link associated
  with the iface has changed.  The new primary phys link should be specified.

  THIS FUNCTION SHOULD NOT BE CALLED IN A ISR.

PARAMETERS
  iface_ptr: ptr to interface control block on which to operate on
  phys_link_ptr:  New primary phys link.
  info_code:      Extended info code indicating the reason of the primary
                  change.

RETURN VALUE
  None

DEPENDENCIES
  phys_link_ptr must be associatd with this iface.

SIDE EFFECTS
  Clients implcitly accessing the primary phys link through the iface ptr
  will obtain the new phys link.

  All the flows that are bound to old primary are migrated to new primary,
  and vice versa

  Capabilities of old and new primary phys links are swapped
===========================================================================*/
void ps_iface_change_pri_phys_link
(
  ps_iface_type                    * iface_ptr,
  ps_phys_link_type                * new_pri_phys_link_ptr,
  ps_extended_info_code_enum_type    info_code
)
{
  ps_phys_link_type           * curr_pri_phys_link_ptr;
  ps_flow_type                * flow_ptr;
  ps_iface_event_info_u_type    event_info;
  uint8                         tmp_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(iface_ptr) ||
      !PS_PHYS_LINK_IS_VALID(new_pri_phys_link_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p or phys link 0x%p",
              iface_ptr, new_pri_phys_link_ptr, 0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("IFACE PRI PHYS LINK CHANGE IND 0x%x:%d, state %d",
          iface_ptr->name,
          iface_ptr->instance,
          PS_IFACEI_GET_STATE(iface_ptr));

  curr_pri_phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(iface_ptr);

  if(curr_pri_phys_link_ptr == NULL)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("ps_iface_change_pri_phys_link : "
              "Invalid phys_link pointer 0x%p ",
              curr_pri_phys_link_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Make sure the specified new primary phys link is associated with this
    iface.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IS_PHYS_LINK_VALID(iface_ptr, new_pri_phys_link_ptr))
  {
    iface_ptr->iface_private.phys_link.primary =
      PS_PHYS_LINKI_GET_INST(new_pri_phys_link_ptr);
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Iface 0x%p, no associated phys link 0x%p",
              iface_ptr, new_pri_phys_link_ptr, 0);
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Migrate all the flows, that are bound to old primary, to new primary and
    vice versa. Bind default flow to new primary
  -------------------------------------------------------------------------*/
  (void) ps_flowi_unbind_phys_link(PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr));

  flow_ptr = list_peek_front(&(iface_ptr->iface_private.flow.sec_flow_list));
  while (flow_ptr != NULL)
  {
    if (PS_FLOWI_GET_PHYS_LINK(flow_ptr) == new_pri_phys_link_ptr)
    {
      (void) ps_flowi_rebind_phys_link(flow_ptr, curr_pri_phys_link_ptr);
    }
    else if (PS_FLOWI_GET_PHYS_LINK(flow_ptr) == curr_pri_phys_link_ptr)
    {
      (void) ps_flowi_rebind_phys_link(flow_ptr, new_pri_phys_link_ptr);
    }

    flow_ptr = list_peek_next(&iface_ptr->iface_private.flow.sec_flow_list,
                              &flow_ptr->link);
  }

  (void) ps_flowi_bind_phys_link(PS_IFACEI_GET_DEFAULT_FLOW(iface_ptr),
                                 new_pri_phys_link_ptr);

  /*-------------------------------------------------------------------------
    Swap capability of old and new primary phys links
  -------------------------------------------------------------------------*/
  tmp_val = curr_pri_phys_link_ptr->phys_private.capability_mask;
  curr_pri_phys_link_ptr->phys_private.capability_mask =
    new_pri_phys_link_ptr->phys_private.capability_mask;
  new_pri_phys_link_ptr->phys_private.capability_mask = tmp_val;

  /*-------------------------------------------------------------------------
    Call the event callback passing the ptr to previous pri phys link
  -------------------------------------------------------------------------*/
  event_info.pri_changed_info.pri_phys_link_ptr = curr_pri_phys_link_ptr;
  event_info.pri_changed_info.info_code         = info_code;

  LOG_MSG_INFO1("If 0x%p, changed pri PL from 0x%p to 0x%p",
           iface_ptr,
           event_info.pri_changed_info.pri_phys_link_ptr,
           new_pri_phys_link_ptr);

  ps_ifacei_invoke_event_cbacks(iface_ptr,
                                NULL,
                                IFACE_PRI_PHYS_LINK_CHANGED_EV,
                                event_info);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_change_pri_phys_link() */
#endif /* FEATURE_DATA_PS_QOS */



/*===========================================================================
FUNCTION PS_IFACEI_DORM_STATUS

DESCRIPTION
  Called by the underlying phys link when the phys link wants to either
  orig from dormancy or is going dormant.  Calls functions registered by
  IFACE mgr to return information to phys link.  If no functions are
  registered, then returns TRUE (i.e, meaning go dormant or ok to orig
  from dorm).

PARAMETERS
  phys_link_ptr:  Ptr to phys link control block on which is asking the
                  question.
  action:         Either wants to orig or wants to go dormant.
  reason:         Applies only for orig's.  Indicates why we are originating.
  unique_mem_ptr: Most likely (and should be) set to to the iface_ptr.

RETURN VALUE
  - 1:  Do not go dormant or do not orig from dormancy.
    0:  Ok to go dormant or ok to orig from dormancy.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_ifacei_dorm_status
(
  ps_phys_link_type               *phys_link_ptr,
  ps_iface_dorm_action_enum_type   action,
  ps_iface_dorm_reason_enum_type   reason,
  void                            *unique_mem_ptr
)
{
  ps_iface_type     *this_iface_ptr;
  int                ret_val = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  this_iface_ptr = (ps_iface_type *)unique_mem_ptr;

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    LOG_MSG_FATAL_ERROR("Invalid ps_iface",0,0,0);
    return(-1);
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if ((PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_DOWN) ||
      (PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_DISABLED))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("Dorm status when iface 0x%p down or disabled",
              this_iface_ptr,0,0);
    return(-1);
  }

  if (this_iface_ptr->dorm_action_f_ptr != NULL)
  {
    ret_val = this_iface_ptr->dorm_action_f_ptr(this_iface_ptr,
                                                phys_link_ptr,
                                                action,
                                                reason);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return(ret_val);
} /* ps_ifacei_dorm_status() */



/*===========================================================================
FUNCTION PS_IFACEI_INVOKE_EVENT_CBACKS()

DESCRIPTION
  This function will invoke all of the event callbacks for a given interface
  and event.  It will also call the global callbacks for said event, if any.

PARAMETERS
  this_iface_ptr: ptr to the interface on which we are operating
  event: the event for which callbacks need to be called
  event_info: event information that needs to be passed into callback.

RETURN VALUE
  None

DEPENDENCIES
  Must be called in a iface critical section

SIDE EFFECTS
  None
===========================================================================*/
void ps_ifacei_invoke_event_cbacks
(
  ps_iface_type             *this_iface_ptr,
  ps_phys_link_type         *this_phys_link_ptr,
  ps_iface_event_enum_type   event,
  ps_iface_event_info_u_type event_info
)
{
  q_type                       * working_q_ptr        = NULL;
  q_type                       * working_global_q_ptr = NULL;
  void                         * event_buf_ptr;
  void                         * next_event_buf_ptr;
  ps_iface_event_buf_type      * ps_iface_event_buf_ptr;
  ps_phys_link_event_buf_type  * phys_link_event_buf_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    If iface ptr is not NULL them validate iface ptr.
  -------------------------------------------------------------------------*/
  if (this_iface_ptr != NULL && !PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    If physlink ptr is not NULL them validate phys link ptr - also make sure
    the event is in the valid range.
  -------------------------------------------------------------------------*/
  if (this_phys_link_ptr != NULL &&
        (!PS_PHYS_LINK_IS_VALID(this_phys_link_ptr) ||
          event > PHYS_LINK_MAX_EV))
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    Validate if we need to invoke callbacks. If iface or phys_link is already
    in the same state as the occuring event, then do not invoke callbacks.
  -------------------------------------------------------------------------*/
  if (FALSE == ps_ifacei_validate_event_cbacks(this_iface_ptr,
                                               this_phys_link_ptr,
                                               event,
                                               event_info))
  {
    LOG_MSG_INFO2("Not invoking event %d ", event, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Start with the local callbacks
  -------------------------------------------------------------------------*/
  if (this_iface_ptr == NULL && this_phys_link_ptr == NULL)
  {
    working_q_ptr        = global_iface_event_q_array + event;
    working_global_q_ptr = global_iface_event_q_array + event;
  }
  else if (this_iface_ptr != NULL)
  {
    working_q_ptr = (this_iface_ptr->iface_private.event_q_array) + event;
    working_global_q_ptr = global_iface_event_q_array + event;
  }
  else if (this_phys_link_ptr != NULL)
  {
    /*-----------------------------------------------------------------------
      Phys link ptr will always be non-NULL if we reach here, the if is
      just to satisfy lint
    -----------------------------------------------------------------------*/
    working_q_ptr = (this_phys_link_ptr->phys_private.event_q_array) + event;
    working_global_q_ptr = global_phys_link_event_q_array + event;
  }

  for (;;)
  {
    /*-----------------------------------------------------------------------
      Traverse the queue of callbacks for this event
    -----------------------------------------------------------------------*/
    event_buf_ptr = q_check(working_q_ptr);
    while (event_buf_ptr != NULL)
    {
      next_event_buf_ptr =
        q_next(working_q_ptr, &((q_generic_item_type *) event_buf_ptr)->link);

      /*---------------------------------------------------------------------
        As both ps_iface and ps_phys_link use this function to call callback,
        cast event_buf_ptr to relevant type and post callback appropriately
      ---------------------------------------------------------------------*/
      if (this_iface_ptr != NULL)
      {
        ps_iface_event_buf_ptr = (ps_iface_event_buf_type *) event_buf_ptr;
        ps_iface_event_buf_ptr->event_cback_f_ptr
        (
          this_iface_ptr,
          event,
          event_info,
          ps_iface_event_buf_ptr->user_data_ptr
        );
      }
      else
      {
        phys_link_event_buf_ptr =
          (ps_phys_link_event_buf_type *) event_buf_ptr;
        phys_link_event_buf_ptr->event_cback_f_ptr
        (
          this_phys_link_ptr,
          event,
          event_info,
          phys_link_event_buf_ptr->user_data_ptr
        );
      }

      event_buf_ptr = next_event_buf_ptr;
    } /* while (items in queue) */

    /*-----------------------------------------------------------------------
      If the working_q_ptr is set to qlobal_event_q_array then we are done,
      otherwise we need to call those callbacks - but only if the global
      event q has been initialized.
    -----------------------------------------------------------------------*/
    if (working_q_ptr == working_global_q_ptr)
    {
      break;
    }
    else
    {
      working_q_ptr = working_global_q_ptr;
    }
  } /* for (all specific and global callbacks) */

  /*-------------------------------------------------------------------------
    Now call callbacks for IFACE_ALL_EV registration
  -------------------------------------------------------------------------*/
  if (this_iface_ptr != NULL && event > PHYS_LINK_MAX_EV)
  {
    working_q_ptr = this_iface_ptr->iface_private.event_q_array + IFACE_ALL_EV;

    /*---------------------------------------------------------------------
      Traverse the queue of callbacks for this event
    ---------------------------------------------------------------------*/
    event_buf_ptr = q_check(working_q_ptr);

    while (event_buf_ptr != NULL)
    {
      ps_iface_event_buf_ptr = (ps_iface_event_buf_type *) event_buf_ptr;
      ps_iface_event_buf_ptr->event_cback_f_ptr
        (
          this_iface_ptr,
          event,
          event_info,
          ps_iface_event_buf_ptr->user_data_ptr
        );

      event_buf_ptr = q_next(working_q_ptr,
                               &((q_generic_item_type *) event_buf_ptr)->link);

    } /* while (items in queue) */
  }

} /* ps_ifacei_invoke_event_cbacks() */

static void ps_iface_update_stats_on_up_state
(
 ps_iface_type  *this_iface_ptr
 )
{
  uint32 dwNow = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  dwNow = (uint32)msclock();

  if(0 == this_iface_ptr->iface_i_stats.bytes_transferred)
  {
    this_iface_ptr->iface_i_stats.bytes_transferred =
      this_iface_ptr->iface_i_stats.bytes_rx +
      this_iface_ptr->iface_i_stats.bytes_tx;
  }
  if(0 == this_iface_ptr->iface_i_stats.last_up_state_timestamp)
  {
    this_iface_ptr->iface_i_stats.last_up_state_timestamp = dwNow;
  }
}/* ps_iface_update_stats_on_up_state() */

static void ps_iface_update_stats_on_down_state
(
 ps_iface_type  *this_iface_ptr
 )
{
  uint32 dwNow = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  dwNow = (uint32)msclock();

  /*-----------------------------------------------------------------------
    update total counters
  -----------------------------------------------------------------------*/
  this_iface_ptr->iface_i_stats.total_bytes_transferred_since_power_up +=
    ((0 == this_iface_ptr->iface_i_stats.bytes_transferred)
      ? 0 : (this_iface_ptr->iface_i_stats.bytes_rx +
             this_iface_ptr->iface_i_stats.bytes_tx -
             this_iface_ptr->iface_i_stats.bytes_transferred)
    );
  this_iface_ptr->iface_i_stats.total_open_time_since_power_up +=
    ((0 == this_iface_ptr->iface_i_stats.last_up_state_timestamp)
      ? 0 : (dwNow - this_iface_ptr->iface_i_stats.last_up_state_timestamp)
    );
  this_iface_ptr->iface_i_stats.total_active_time_since_power_up +=
    ((0 == this_iface_ptr->iface_i_stats.first_io_timestamp)
      ? 0 : (dwNow - this_iface_ptr->iface_i_stats.first_io_timestamp)
    );
  /*-----------------------------------------------------------------------
    clear counters
  -----------------------------------------------------------------------*/
  this_iface_ptr->iface_i_stats.first_io_timestamp = 0;
  this_iface_ptr->iface_i_stats.last_up_state_timestamp = 0;
  this_iface_ptr->iface_i_stats.bytes_transferred = 0;

} /* ps_iface_update_stats_on_down_state() */

#endif /* FEATURE_DATA_PS */

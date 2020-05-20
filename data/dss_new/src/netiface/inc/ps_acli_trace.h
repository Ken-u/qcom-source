#ifndef PS_ACLI_TRACE_H
#define PS_ACLI_TRACE_H
/*===========================================================================
@file ps_acli_trace.h

This file contains the internal ACL rules macros. The internal macros are
used in the external macros, along with some additional rules.


            Copyright (c) 2010 Qualcomm Technologies, Inc.
            All Rights Reserved.
            Qualcomm Technologies Confidential and Proprietary

===========================================================================*/

/* $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/inc/ps_acli_trace.h#1 $ */

/*===========================================================================

                           INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

/*===========================================================================

                              DECLARATIONS

===========================================================================*/
extern boolean is_inbound_acl;

#define ACL_LEVEL_LOW    MSG_LEGACY_LOW
#define ACL_LEVEL_MED    MSG_LEGACY_MED
#define ACL_LEVEL_HIGH   MSG_LEGACY_HIGH

#define ACL_TRACE_DUMP(level, str, arg1, arg2, arg3)                        \
  if (is_inbound_acl == FALSE)                                              \
  {                                                                         \
    MSG_SPRINTF_3( MSG_SSID_DS_ACLPOLICY, level, str, arg1, arg2, arg3);    \
  }

#define ACL_TRACE_DUMP_6(level, str, arg1, arg2, arg3, arg4, arg5, arg6)    \
  if (is_inbound_acl == FALSE)                                              \
  {                                                                         \
    MSG_SPRINTF_6( MSG_SSID_DS_ACLPOLICY, level, str, arg1, arg2, arg3,     \
                   arg4, arg5, arg6);                                       \
  }

#endif /* PS_ACLI_TRACE_H */

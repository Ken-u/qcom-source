#ifndef PS_POLICYI_MGR_H
#define PS_POLICYI_MGR_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                       P S _ P O L I C Y _ M G R . H


GENERAL DESCRIPTION
  This file defines the API exported by policy manager module

EXTERNAL FUNCTIONS
  PS_POLICY_MGR_INIT()
    Initializes policy mgr module

  PS_POLICY_MGR_GET_PRIORITY()
    Returns priority of a given app

  PS_POLICY_MGR_GET_PROFILE()
    Calls registered DS function to retireve profile of an appilcation

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/inc/ps_policyi_mgr.h#1 $
  $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS

/*===========================================================================

                        INTERNAL FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_POLICY_MGR_INIT()

DESCRIPTION
  This function is called at power up to initialize the 
  policy information database.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_policy_mgr_init
(
  void
);

/*===========================================================================
FUNCTION PS_POLICY_MGR_GET_PRIORITY()

DESCRIPTION
  Fetches priority given an app_identifier/profile_id.

PARAMETERS
  app_identifier : App whose priority is needed
  profile_id     : profile of the requesting App

RETURN VALUE
  priority                       : if inputs are valid
  PS_POLICY_MGR_PRIORITY_INVALID : otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_policy_mgr_get_priority
(
  uint64 app_identifier,
  int32  profile_id
);

/*===========================================================================
FUNCTION PS_POLICY_MGR_GET_PROFILE()

DESCRIPTION
  Calls the registered callback from DS to get application profile based on
  application identifier.

PARAMETERS
  app_identifier : App identifier value

RETURN VALUE
  (int32)profile id on SUCCESS 
  PROFILE_INVALID on FAILURE

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_policy_mgr_get_profile
(
  int64  app_identifier
);

#endif /* FEATURE_DATA_PS */
#endif /* PS_POLICY_MGR_H */

#ifndef PS_PLATFORM_TIMER_H
#define PS_PLATFORM_TIMER_H

/*==========================================================================*/
/*!
  @file
  ps_platform_timer.h

  @brief
  This file provides platform timer APIs that can be implemented by
  different platforms (Rex/WM/Linux etc)

  Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/utils/main/latest/src/ps_platform_timer.h#1 $
  $DateTime: 2010/01/14 15:44:13 $$Author: hmurari $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-12-03 hm  Created module.

===========================================================================*/
/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "err.h"

#define REX_ENABLE_PREMPTION()
#define REX_DISABLE_PREMPTION()


/*===========================================================================

                        PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_DEF()

DESCRIPTION
  This function takes a timer structure, callback pointer, and timer handle
  from the common layer.
  Defines/Initializes timer in platform specific layer.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  Timer buffer allocated from dynamic memory
===========================================================================*/
void ps_platform_timer_def
(
  void **      timer_ptr_ptr,
  void *       ps_timer_handler_cb,
  void *       timer_handle
);

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_SET()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Calls platform specific timer set API.  The void pointer is casted
  to platform specific timer structure in platform layer.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  NOne
===========================================================================*/
void ps_platform_timer_set
(
  void *        timer_ptr,
  uint32        timer_val
);

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_ACTIVE()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Returns true if timer is still running, returns false otherwise.

DEPENDENCIES
  None

RETURN VALUE
  True: if timer is running
  False: if timer is stopped

SIDE EFFECTS
  NOne
===========================================================================*/
int ps_platform_timer_is_active
(
  void *        timer_ptr
);

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_CLR()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Stops specified timer and remove it from the active timer list.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  NOne
===========================================================================*/
void ps_platform_timer_clr
(
  void *        timer_ptr
);

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_GET_MAX_SUPPORTED_TIMER()

DESCRIPTION
  This function retrieves the maximum time supported by the platform timer

DEPENDENCIES
  None

RETURN VALUE
  Maximum time supported by the platform timer

SIDE EFFECTS
  NOne
===========================================================================*/
uint32 ps_platform_timer_get_max_supported_timer
(
  void
);

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_GET()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Checks the timer to determine how much time is left.

DEPENDENCIES
  None

RETURN VALUE
  Time left in the specified timer.

SIDE EFFECTS
  NOne
===========================================================================*/
int64 ps_platform_timer_get
(
  void *        timer_ptr
);

/*===========================================================================
FUNCTION PS_PLATFORM_TIMER_FREE()

DESCRIPTION
  This function takes a timer structure from the common layer.
  Timer structure will be deallocated.

DEPENDENCIES
  None

PARAMETERS
  timer: timer structure holding parameters required for platform timer APIs

RETURN VALUE
  None

SIDE EFFECTS
  Timer buffer released to dynamic memory
===========================================================================*/
void ps_platform_timer_free
(
  void **        timer_ptr_ptr
);

#endif /* PS_PLATFORM_TIMER_H */


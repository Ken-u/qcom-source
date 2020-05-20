#ifndef SLIP_IFACE_EXT_H
#define SLIP_IFACE_EXT_H
/*===========================================================================

              S L I P _ I F A C E _ E X T . H

DESCRIPTION
   This is SLIP iface handler external header file. It contains the
   external function declarations.


Copyright (c) 2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/interface/api/rel/11.03/slip_iface_ext.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"

/*===========================================================================
  List of commands external modules post to DCC task for SLIP processing
  - SLIP_IFACE_CALLP_UP_CMD - CallP UP command
===========================================================================*/
#define SLIP_IFACE_CALLP_UP_CMD 1

/*===========================================================================

FUNCTION SLIP_IFACE_POST_CMD

DESCRIPTION
  This function posts corresponding SLIP command to DCC task for processing.
  
DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  System asserts if command posting fails.

===========================================================================*/
void slip_iface_post_cmd
( 
  uint32 cmd
);

#endif /* SLIP_IFACE_EXT_H */

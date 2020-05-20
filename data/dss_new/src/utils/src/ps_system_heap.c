/*===========================================================================

                       P S _ S Y S T E M _ H E A P . C

DESCRIPTION
  The Data Services Protocol Stack system heap wrapper.

EXTERNALIZED FUNCTIONS

  ps_system_heap_mem_alloc()
    Allocates requested amount of memory from the system heap.

  PS_SYSTEM_HEAP_MEM_FREE()
    Frees the memory allocated through ps_system_heap_mem_alloc().

INTIALIZATION AND SEQUENCING REQUIREMENTS
  None.

Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_system_heap.c#1 $
  $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/23/10    ss     Cleaning up of Globals for thread safety in Q6 Free 
                   Floating environment.
===========================================================================*/

/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#include "ps_system_heap.h"
#include "ds_Utils_DebugMsg.h"

#ifdef TEST_FRAMEWORK
static boolean simulate_out_of_mem = FALSE;
static uint32  alloc_buf_cnt;
#endif

#ifdef FEATURE_DATA_PS_SYSTEM_HEAP

/* Include the modem memory manager API */
#include "modem_mem.h"

#else

/*! 
 * Defines the PS system heap used in the absense of modem heap.
 *
 * The value of this constant is currently configured for AMSS environments.
 * For other environments, we have a limited set of VUs and functionality
 * exported and this needs to be tuned accordingly.
 */

#include "memheap.h"
 
#define PS_SYSTEM_HEAP_SIZE (500 * 1024)

static mem_heap_type ps_system_heap;
static uint8 ps_system_heap_buffer[PS_SYSTEM_HEAP_SIZE];

#endif 

/*===========================================================================

                         EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/
   
/*===========================================================================
FUNCTION  ps_system_heap_mem_alloc

DESCRIPTION
  This function is a wrapper on top of the system's malloc function.

PARAMETERS
  num_bytes - Size (in bytes) of the memory to be allocated.

RETURN VALUE
  Pointer to memory block if successful.
  NULL if memory could not be allocated.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void* ps_system_heap_mem_alloc
(
  unsigned long num_bytes
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef TEST_FRAMEWORK
  if (TRUE == simulate_out_of_mem)
  {
    return NULL;
  }
  alloc_buf_cnt++;
#endif

#ifdef FEATURE_DATA_PS_SYSTEM_HEAP

  return modem_mem_alloc(num_bytes, MODEM_MEM_CLIENT_DATACOMMON);

#else

  return mem_malloc( &ps_system_heap, (size_t) num_bytes );

#endif
} /* ps_system_heap_mem_alloc() */

/*===========================================================================
FUNCTION  ps_system_heapi_mem_free

DESCRIPTION
  Frees the memory allocated by ps_system_heap_mem_alloc(). Internal method, 
  Use only exported free method: PS_SYSTEM_HEAP_MEM_FREE

PARAMTERS
  mem_ptr   - Memory to free.

RETURN VALUE
  None.

DEPENDENCIES
  The memory chunk passed to be freed must have been allocated by
  ps_system_heap_mem_alloc().

SIDE EFFECTS
  Sets the passed in pointer to memory to NULL.
===========================================================================*/
void ps_system_heapi_mem_free
(
  void *mem_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == mem_ptr)
  {
    return;
  }

#ifdef TEST_FRAMEWORK
  alloc_buf_cnt--;
#endif

#ifdef FEATURE_DATA_PS_SYSTEM_HEAP

  modem_mem_free (mem_ptr, MODEM_MEM_CLIENT_DATACOMMON);

#else

  mem_free (&ps_system_heap, mem_ptr);

#endif
  
  return;

} /* ps_system_heapi_mem_free() */

/*===========================================================================
FUNCTION  ps_system_heap_init

DESCRIPTION
  Performs system heap initialization

PARAMTERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_system_heap_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifndef FEATURE_DATA_PS_SYSTEM_HEAP
  /*------------------------------------------------------------------------
    Initialize the PS system heap module.
  -------------------------------------------------------------------------*/
  mem_init_heap (&ps_system_heap,
                 ps_system_heap_buffer,
                 PS_SYSTEM_HEAP_SIZE,
                 NULL);
#endif

} /* ps_system_heap_mem_init */

/*===========================================================================
FUNCTION  ps_system_heap_deinit

DESCRIPTION
  Performs system heap cleanup

PARAMTERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_system_heap_deinit
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

} /* ps_system_heap_deinit */

/*===========================================================================
FUNCTION  ps_system_heap_simulate_out_of_mem

DESCRIPTION
  Simulates out of memory scenario for test purposes. Only to be used in 
  test scenarios, not in production builds.

PARAMTERS
  out_of_mem  - If TRUE  - then simulate out of memory.
                If FALSE - then reset simulate out of memory flag.

RETURN VALUE
  None.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
void ps_system_heap_simulate_out_of_mem
(
  boolean out_of_mem
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef TEST_FRAMEWORK

  simulate_out_of_mem = out_of_mem;

#else

  LOG_MSG_FATAL_ERROR ("Not supported!", 0, 0, 0);
  ASSERT(0);

#endif /* TEST_FRAMEWORK */

} /* ps_system_heap_simulate_out_of_mem */


/*===========================================================================
FUNCTION  ps_system_heap_get_alloc_buf_count

DESCRIPTION
  Returns the number of allocated buffers by the PS_SYSTEM_HEAP. Used on
  test environments to ensure no memory leak.

PARAMTERS
  None.

RETURN VALUE
  Count of currently allocated buffers from ps_system_heap.

DEPENDENCIES
  None.

SIDE EFFECTS
  None.
===========================================================================*/
uint32 ps_system_heap_get_alloc_buf_count
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef TEST_FRAMEWORK

  return alloc_buf_cnt; 

#else

  LOG_MSG_FATAL_ERROR ("Not supported!", 0, 0, 0);
  ASSERT(0);

#endif /* TEST_FRAMEWORK */

} /* ps_system_heap_get_alloc_buf_count */



#ifndef IP_FILTER_REG_H
#define IP_FILTER_REG_H

/*==========================================================================*/
/*!
  @file
  ds_Net_IPFilterReg.h

  @brief
  This file exports ds::Net::IPFilterReg class which implements IIPFilterRegPriv
  interface.

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_IPFilterReg.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-10-10 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Net_IIPFilterRegPriv.h"
#include "ds_Net_Handle.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_Utils.h"

namespace ds
{
namespace Net
{

/*lint -esym(1510, IQI) */
/*lint -esym(1510, IIPFilterRegPriv) */

/*!
  @class
  IPFilterReg

  @details
  The IPFilterReg class is an provides implementation for the IIPFilterRegPriv
  interface. This interface is an empty interface designed to register for
  a single event ds::Net::IPFilter::Event::UPDATED. An IPFilterReg object
  is created when the request is sent for registering for IPFilter events.
  When the object is deleted, the filters are automatically de-registered.

*/
class IPFilterReg : public IIPFilterRegPriv,
                    public Handle
{
private:
  int32             mFltrHandle;
  ISignalBus *      mpSigBusStateChange;

  /*!
  @brief
  Destructor of the IPFilterReg object.

  @details
  Destructor of the IPFilterReg object. The destructor is private and
  hence we cannot call "delete pIpFilterReg" etc. Use Release() method
  to release the pointer to this object.

  @param      None.
  @return     None.
  @see        IQI::Release()
  */
  virtual void Destructor
  (
    void 
  )
  throw();
  
  /*!
  @brief
  Dummy destructor. Only used to free memory. 
  
  @details
  For objects that support weak references, destruction of the object and
  freeing up of memory can happen at different times. Hence Destructor()
  is the one which performs actual destruction of the object, where as 
  ~Object() would perform memory cleanup. 
  @params     None.
  @return     None.
  */
  virtual ~IPFilterReg
  (
    void
  )
  throw();

public:

  /*!
  @brief
  Constructor.

  @details
  Constructor of the IPFilterReg object. IPFilterReg object provides 
  implementation for IDSIPFilterReg interface. It is used only for 
  registering for ds::Net::IPFilter::Event::UPDATED event. 

  @param[in]  ifaceHandle - Iface Handle.
  @param[in]  fltrHandle - Handle obtained after filters are installed.

  @return     Object created.
  */
  IPFilterReg
  (
    int32 ifaceHandle,
    int32 fltrHandle
  );

  virtual ds::ErrorType CDECL OnStateChange 
  (
    ISignal*                                signalObj,
    ds::Net::EventType                      eventID,
    IQI**                                   regObj
  )
  {
    return Handle::OnStateChange(signalObj, eventID, regObj);
  }

  virtual ds::ErrorType GetSignalBus
  (
    ds::Net::EventType  eventID,
    ISignalBus **       ppISigBus
  );

  /*-------------------------------------------------------------------------
    Macro to define IWeakRef/IQI methods
  -------------------------------------------------------------------------*/
  DS_UTILS_IWEAKREF_IMPL_DEFAULTS2(IIPFilterRegPriv)

  /*-------------------------------------------------------------------------
    Methods to overload new/delete operators.
  -------------------------------------------------------------------------*/
  DSNET_OVERLOAD_OPERATORS (PS_MEM_DS_NET_IP_FILTER_REG)

}; /* class IPFilterSpec */

} /* namespace Net */
} /* namespace ds */


#endif /* IP_FILTER_REG_H */

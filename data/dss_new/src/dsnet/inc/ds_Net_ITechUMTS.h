#ifndef DS_NET_ITECHUMTS_H
#define DS_NET_ITECHUMTS_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IPolicy.h"

/**
  * ds Technology UMTS interface.
  * Instantiation of this interface shall create an object that
  * can be used to register to MTPD events.
  */
#define ds_Net_AEEIID_ITechUMTS 0x107d26e

/** @interface ds_Net_ITechUMTS
  * 
  * ds ITechUMTS interface.
  * This interface is used to register for Mobile Terminated Packet Data
  * events. Instantiation of this interface is done via INetworkFactory.
  * @See INetworkFactory_CreateTechUMTS
  *
  * To Register for MTPD events:
  * Use RegMTPD() method of this interface. Via RegMTPD() the client
  * also provides a signal object that would be set to notify the 
  * application on a requested MTPD call.
  *
  * Once the application is notified of an MTPD call it should create an
  * ACTIVE mode INetwork object specifying the same policy used in RegMTPD
  * for registering to the MTPD events (see note below on address family
  * of the policy). If this step is not carried out as soon as the
  * application is notified of the MTPD call, the call shall not be
  * established.
  * @See INetworkFactory_CreateNetwork
  * The application should use different signals for different MTPD
  * registrations. This shall allow the application to identify the
  * applicable policy for creating the INetwork object when it receives
  * an MTPD call notification.
  *
  * To transfer data over the MTPD call, the application should create
  * socket based on the ACTIVE INetwork object or on the same policy used
  * for registering to the MTPD events.
  * @See ISocketFactory_CreateSocket
  * @See ISocketFactory_CreateSocketByNetwork
  *
  * To cancel the registration the application should release the signal.
  * 
  */
#define INHERIT_ds_Net_ITechUMTS(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*RegMTPD)(iname* _pif, ds_Net_IPolicy* Policy, ISignal* signal, IQI** mtpdReg)
AEEINTERFACE_DEFINE(ds_Net_ITechUMTS);

/** @memberof ds_Net_ITechUMTS
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_ITechUMTS_AddRef(ds_Net_ITechUMTS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_ITechUMTS)->AddRef(_pif);
}

/** @memberof ds_Net_ITechUMTS
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_ITechUMTS_Release(ds_Net_ITechUMTS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_ITechUMTS)->Release(_pif);
}

/** @memberof ds_Net_ITechUMTS
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_ITechUMTS_QueryInterface(ds_Net_ITechUMTS* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_ITechUMTS)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_ITechUMTS
  * 
  * Register for MTPD notifications.
  * @param _pif Pointer to interface
  * @param Policy Requested Network Policy. MTPD events are address
  *               family agnostic, meaning the Network cannot specify
  *               that the MTPD event is IPv4 or IPv6. Therefor, the
  *               address family of the policy shall not effect the
  *               type of MTPD events the application shall receive
  *               (in respect to address family).
  * @param signal Signal that shall be set when MTPD notification occurs.
  *               To cancel the registration the application should
  *               release this signal.
  * @param mtpdReg The application must hold this output registration
  *                object to ensure its Signal stays registered. The
  *                application can release this object once it has
  *                released the signal object.
  * @retval AEE_SUCCESS NetType successfully set in the policy.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_ITechUMTS_RegMTPD(ds_Net_ITechUMTS* _pif, ds_Net_IPolicy* Policy, ISignal* signal, IQI** mtpdReg)
{
   return AEEGETPVTBL(_pif, ds_Net_ITechUMTS)->RegMTPD(_pif, Policy, signal, mtpdReg);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IPolicy.h"

/**
  * ds Technology UMTS interface.
  * Instantiation of this interface shall create an object that
  * can be used to register to MTPD events.
  */
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_ITechUMTS = 0x107d26e;
      
      /** @interface ITechUMTS
        * 
        * ds ITechUMTS interface.
        * This interface is used to register for Mobile Terminated Packet Data
        * events. Instantiation of this interface is done via INetworkFactory.
        * @See INetworkFactory::CreateTechUMTS
        *
        * To Register for MTPD events:
        * Use RegMTPD() method of this interface. Via RegMTPD() the client
        * also provides a signal object that would be set to notify the 
        * application on a requested MTPD call.
        *
        * Once the application is notified of an MTPD call it should create an
        * ACTIVE mode INetwork object specifying the same policy used in RegMTPD
        * for registering to the MTPD events (see note below on address family
        * of the policy). If this step is not carried out as soon as the
        * application is notified of the MTPD call, the call shall not be
        * established.
        * @See INetworkFactory::CreateNetwork
        * The application should use different signals for different MTPD
        * registrations. This shall allow the application to identify the
        * applicable policy for creating the INetwork object when it receives
        * an MTPD call notification.
        *
        * To transfer data over the MTPD call, the application should create
        * socket based on the ACTIVE INetwork object or on the same policy used
        * for registering to the MTPD events.
        * @See ISocketFactory::CreateSocket
        * @See ISocketFactory::CreateSocketByNetwork
        *
        * To cancel the registration the application should release the signal.
        * 
        */
      struct ITechUMTS : public ::IQI
      {
         
         /**
           * Register for MTPD notifications.
           * @param Policy Requested Network Policy. MTPD events are address
           *               family agnostic, meaning the Network cannot specify
           *               that the MTPD event is IPv4 or IPv6. Therefor, the
           *               address family of the policy shall not effect the
           *               type of MTPD events the application shall receive
           *               (in respect to address family).
           * @param signal Signal that shall be set when MTPD notification occurs.
           *               To cancel the registration the application should
           *               release this signal.
           * @param mtpdReg The application must hold this output registration
           *                object to ensure its Signal stays registered. The
           *                application can release this object once it has
           *                released the signal object.
           * @retval AEE_SUCCESS NetType successfully set in the policy.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RegMTPD(::ds::Net::IPolicy* Policy, ::ISignal* signal, ::IQI** mtpdReg) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_ITECHUMTS_H

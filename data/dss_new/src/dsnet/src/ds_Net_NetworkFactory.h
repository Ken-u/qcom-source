#ifndef DS_NET_NETWORK_FACTORY_H
#define DS_NET_NETWORK_FACTORY_H

/*=========================================================================*/
/*!
  @file
  NetworkFactory.h

  @brief
  This file provides implementation for ds::Net::NetworkFactory class.
  NetworkFactory exports methods for creation of various network objects.

  @details
  NetworkFactory implements the interfaces - INetworkFactory and
  INetworkFactoryPriv. Implementation is done via proxy class
  NetworkFactoryClient. These interfaces provide methods for creation of
  various objects for DSNET module:

  CreateNetwork()
    Creates a network object using application specified policy.
    if policy NULL, default policy shall be used.

  CreateIPFilterSpec()
    Creates an IPFilterSpec object (implements ds::Net::IIPFilterSpec)

  CreateQoSFlowSpec()
    Creates a QoSFlowSpec object (implements ds::Net::IQoSFlowSpec)

  CreatePolicy()
    Creates a Policy object (implements ds::Net::IPolicy)

  CreateNetworkPriv()
    Creates a privileged network object using application specified policy.
    (implements ds::Net::INetworkPriv interface)

  CreatePolicyPriv()
    Creates a privileged policy object. (implements ds::Net::IPolicyPriv)

  CreateNatSessionPriv()
    Creates a NAT session priv object (implements DS::Net::INatSessionPriv)

  Copyright (c) 2008-2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkFactory.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_CSSupport.h"
#include "AEEICritSect.h"
#include "ds_Net_Network.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_Utils.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_Policy.h"
#include "ds_Net_QoSFlowSpec.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_INetworkFactory.h"
#include "ds_Net_NatSession.h"

/*===========================================================================
                     FORWARD DECLERATION
===========================================================================*/
struct IPrivSet;

/*===========================================================================

                     PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
  namespace Net
  {

    /*lint -esym(1510, IQI) */

    /*!
    @class
    NetworkFactory

    @brief
    Implements INetworkFactory and INetworkFactoryPriv interfaces.

    @details
    Used to creates various objects in the DSNET module.
    Implementation is done via proxy class NetworkFactoryClient

    @see
    ds_Net_NetworkFactory.h documentation
    */
    class NetworkFactory
    {
    private:

      /*-------------------------------------------------------------------------
      Private member variables.
      -------------------------------------------------------------------------*/
      ICritSect*                    mpICritSect;
      static NetworkFactory*        instance;

      /*-------------------------------------------------------------------------
      Private methods: constructor/destructors
      -------------------------------------------------------------------------*/
      /*!
      @brief
      Constructor of the ds::Net::NetworkFactory object.

      @details
      NetworkFactory is a singleton. It is created during powerup of the phone/
      library and is deleted during the powerdown (in case of library).

      @param      None.
      @see        ds::Net::NetworkFactory::Init()
      @return     Pointer to the NetworkFactory object.
      */
      NetworkFactory
        (
        void
        )
        throw();

      /*!
      @brief
      Destructor of the ds::Net::NetworkFactory object.

      @details
      NetworkFactory is deleted when the library is unloaded (powerdown).

      @param      None.
      @see        ds::Net::NetworkFactory::DeInit()
      @return     None.
      */
      virtual ~NetworkFactory
        (
        void
        )
        throw();

      /*-------------------------------------------------------------------------
      Methods to overload new/delete operators.
      -------------------------------------------------------------------------*/
      DSNET_OVERLOAD_OPERATORS (PS_MEM_DS_NET_NETWORK_FACTORY)

    public:
      /*!
      @brief
      Returns the singleton instance of the network factory.

      @details
      Instance is a static method used to get an instance of the
      singleton NetworkFactory object. Since the network factory is created
      during powerup and deleted during powerdown, this function can be safely
      called anytime and can be expected to return the correct instance.

      This method does not perform ref-count management. If callers of this
      routine need to store the returned pointer, they would have to perform
      AddRef()/Release() themselves.

      @param      None.
      @return     Pointer to the NetworkFactory object.
      */
      static NetworkFactory* Instance
        (
        void
        )
        throw();

      /*-------------------------------------------------------------------------
      Inherited functions from INetworkFactory
      -------------------------------------------------------------------------*/
      /*!
      @function
      CreateNetwork()

      @brief      
      This function creates network object based on the network policy object 
      passed in, or default network in case of NULL policy.

      @details
      This function is used to create a network object using the default 
      IPolicy object in case of NULL policy, or allows applications to specify a
      network policy instead of just using default policy. Network objects in regular
      or monitored mode can be created using this method. 

      @param[in]  networkMode - ACTIVE/MONITORED
      @param[in]  pIPolicy    - IPolicy object specifying the policy,
                                if NULL default policy shall be used.
      @param[out] ppINetwork  - Returned INetwork object.
      @param[in]  privSetPtr  - Privileges of the client

      @see        ds::Net::Network::CreateDefaultNetwork()
      @see        ds::Net::NetworkModeType

      @return     SUCCESS if the network object can be created successfully.
      @return     QDS_EFAULT - Invalid arguments.
      @return     DSS_ENOMEM - Out of memory.
      */
      virtual ds::ErrorType CDECL CreateNetwork
        (
        NetworkModeType     networkMode,
        IPolicy*            pIPolicy,
        INetwork**          ppINetwork,
        IPrivSet*           privSetPtr
        );

      /*!
      @brief
      Creates instance of IPFilterSpec class.

      @details
      This function creates an instance of IPFilterSpec. IPFilterSpec
      creation is supported only via INetworkFactory.

      @param[out]  ppIIPFilterSpec - Output IPFilterSpec instance.
      @retval      ds::SUCCESS IPFilterSpec created successfully.
      @retval      Other DS designated error codes might be returned.
      */
      virtual ds::ErrorType CDECL CreateIPFilterSpec
        (
        IIPFilterPriv**         ppIIPFilterSpec
        );

      /*!
      @brief
      Creates instance of QoSFlowSpec class.

      @details
      This function creates an instance of QoSFlowSpec. QoSFlowSpec
      creation is supported only via INetworkFactory.

      @param[out]  ppIQoSSpec - Output interface.
      @retval      ds::SUCCESS Object created successfully.
      @retval      Other DS designated error codes might be returned.
      */
      virtual ds::ErrorType CDECL CreateQoSFlowSpec
        (
        IQoSFlowPriv**          ppIQoSSpec
        );

      /*!
      @brief
      Creates instance of Policy object.

      @details
      This function creates an instance of Policy. Policy object
      creation is supported only via INetworkFactory.

      @param[out]  ppIPolicy - Output interface.
      @retval      ds::SUCCESS Object created successfully.
      @retval      Other DS designated error codes might be returned.
      */
      virtual ds::ErrorType CDECL CreatePolicy
        (
        IPolicy**           ppIPolicy,
        IPrivSet*           privSetPtr
        );

      /*!
      @brief
      This function creates an instance of ITechUMTS object.

      @details
      This function creates an instance of ITechUMTS. ITechUMTS
      object creation is supported only via INetworkFactory.

      @param[out]  newTechUMTS - Output interface.
      @retval      AEE_SUCCESS Object created successfully.
      @retval      Other DS designated error codes might be returned.
      */
      virtual ds::ErrorType CDECL CreateTechUMTS
        (
        ITechUMTS** newTechUMTS,
        IPrivSet *           privSetPtr
        );

      /*-------------------------------------------------------------------------
      Inherited functions from INetworkFactoryPriv
      -------------------------------------------------------------------------*/
      /*!
      @brief
      Creates instance of INetworkPriv object.

      @details
      This function creates an instance of INetworkPriv. INetworkPriv
      object creation is supported only via INetworkFactory.

      @param[in]   pIPolicy - Policy object to be associated with INetwork.
                              If NULL then the default policy will be used.
      @param[out]  ppIPolicy - Output interface.
      @param[in]  privSetPtr - Privileges of the client

      @retval      ds::SUCCESS Object created successfully.
      @retval      Other DS designated error codes might be returned.
      */
      virtual ds::ErrorType CDECL CreateNetworkPriv
        (
        IPolicyPriv*        pIPolicy,
        INetworkPriv**      ppIPolicy,
        IPrivSet*           privSetPtr
        );

      /*!
      @brief
      Creates instance of IPolicyPriv object.

      @details
      This function creates an instance of IPolicyPriv. IPolicyPriv
      object creation is supported only via INetworkFactory.

      @param[out]  ppIPolicy - Output interface.
      @retval      ds::SUCCESS Object created successfully.
      @retval      Other DS designated error codes might be returned.
      */
      virtual ds::ErrorType CDECL CreatePolicyPriv
        (
        IPolicyPriv**       ppIPolicy
        );

      /*!
      @brief      
      Creates an instance of INatSessionPriv interface.

      @param[in]  pIPolicy - Policy object to be used for NAT Priv session.
      @param[out] ppINatSessionPriv - Output interface.

      @return     DS::SUCCESS - on success
      @return     DSS_EFAULT - Invalid arguments
      @return     DSS_ENOMEM - No memory to create NAT session.

      @see        Interface: INatSessionPriv.
      */
      virtual ds::ErrorType CDECL CreateNatSessionPriv
      (
        ds::Net::IPolicyPriv          *pIPolicy,
        ds::Net::INatSessionPriv     **ppINatSessionPriv
      );

      /**
      @brief IQI interface Methods
      */
      DSIQI_IMPL_DEFAULTS_NO_QI();

    };/* class NetworkFactory */
  } /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_NETWORK_FACTORY_H */


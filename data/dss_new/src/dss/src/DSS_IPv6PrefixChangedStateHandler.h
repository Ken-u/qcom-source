#ifndef __DSS_IPV6PREFIXCHANGEDSTATEHANDLER_H__
#define __DSS_IPV6PREFIXCHANGEDSTATEHANDLER_H__

/*====================================================

FILE:  DSS_IPv6PrefixChangedEventHandler.h

SERVICES:
   Handle IPV6 prefix change state events.

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IPv6PrefixChangedStateHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "DSS_EventHandlerNetApp.h"
#include "ds_Net_INetworkIPv6Priv.h"
#include "DSS_MemoryManagement.h"




class DSSIPv6PrefixChangedStateHandler : public DSSEventHandlerNetApp
{
   typedef enum {PREFIXES_EQUAL,PREFIXES_DIFFERENT,PREFIXES_STATE_CHANGED} IPV6PrefixCompareResult;

public:
   static DSSIPv6PrefixChangedStateHandler* CreateInstance();
   virtual void Destructor() throw();
   DSSIPv6PrefixChangedStateHandler();

protected:
   virtual void EventOccurred();
   virtual AEEResult RegisterIDL();
   virtual AEEResult SetEventData(dss_iface_ioctl_event_enum_type event,
                                  bool bReg,
                                  dss_iface_ioctl_event_cb userCB,
                                  void* userData);

private:
   AEEResult GetNetworkIPv6Priv(ds::Net::INetworkIPv6Priv** ppiNetworkIPv6Priv);

   /* prefixes, num - are output params, their values do not matter, can be NULL and 0 */
   AEEResult GetAllPrefixes(ds::Net::INetworkIPv6Priv* piNetworkIPv6Priv,
                            ds::Net::IPv6PrivPrefixInfoType** prefixes,
                            int *num);

   IPV6PrefixCompareResult ComparePrefixes(ds::Net::IPv6PrivPrefixInfoType* source,
                                           ds::Net::IPv6PrivPrefixInfoType* dest);
   void ProcessCB(ds::Net::IPv6PrivPrefixInfoType* event, dss_iface_ioctl_prefix_update_enum_type prefix);

   struct DSSIPv6PrefixHndlData : public HandlerData{
      ds::Net::IPv6PrivPrefixInfoType* mPrevPrefixes;
      int mNumPrevPrefixes;
   };

};

#endif // __DSS_IPV6PREFIXCHANGEDSTATEHANDLER_H__

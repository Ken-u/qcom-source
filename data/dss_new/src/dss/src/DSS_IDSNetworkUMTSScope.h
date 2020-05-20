#ifndef __DSS_IDSNETWORKUMTSSCOPE_H__
#define __DSS_IDSNETWORKUMTSSCOPE_H__

/*===================================================

FILE:  DSS_IDSNetworkUMTSScope.h

SERVICES:
   A utility class to facilitate IDSNetworkUMTS fetching
   and releasing.

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IDSNetworkUMTSScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/


#include "DSS_Common.h"

#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_INetworkUMTS.h"


// This class provides an abstraction over IDSNetworkUMTS pointer.
// Define and initialize (Init) an instance of this class at the top of the scope.
// If init is successful, the IDSNetworkUMTS pointer is valid and can be used
// until the end of the scope where the IDSNetworkUMTS object's reference counting
// decremented.
class DSSIDSNetworkUMTSScope {
public:
   DSSIDSNetworkUMTSScope();
   ~DSSIDSNetworkUMTSScope();
   AEEResult Init(ds::Net::INetwork* pIDSNetwork);
   ds::Net::INetworkUMTS* Fetch();
   
private:
   ds::Net::INetworkUMTS* mpIDSNetworkUMTS;   
};

inline DSSIDSNetworkUMTSScope::DSSIDSNetworkUMTSScope() : mpIDSNetworkUMTS(NULL)
{
}

inline AEEResult DSSIDSNetworkUMTSScope::Init(ds::Net::INetwork* pIDSNetwork)
{   
   if (NULL == pIDSNetwork) {
      return QDS_EFAULT;
   }
   
   AEEResult res = pIDSNetwork->GetTechObject(ds::Net::AEEIID_INetworkUMTS, (void**)&mpIDSNetworkUMTS);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Failed to fetch INetworkUMTS object: %d", res, 0, 0);
      return res;
   }

   return AEE_SUCCESS;
}

inline ds::Net::INetworkUMTS* DSSIDSNetworkUMTSScope::Fetch()
{   
   return mpIDSNetworkUMTS;
}

/*lint -e{1551} */
inline DSSIDSNetworkUMTSScope::~DSSIDSNetworkUMTSScope()
{
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetworkUMTS);
}
/*lint �restore */

#endif // __DSS_IDSNETWORKUMTSSCOPE_H__

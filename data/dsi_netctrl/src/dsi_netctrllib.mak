# -*- Mode: text -*-
#==============================================================================
# FILE: dsi_netctrllib.mak
#
# SERVICE: 
#
# DESCRIPTION:
#
# Copyright 2012 Qualcomm Technologies, Inc. All rights reserved.
#==============================================================================

default: all

RELPATHTOROOT=../../../../../..

TARGET = libdsi_netctrl
MAKEFILETYPE = shared
MAKEFILENAME = dsi_netctrllib.mak

C_SRC =\
   dsi_netctrl.c\
   dsi_netctrl_init.c\
   dsi_netctrli.c\
   dsi_netctrl_mni_cb.c\
   dsi_netctrl_mni.c\
   dsi_netctrl_multimodem.c\
   dsi_netctrl_netmgr.c\
   dsi_netctrl_platform.c\
   dsi_netctrl_qos.c\
   dsi_netctrl_cb_thrd.c\
   dsi_netctrl_embms.c\

NEEDINC=\
   vendor/qcom-source/data/dsi_netctrl/inc\
   vendor/qcom-source/data/qdi/inc\
   vendor/qcom-source/qmi/inc\
   vendor/qcom-source/qmi/platform\
   vendor/qcom-source/diag/include\
   vendor/qcom-source/data/dsutils/inc\
   vendor/qcom-source/data/netmgr/inc\
   vendor/qcom-source/common/inc\
   system/core/include\

NEEDLIB =\
   vendor/qcom-source/data/dsutils/src/libdsutils\
   vendor/qcom-source/data/netmgr/src/libnetmgr\
   vendor/qcom-source/data/qdi/src/libqdi\

CFLAGS += -m32
CFLAGS += -DFEATURE_DATA_LOG_STDERR
CFLAGS += -DDSI_NETCTRL_OFFTARGET
CFLAGS += -DFEATURE_DSI_MULTIMODEM_ROUTELOOKUP
CFLAGS += -DFEATURE_DSI_TEST
CFLAGS += -Dstrlcpy=strncpy -Dstrlcat=strncat
CFLAGS += -include string.h

LDFLAGS += -m32

BTBUILD=vendor/qcom-source/test/ril/btbuild
include $(RELPATHTOROOT)/$(BTBUILD)/bt.driver.mki

-include ./${MAKEFILENAME}.depend.mki

#======================================================================
# Add Mannual dependencies and makefile overrides here
#======================================================================

#EOF

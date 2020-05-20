# -*- Mode: text -*-
#==============================================================================
# FILE: qdilib.mak
#
# SERVICE: 
#
# DESCRIPTION:
#
# Copyright 2012 Qualcomm Technologies, Inc. All rights reserved.
#==============================================================================

default: all

RELPATHTOROOT=../../../../../..

TARGET = libqdi
MAKEFILETYPE = shared
MAKEFILENAME = qdilib.mak

C_SRC =\
   qdi.c\
   qdi_netlink.c\

NEEDINC=\
   vendor/qcom-source/data/qdi/inc\
   vendor/qcom-source/diag/include\
   vendor/qcom-source/qmi/inc\
   vendor/qcom-source/qmi/platform\
   vendor/qcom-source/data/dsutils/inc\
   vendor/qcom-source/data/dsi_netctrl/inc\
   vendor/qcom-source/common/inc\

NEEDLIB =\
   vendor/qcom-source/qmi/libqmi\
   vendor/qcom-source/diag/src/libdiag\
   vendor/qcom-source/data/dsutils/src/libdsutils\

CFLAGS += -m32
CFLAGS += -DDSI_NETCTRL_OFFTARGET
CFLAGS += -DFEATURE_DSI_MULTIMODEM_ROUTELOOKUP
CFLAGS += -DFEATURE_DSI_TEST
CFLAGS += -Dstrlcpy=strncpy -Dstrlcat=strncat
CFLAGS += -DQDI_OFFTARGET
CFLAGS += -DFEATURE_DATA_LOG_QXDM

LDFLAGS += -m32

BTBUILD=vendor/qcom-source/test/ril/btbuild
include $(RELPATHTOROOT)/$(BTBUILD)/bt.driver.mki

-include ./${MAKEFILENAME}.depend.mki

#======================================================================
# Add Mannual dependencies and makefile overrides here
#======================================================================

#EOF

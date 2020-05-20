# -*- Mode: text -*-
#==============================================================================
# FILE: dsutilslib.mak
#
# SERVICE: 
#
# DESCRIPTION:
#
# Copyright 2012 Qualcomm Technologies, Inc. All rights reserved.
#==============================================================================

default: all

RELPATHTOROOT=../../../../../..

TARGET = libdsutils
MAKEFILETYPE = shared
MAKEFILENAME = dsutilslib.mak

C_SRC =\
   ds_cmdq.c\
   ds_list.c\
   ds_util.c\
   ds_sl_list.c\
   queue.c\
   stm2.c

NEEDINC=\
   vendor/qcom-source/data/dsutils/inc\
   vendor/qcom-source/data/dss_new/src/platform/inc\
   vendor/qcom-source/diag/include\
   vendor/qcom-source/common/inc\
   system/core/include\
   frameworks/base/include\

NEEDLIB =\

CFLAGS += -m32
CFLAGS += -DFEATURE_DATA_LINUX  ## use pthreads locks for queue
CFLAGS += -DFEATURE_DATA_LOG_QXDM
CFLAGS += -DFEATURE_DS_LINUX_NO_RPC
CFLAGS += -DFEATURE_DS_LINUX_ANDROID
CFLAGS += -DFEATURE_Q_NO_SELF_QPTR

LDFLAGS += -m32

BTBUILD=vendor/qcom-source/test/ril/btbuild
include $(RELPATHTOROOT)/$(BTBUILD)/bt.driver.mki

-include ./${MAKEFILENAME}.depend.mki

#======================================================================
# Add Mannual dependencies and makefile overrides here
#======================================================================

#EOF

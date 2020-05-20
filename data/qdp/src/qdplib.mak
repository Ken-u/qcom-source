# -*- Mode: text -*-
#==============================================================================
# FILE: qdplib.mak
#
# SERVICE: 
#
# DESCRIPTION:
#
# Copyright 2012 Qualcomm Technologies, Inc. All rights reserved.
#==============================================================================

default: all

RELPATHTOROOT = ../../../../../..

TARGET = libqdp
MAKEFILETYPE = shared
MAKEFILENAME = qdplib.mak

C_SRC =\
   qdp.c\
   qdp_platform.c

NEEDINC=\
   vendor/qcom-source/data/qdp/inc\
   vendor/qcom-source/qmi/inc\
   vendor/qcom-source/qmi/platform\
   vendor/qcom-source/data/dsutils/inc\
   vendor/qcom-source/common/inc\
   vendor/qcom-source/diag/include\

NEEDLIB =\

CFLAGS += -m32
CFLAGS += -D_GNU_SOURCE # required for sighandler_t
CFLAGS += -DFEATURE_TARGET_GENERIC_x86

LDFLAGS += -m32

BTBUILD=vendor/qcom-source/test/ril/btbuild
include $(RELPATHTOROOT)/$(BTBUILD)/bt.driver.mki

-include ./${MAKEFILENAME}.depend.mki

#======================================================================
# Add Mannual dependencies and makefile overrides here
#======================================================================

#EOF

# -*- Mode: text -*-
#==============================================================================
# FILE: qmilib.mak
#
# SERVICE: 
#
# DESCRIPTION:
#
# Copyright 2012 Qualcomm Technologies, Inc. All rights reserved.
#==============================================================================

default: all

RELPATHTOROOT = ../../../..

TARGET = libqmi
MAKEFILETYPE = shared
MAKEFILENAME = qmilib.mak

C_SRC =\
   src/qmi.c\
   src/qmi_atcop_srvc.c\
   src/qmi_cat_srvc.c\
   src/qmi_eap_srvc.c\
   src/qmi_nas_srvc.c\
   src/qmi_qos_srvc.c\
   src/qmi_service.c\
   src/qmi_uim_srvc.c\
   src/qmi_wds_srvc.c\
   src/qmi_wds_utils.c\
   src/qmi_errors.c\
   src/qmi_client.c ## realy is qcci_legacy, but just put here for now

NEEDINC=\
   vendor/qcom-source/qmi/inc\
   vendor/qcom-source/qmi/src\
   vendor/qcom-source/qmi/platform\
   vendor/qcom-source/qmi/proxy\
   vendor/qcom-source/qmi/core/lib/inc\
   vendor/qcom-source/qmi/services\
   vendor/qcom-source/common/inc\
   vendor/qcom-source/data/dsutils/inc\
   vendor/qcom-source/diag/include\

NEEDLIB =\
   vendor/qcom-source/qmi/services/libqmiservices\
   vendor/qcom-source/qmi/qmi_client_qmux/libqmi_client_qmux\

CFLAGS += -m32
CFLAGS += -DFEATURE_DATA_LOG_QXDM
CFLAGS += -DFEATURE_RILTEST
CFLAGS += -include string.h

CFLAGS += -DFEATURE_QMI_ANDROID

LDFLAGS += -m32

BTBUILD=vendor/qcom-source/test/ril/btbuild
include $(RELPATHTOROOT)/$(BTBUILD)/bt.driver.mki

-include ./${MAKEFILENAME}.depend.mki

#======================================================================
# Add Mannual dependencies and makefile overrides here
#======================================================================

#EOF

# Copyright 2025 Terry Golubiewski, all rights reserved.

export PROJNAME:=tjg32
export PROJDIR := $(abspath .)
# SWDEV := $(PROJDIR)/SwDev
include $(SWDEV)/project.mk

APP:=$(abspath $(HOME)/App)
BOOST:=$(APP)/boost
MP11:=$(APP)/mp11

# Must use "=" instead of ":=" because $E will be defined below.
CKSUM_E=cksum.$E

TARGET1=$(CKSUM_E)
TARGETS=$(TARGET1)

SRC1:=main.c cksum.c crctab.c cksum_slice8.c
ifeq ($(COMPILER), gcc)
CDEFS+=-DUSE_PCLMUL_CRC32=1
SRC1+=cksum_pclmul.c
endif
ifeq ($(COMPILER), clang)
CDEFS+=-DUSE_VMULL_CRC32=1
SRC1+=cksum_vmull.c
endif

SOURCE:=$(SRC1)

#SYSINCL:=$(addsuffix /include, $(UNITS)/core $(UNITS)/systems $(GSL))
SYSINCL:=$(BOOST) $(addsuffix /include, $(MP11))
INCLUDE:=$(PROJDIR)

# Must use "=" because LIBS will be changed below.
LIBS=

#DEBUG=1

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

.ONESHELL:

.PHONY: all clean scour install uninstall

all: depend $(TARGETS)

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)

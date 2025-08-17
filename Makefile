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

SRC1:=main.cpp cksum.cpp crctab.cpp cksum_slice8.cpp
ifeq ($(COMPILER), gcc)
CDEFS+=-DUSE_PCLMUL_CRC32=1
SRC1+=cksum_pclmul.cpp
endif
ifeq ($(COMPILER), clang)
CDEFS+=-DUSE_VMULL_CRC32=1
SRC1+=cksum_vmull.cpp
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

.PHONY: all clean scour test

all: depend $(TARGETS)

test: all
	./cksum.exe bigfile.bin
	cat cksum.txt

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)

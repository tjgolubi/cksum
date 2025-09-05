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
CRCTIME_E=CrcTime.$E

TARGET1=$(CKSUM_E)
TARGET2=$(CRCTIME_E)
TARGETS=$(TARGET1) $(TARGET2)

SRC1:=main.cpp cksum.cpp CrcTab.cpp cksum_slice8.cpp
SRC2:=CrcTime.cpp cksum.cpp CrcTab.cpp CrcTab2.cpp cksum_slice8.cpp
SRC2+=cksum_slice8a.cpp
SRC2+=cksum_slice8b.cpp
ifeq ($(COMPILER), gcc)
#CDEFS+=-DUSE_PCLMUL_CRC32=1
#SRC1+=cksum_pclmul.cpp
endif
ifeq ($(COMPILER), clang)
CDEFS+=-DUSE_VMULL_CRC32=1
SRC1+=cksum_vmull.cpp
SRC2+=cksum_vmull.cpp
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

CLEAN+=cksum_core.txt cksum_tjg.txt

.ONESHELL:

.PHONY: all clean scour test

all: depend $(TARGETS)

FILES:=Int.hpp README.md cksum.txt CrcTab.cpp files.txt main.cpp tjg.bin tjg.txt bigfile.bin

cksum_core.txt: $(FILES)
	cksum $(FILES) > $@

cksum_tjg.txt: $(CKSUM_E) $(FILES)
	./$(CKSUM_E) $(FILES) > $@

test: all cksum_core.txt cksum_tjg.txt
	diff cksum_core.txt cksum_tjg.txt

test2: depend $(CRCTIME_E)
	./$(CRCTIME_E)

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)

$(TARGET2): $(OBJ2) $(LIBS)
        $(LINK)

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
SRC2:=CrcTime.cpp cksum.cpp CrcTab.cpp cksum_slice8.cpp
ifeq ($(COMPILER), gcc)
CDEFS+=-DUSE_PCLMUL_CRC32=1
SRC1+=cksum_pclmul.cpp
SRC2+=cksum_pclmul.cpp
SRC2+=cksum_pclmul0.cpp
endif
ifeq ($(COMPILER), clang)
CDEFS+=-DUSE_VMULL_CRC32=1
SRC1+=cksum_vmull.cpp
SRC2+=cksum_vmull.cpp
SRC2+=cksum_vmull0.cpp
endif

SOURCE:=$(SRC1) $(SRC2)

#SYSINCL:=$(addsuffix /include, $(UNITS)/core $(UNITS)/systems $(GSL))
SYSINCL:=$(BOOST) $(addsuffix /include, $(MP11))
INCLUDE:=$(PROJDIR)

# Must use "=" because LIBS will be changed below.
LIBS=

#DEBUG=1

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

CLEAN+=cksum_core.txt cksum_tjg.txt
SCOUR+=bigfile.bin cksum.txt tjg.txt tjg.bin

.ONESHELL:

.PHONY: all clean scour test

all: depend $(TARGETS)

FILES:=Int.hpp README.md cksum.txt CrcTab.cpp main.cpp tjg.bin tjg.txt bigfile.bin

bigfile.bin:
	dd if=/dev/urandom of=$@ bs=1G count=2 status=progress

cksum.txt: bigfile.bin
	"cksum" bigfile.bin > $@

tjg.txt:
	echo "123456789" > $@

tjg.bin:
	dd if=/dev/urandom of=$@ bs=256 count=1 status=progress

cksum_core.txt: $(FILES)
	cksum $(FILES) > $@

cksum_tjg.txt: $(CKSUM_E) $(FILES)
	./$(CKSUM_E) $(FILES) > $@

test: all cksum_core.txt cksum_tjg.txt
	diff -bc cksum_core.txt cksum_tjg.txt

test2: depend $(CRCTIME_E)
	./$(CRCTIME_E)

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)

$(TARGET2): $(OBJ2) $(LIBS)
        $(LINK)

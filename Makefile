# Copyright 2025 Terry Golubiewski, all rights reserved.

export PROJNAME:=tjg32
export PROJDIR := $(abspath ..)
# SWDEV := $(PROJDIR)/SwDev
include $(SWDEV)/project.mk

APP:=$(abspath $(HOME)/App)
BOOST:=$(APP)/boost
MP11:=$(APP)/mp11
CORE:=$(APP)/coreutils

TJG:=$(PROJDIR)/tjg
CRC:=$(PROJDIR)/crc

# Must use "=" instead of ":=" because $E will be defined below.
CKSUM_E=cksum.$E

TARGET1=$(CKSUM_E)
TARGETS=$(TARGET1)

SRC1:=main.c cksum.c crctab.c cksum_vmull.c
#SRC1:=main.c cksum.c crctab.c cksum_pclmul.c
SOURCE:=$(SRC1)

#SYSINCL:=$(addsuffix /include, $(UNITS)/core $(UNITS)/systems $(GSL))
SYSINCL:=$(BOOST) $(addsuffix /include, $(MP11))
INCLUDE:=$(PROJDIR) $(CORE)/lib $(CORE)/src

# Must use "=" because LIBS will be changed below.
LIBS=

#DEBUG=1

include $(SWDEV)/$(COMPILER).mk
include $(SWDEV)/build.mk

#$(PATH_OBJ)/mkcrctab.$O: cksum.c
#	@echo "Module: $<"
#	@$(RM) $@
#	@$(CC) $(CFLAGS) $(CDEFS) $(GCCINCL) -DCRCTAB -c -o$@ $<
#
#$(PATH_OBJ)/mkcrctab.$E: $(PATH_OBJ)/mkcrctab.$O
#        $(LINK)
#
#$(PATH_OBJ)/crctab.c: $(PATH_OBJ)/mkcrctab.$E
#	$(PATH_OBJ)/mkcrctab.$E > $@

.ONESHELL:

.PHONY: all clean scour install uninstall

all: depend $(TARGETS)

$(TARGET1): $(OBJ1) $(LIBS)
        $(LINK)

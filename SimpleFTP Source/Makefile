TITLE := SFTP (SimpleFTP)
VERSION := 1.00
TITLE_ID := LANT00001
CONTENT_ID := IV0000-LANT00001_00-LANTRANSFER00000
LIBS := -lc -lkernel -lc++ -lSceVideoOut -lSceSysmodule -lSceFreeType -lSceNet -lSceNetCtl -lScePad -lSceUserService
EXTRAFLAGS := -DGRAPHICS_USES_FONT
PACKAGE_FILES := eboot.bin sce_sys/param.sfo sce_sys/icon0.png assets/fonts/Gontserrat-Regular.ttf sce_module/libc.prx sce_module/libSceFios2.prx
ifneq ($(wildcard sce_sys/pic1.png),)
PACKAGE_FILES += sce_sys/pic1.png
endif
TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
PROJDIR := $(shell basename $(CURDIR))
COMMONDIR := $(TOOLCHAIN)/samples/_common
INTDIR := $(PROJDIR)/x64/Debug
CXX := clang++
LD := ld.lld
OBJS := $(INTDIR)/main.o $(INTDIR)/graphics.o
CXXFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c $(EXTRAFLAGS) -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -isystem $(TOOLCHAIN)/include/c++/v1
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

all: $(CONTENT_ID).pkg

sce_module/libc.prx: $(TOOLCHAIN)/samples/font/sce_module/libc.prx
	cp $< $@

sce_module/libSceFios2.prx: $(TOOLCHAIN)/samples/font/sce_module/libSceFios2.prx
	cp $< $@

$(INTDIR):
	mkdir -p $@

$(INTDIR)/main.o: lan_transfer/main.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/graphics.o: lan_transfer/graphics.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -Ilan_transfer -o $@ $<

eboot.bin: $(OBJS)
	ld.lld $(OBJS) -o $(INTDIR)/lan_transfer.elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/create-fself -in=$(INTDIR)/lan_transfer.elf -out=$(INTDIR)/lan_transfer.oelf --eboot "$@" --paid 0x3800000000000011

sce_sys/param.sfo:
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0

$(CONTENT_ID).pkg: eboot.bin sce_sys/param.sfo sce_module/libc.prx sce_module/libSceFios2.prx
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/create-gp4 -out pkg.gp4 --content-id=$(CONTENT_ID) --files "$(PACKAGE_FILES)"
	$(TOOLCHAIN)/bin/$(shell uname -s | tr A-Z a-z)/PkgTool.Core pkg_build pkg.gp4 .
	mv "$@" "SFTP (SimpleFTP).pkg"

clean:
	rm -rf $(INTDIR) eboot.bin pkg.gp4 $(CONTENT_ID).pkg "SFTP (SimpleFTP).pkg" sce_module/libc.prx sce_module/libSceFios2.prx

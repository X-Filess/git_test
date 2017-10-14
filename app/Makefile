##############################################################################
SUBDIRS=

ifeq ($(OS), ecos)
SRC += $(shell find . -iname "*.c"  | grep -v "./opensource/*" | grep -v "./linux_only/*" | grep -v "./module/app_curl.c")
SRC += signal_connect_cplus.cpp
SRC += bsp.cpp
SRC += ./linux_only/app_wifi.c
SRC += ./linux_only/app_network.c
SRC += ./linux_only/app_3g.c
endif

ifeq ($(OS), linux)
SRC += $(shell find . -iname "*.c")
SRC += $(shell find . -iname "*.cpp")
#SRC += $(shell find . -iname "*.c" | grep -v "./opensource/*")
endif



LIB = libapp.a
BIN = out.elf

CFLAGS += -I./include 
#CFLAGS += -I./module/cim

CFLAGS += -I./log/include

CFLAGS += -I./linux_only/include 
CFLAGS += -I./player/include

CFLAGS  += -I$(GXLIB_PATH)/include/bus
CFLAGS  += -I$(GXLIB_PATH)/include/bus/gui_core
CFLAGS  += -I$(GXLIB_PATH)/include/bus/service
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/player
CFLAGS  += -I$(GXLIB_PATH)/include/bus/module/ca
CFLAGS  += -I$(GXSRC_PATH)/include


default: del all
	
del:
	rm -rf out.elf
	
##############################################################################
-include  $(GXSRC_PATH)/scripts/inc.Makefile.conf.mak
##############################################################################

# Include library makefile auto generated
# Ref: scripts/factory_makefile_lib.sh
-include Makefile.a
-include Makefile_w.a

ifeq ($(OS), ecos)
 #LDFLAGS += -lui -lgxplayer -ldvbs -lgxfrontend -lgxav -lfreetype -ljpeg -lpng -ldb -lgxca -lenca -lxml -ljansson -lgxlowpower -lxml2 -lcjson -lcurl -lcyassl -lz -ltiff
 LDFLAGS += -lui -lgxplayer -ldvbs -lgxfrontend -lgxav -lfreetype -ljpeg -lpng -ldb -lgxca -lenca -lxml -ljansson -lgxlowpower -lz -ltiff -lcyassl -lmpeg2
endif

ifeq ($(OS), linux)
 LDFLAGS += -lui -lgxplayer -ldvbs -lfreetype -ljpeg -lpng -ldb -lgxca -lenca -lxml -lgxbox -ljansson -lxml2 -lcjson -lcurl -lcyassl -lstdc++
endif

print_ld:
	@echo "--------------------------------------LDFLAGS------"
	@echo $$LDFLAGS

#test: 
#	@echo making test begin...	
#	$(CC)  main.c  -o test \
#	$(CFLAGS)                       \
#	-L$(GXLIB_PATH)/lib -lservice -L./ -lapp $(LDFLAGS) -lui   

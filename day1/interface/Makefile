
CC   = arm-linux-gnueabihf-gcc
CPP = arm-linux-gnueabihf-g++
OPTS = -Wall -W -O3

DEFS = -DNSC200
DEFS += -DCN_VERSION
#DEFS += -DGW
#DEFS += -DWD100

ifeq (YES, ${ENCRYPT_XML})
	DEFS += -DENCRYPT_XML
endif

OPTS += $(DEFS)

INCLUDE = -I headers -I ../util/inc -I ../tinyxml -I ../Crypto/sm4 -I ../Crypto/aes/ -I../Crypto/openssl-1.1.1/include

LIBS = ../util/libipcfifoutil.a ../tinyxml/libtinyxml.a wireless/iwlib.so curl/libcurl.arm.so.4.3.0 ../Crypto/sm4/libsm4.a ../Crypto/aes/libaes.a -L../Crypto/openssl-1.1.1/lib

LDFLAGS = -lpthread -lm -lstdc++ -lssl -lcrypto

CPPOBJ = command/cmd_get.o command/cmd_list.o command/cmd_save.o command/cmd_set.o command/cmd_update.o command/command.o command/keywords.o src/HemConf.o src/NovConf.o src/TrimbleConf.o src/UnicoreConf.o src/main.o src/oemConf.o src/procd.o src/util.o xmlconf/xmlconf.o
COBJ = src/24cXX.o src/decrypt.o

TARGETNAME=interface

$(TARGETNAME): $(CPPOBJ) $(COBJ)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)
	if [ "$(outputdir)" != "" ]; then cp $@ ../$(outputdir)/; fi

$(CPPOBJ):%.o:%.cpp
	$(CC) $(OPTS) $(INCLUDE) -c $< -o $@

$(COBJ):%.o:%.c
	$(CC) $(OPTS) $(INCLUDE) -c $< -o $@

%.d:%.c
	$(CC) $(OPTS)  $(INCLUDE) -MF"$@" -MM -MT"$@" -MT"$(<:.c=.o)" "$<"

-include $(COBJ:.o=.d)

%.dd:%.cpp
	$(CC) $(OPTS)  $(INCLUDE) -MF"$@" -MM -MT"$@" -MT"$(<:.cpp=.o)" "$<"

-include $(CPPOBJ:.o=.dd)

clean:
	$(RM) $(TARGETNAME)
	find . -name "*.o"  | xargs $(RM)
	find . -name "*.d"  | xargs $(RM)
	find . -name "*.dd"  | xargs $(RM)

.PHONY: clean


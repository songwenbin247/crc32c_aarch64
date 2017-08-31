ifeq ($(SRCDIR),)
SRCDIR := .
endif

VPATH := $(SRCDIR)

ifneq ($(wildcard config-host.mak),)
all:
include config-host.mak
config-host-mak: configure
	@echo $@ is out-of-date, running configure
	@sed -n "/.*Configured with/s/[^:]*: //p" $@ | sh
else
config-host.mak:
ifneq ($(MAKECMDGOALS),clean)
	@echo "Running configure for you..."
	@./configure
endif
all:
include config-host.mak
endif

CFLAGS += -O3
OBJS = $(SOURCE:.c=.o)
-include $(OBJS:.o=.d)

CRC32C_OBJ = Michael_crc32c.o fio_crc32c.o test.o ceph_crc32c.o

crc_test: $(SOURCE:.c=.o)
	gcc $(LDFLAGS) $(CFLAGS) -o $@ $(SOURCE:.c=.o)

SOURCE := $(wildcard *.c)

$(SOURCE:.c=.o): SOURCE
	gcc -c $^  -o $@






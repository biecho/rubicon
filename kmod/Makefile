# kmod/Makefile  – corrected snippet
CC := gcc
KVER  := $(shell uname -r)
KDIR  := /lib/modules/$(KVER)/build
PWD   := $(shell pwd)

EXTRA_CFLAGS += -I/local/home/dimeyer/rubicon/include/uapi

# Does KDIR/Makefile exist?
KBUILD_OK := $(shell test -e $(KDIR)/Makefile && echo 1 || echo 0)

ifeq ($(KBUILD_OK),0)
$(error \
Kernel headers for $(KVER) not found. \
Please install BOTH packages           \
  linux-headers-$(KVER) and            \
  linux-headers-$(KVER)-generic)
endif

# SPDX-License-Identifier: GPL-3.0-only

obj-m := rubench.o
EXTRA_CFLAGS += -I$(PWD)/../include/uapi     # header path from earlier

PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

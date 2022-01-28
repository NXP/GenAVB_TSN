LLC ?= llc
CLANG ?= clang
FIRMWARE_DIR ?= $(PREFIX)/lib/firmware/genavb
LLVMLD ?= llvm-link
KERNELDIR ?= /home/user/dev/real-time-edge-linux
GENAVB_INCLUDE ?= ../../include

CXXFLAGS += -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wunused \
 -Wundef -Wold-style-definition -Wvla -Wshadow -Wdouble-promotion

CXXFLAGS+= -I$(KERNELDIR)/tools/lib -I$(GENAVB_INCLUDE) -I$(GENAVB_INCLUDE)/linux

GENAVB_XDP = genavb-xdp.bin
GENAVB_XDP_IR = genavb-xdp.bc
OBJS = genavb_xdp_main.bc
COV_OBJS = $(OBJS:.bc=.o)

$(GENAVB_XDP): $(GENAVB_XDP_IR)
	$(LLC) $^ -march=bpf -filetype=obj -o $@

$(GENAVB_XDP_IR): $(OBJS)
	$(LLVMLD) $^ -o $@

%.bc : %.c
	$(CLANG) -target arm64 -I../include $(CXXFLAGS) \
		 -O2 -emit-llvm -c $^ -o $@

install: $(GENAVB_XDP)
	mkdir -p $(FIRMWARE_DIR)
	cp $(GENAVB_XDP) $(FIRMWARE_DIR)

%.o: %.c
	$(CC) $(CXXFLAGS) -I../include -c $^ -o $@

coverity: $(COV_OBJS)

clean:
	rm -f *.bin
	rm -f *.bc

TARGET  = jailbreakd
OUTDIR ?= bin

CC      = xcrun -sdk iphoneos cc -arch arm64e -Iinclude -Ivoucher_swap/headers -Ivoucher_swap/voucher_swap -Ivoucher_swap/voucher_swap/kernel_call
LDID    = ldid2
CFLAGS  = -Wall -Wno-unused-variable -Wno-unused-function

.PHONY: all clean

all: $(OUTDIR)/$(TARGET)

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS += -DJAILBREAKDDEBUG
else
    CFLAGS += -O2
endif

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OUTDIR)/$(TARGET): voucher_swap/voucher_swap/*.c voucher_swap/voucher_swap/kernel_call/*c *.c *.m | $(OUTDIR)
	$(CC) -o $@ $^ -framework Foundation -framework IOKit $(CFLAGS)

	export LANG=C
	export LC_CTYPE=C
	export LC_ALL=C
	#sed -i "" 's/\/usr\/lib\/librocketbootstrap.dylib/\/var\/ulb\/librocketbootstrap.dylib/g' $@

	$(LDID) -SEnt.plist $@
	tar --disable-copyfile -cvf $(OUTDIR)/$(TARGET).tar -C $(OUTDIR) $(TARGET)
	rm $@

clean:
	rm -f $(OUTDIR)/$(TARGET)
	rm -f $(OUTDIR)/$(TARGET).tar

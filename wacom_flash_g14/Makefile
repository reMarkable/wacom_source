WACOMTOOLSRC = wac_flash.c wac_flash.h
PROGNAME = wacom_flash
STATIC_BUILD ?= n
ifeq ($(STATIC_BUILD),y)
	LDFLAGS += -static
endif

all: $(PROGNAME)
$(PROGNAME): $(WACOMTOOLSRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(WACOMTOOLSRC) -o $(PROGNAME)

clean:
	rm -f $(WACOMTOOLOBJ) $(PROGNAME)

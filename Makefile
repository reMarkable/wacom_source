WACOMTOOLSRC = read_hex.c wacom_flash_aes.c wacom_flash.c
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

install: $(PROGNAME)
	install -d $(DESTDIR)/usr/bin/
	install -m 755 wacom_flash $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/lib/firmware/
	install -m 644 firmware/W9013_0734.hex $(DESTDIR)/lib/firmware/

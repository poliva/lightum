DESTDIR?=/
SHELL = /bin/sh
CC = gcc -O
CFLAGS = -Wall
INSTALL = /usr/bin/install -c
INSTALLDATA = /usr/bin/install -c -m 644

srcdir = .
prefix = $(DESTDIR)
bindir = $(prefix)/usr/bin
docdir = $(prefix)/usr/share/doc

all: lightum

lightum:
	$(CC) $(CFLAGS) lightum.c -o lightum

install: all
	mkdir -p $(bindir)
	$(INSTALL) lightum $(bindir)/lightum
	mkdir -p $(prefix)/etc/xdg/autostart/
	$(INSTALLDATA) lightum.desktop $(prefix)/etc/xdg/autostart/
	mkdir -p $(docdir)/lightum/
	$(INSTALLDATA) $(srcdir)/README $(docdir)/lightum/
	$(INSTALLDATA) $(srcdir)/LICENSE $(docdir)/lightum/

uninstall:
	rm -rf $(bindir)/lightum
	rm -rf $(prefix)/etc/xdg/autostart/lightum.desktop
	rm -rf $(docdir)/lightum/

clean:
	rm lightum 2>/dev/null || exit 0

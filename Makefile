all: lightum

lightum:
	gcc lightum.c -o lightum -Wall

install:
	cp lightum /usr/bin/
	cp lightum.desktop /etc/xdg/autostart/
	mkdir -p /usr/share/doc/lightum/
	cp README /usr/share/doc/lightum/
	cp LICENSE /usr/share/doc/lightum/

uninstall:
	rm /usr/bin/lightum
	rm /etc/xdg/autostart/lightum.desktop
	rm -rf /usr/share/doc/lightum/

clean:
	rm lightum 2>/dev/null || exit 0

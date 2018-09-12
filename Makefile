.PHONY: default
default: build ;

PREFIX ?= /usr

build:
	mkdir -p build
	cd build; qmake -qt=qt5 ../Moolticute.pro; $(MAKE)

check: test ;

test:
	cd build; $(MAKE) check

install:
	install -m 755 -d "$(DESTDIR)$(PREFIX)/bin" "$(DESTDIR)/lib/udev/rules.d" "$(DESTDIR)$(PREFIX)/share/applications" "$(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps" "$(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps" "$(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps"
	install -m 755 build/moolticute "$(DESTDIR)$(PREFIX)/bin/"
	install -m 755 build/moolticuted "$(DESTDIR)$(PREFIX)/bin/"
	install -m 644 69-mooltipass.rules "$(DESTDIR)/lib/udev/rules.d/"
	install -m 644 data/moolticute.desktop "$(DESTDIR)$(PREFIX)/share/applications/"
	install -m 644 img/AppIcon.svg "$(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/moolticute.svg"
	install -m 644 img/AppIcon_32.png "$(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps/moolticute.png"
	install -m 644 img/AppIcon_128.png "$(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/moolticute.png"

uninstall:
	rm "$(DESTDIR)$(PREFIX)/bin/moolticute"
	rm "$(DESTDIR)$(PREFIX)/bin/moolticuted"
	rm "$(DESTDIR)/lib/udev/rules.d/69-mooltipass.rules"
	rm "$(DESTDIR)$(PREFIX)/share/applications/moolticute.desktop"
	rm "$(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/moolticute.svg"
	rm "$(DESTDIR)$(PREFIX)/share/icons/hicolor/32x32/apps/moolticute.png"
	rm "$(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/moolticute.png"

clean:
	rm -rf build

distclean: clean ;


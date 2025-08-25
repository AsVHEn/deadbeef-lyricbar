CFLAGS+=-std=c99 -Wall -O2 -D_GNU_SOURCE -fPIC -fvisibility=hidden -flto=auto
CXXFLAGS+=-std=c++14 -Wall -O2 -fPIC -fvisibility=hidden -flto=auto
LIBFLAGS=`pkg-config --cflags $(GTKMM) $(GTK)`
LIBS=`pkg-config --libs $(GTKMM) $(GTK)`
LCURL=-lcurl
LDFLAGS+=-flto=auto
GLIBC=glib-compile-resources
LOCALES = es ru

PO_FILES = $(foreach loc, $(LOCALES), gettext/$(loc)/deadbeef-lyricbar.po)
MO_FILES = $(PO_FILES:.po=.mo)

#prefix ?= $(out)
prefix ?= /usr

gtk3: GTKMM=gtkmm-3.0  taglib
gtk3: GTK=gtk+-3.0
gtk3: LYRICBAR=ddb_lyricbar_gtk3.so
gtk3: lyricbar
gtk3: mos
gtk3: update_po

gtk2: GTKMM=gtkmm-2.4
gtk2: GTK=gtk+-2.0
gtk2: LYRICBAR=ddb_lyricbar_gtk2.so
gtk2: lyricbar
gtk2: mos
gtk2: update_po

mos: $(MO_FILES)

%.mo: %.po
	msgfmt -o $@ $<

update_po:
	xgettext -o gettext/deadbeef-lyricbar.pot --keyword=_ --language=C++ src/*.cpp src/sources/*.cpp
	$(foreach po,$(PO_FILES), msgmerge --update $(po) gettext/deadbeef-lyricbar.pot;)

lyricbar: resource.h config_dialog.o lrclrclib.o lrcmusic163.o megalobiz.o azlyrics.o rclyricsband.o ui.o utils.o resources.o main.o
	$(if $(LYRICBAR),, $(error You should only access this target via "gtk3" or "gtk2"))
	$(CXX) -rdynamic -shared $(LDFLAGS) main.o resources.o config_dialog.o lrcspotify.o lrclrclib.o lrcmusic163.o megalobiz.o azlyrics.o rclyricsband.o ui.o utils.o $(LCURL) -o $(LYRICBAR) $(LIBS)

#lrcspotify.o: src/sources/lrcspotify.cpp src/sources/lrcspotify.h
#	$(CXX) src/sources/lrcspotify.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl
	
lrclrclib.o: src/sources/lrclrclib.cpp src/sources/lrclrclib.h
	$(CXX) src/sources/lrclrclib.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl
	
lrcmusic163.o: src/sources/lrcmusic163.cpp src/sources/lrcmusic163.h
	$(CXX) src/sources/lrcmusic163.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl

megalobiz.o: src/sources/megalobiz.cpp src/sources/megalobiz.h
	$(CXX) src/sources/megalobiz.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl

azlyrics.o: src/sources/azlyrics.cpp src/sources/azlyrics.h
	$(CXX) src/sources/azlyrics.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl
	
rclyricsband.o: src/sources/rclyricsband.cpp src/sources/rclyricsband.h
	$(CXX) src/sources/rclyricsband.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl

ui.o: src/ui.cpp src/ui.h
	$(CXX) src/ui.cpp -c $(LIBFLAGS) $(CXXFLAGS)

utils.o: src/utils.cpp src/utils.h
	$(CXX) src/utils.cpp -c $(LIBFLAGS) $(CXXFLAGS)

config_dialog.o: src/config_dialog.cpp src/resources.h
	$(CXX) src/config_dialog.cpp  -c $(LIBFLAGS) $(CXXFLAGS)

resources.o: src/resources.c
	$(CC) $(CFLAGS) src/resources.c -c `pkg-config --cflags $(GTK)`

main.o: src/main.c
	$(CC) $(CFLAGS) src/main.c -c `pkg-config --cflags $(GTK)`

resource.h:
	$(GLIBC) --generate-source src/resources.xml
	$(GLIBC) --generate-header src/resources.xml

install:
	install -d $(prefix)/lib/x86_64-linux-gnu/deadbeef
	install -m 666 -D *.so $(prefix)/lib/x86_64-linux-gnu/deadbeef
	install -d $(prefix)/share/locale/es/LC_MESSAGES
	$(foreach loc, $(LOCALES), msgfmt gettext/$(loc)/deadbeef-lyricbar.po -o $(prefix)/share/locale/$(loc)/LC_MESSAGES/deadbeef-lyricbar.mo;)

clean:
	rm -f *.o *.so locales/*/*/*.mo

CFLAGS+=-std=c99 -Wall -O2 -D_GNU_SOURCE -fPIC -fvisibility=hidden -flto=auto
CXXFLAGS+=-std=c++14 -Wall -O2 -fPIC -fvisibility=hidden -flto=auto
LIBFLAGS=`pkg-config --cflags $(GTKMM) $(GTK)`
LIBS=`pkg-config --libs $(GTKMM) $(GTK)`
LCURL=-lcurl
LDFLAGS+=-flto=auto

prefix ?= $(out)
prefix ?= /usr

gtk3: GTKMM=gtkmm-3.0  taglib
gtk3: GTK=gtk+-3.0
gtk3: LYRICBAR=ddb_lyricbar_gtk3.so
gtk3: lyricbar

gtk2: GTKMM=gtkmm-2.4
gtk2: GTK=gtk+-2.0
gtk2: LYRICBAR=ddb_lyricbar_gtk2.so
gtk2: lyricbar

lyricbar: config_dialog.o lrcspotify.o megalobiz.o azlyrics.o ui.o utils.o main.o
	$(if $(LYRICBAR),, $(error You should only access this target via "gtk3" or "gtk2"))
	$(CXX) -rdynamic -shared $(LDFLAGS) main.o config_dialog.o lrcspotify.o megalobiz.o azlyrics.o ui.o utils.o $(LCURL) -o $(LYRICBAR) $(LIBS)

lrcspotify.o: src/lrcspotify.cpp
	$(CXX) src/lrcspotify.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl

megalobiz.o: src/megalobiz.cpp
	$(CXX) src/megalobiz.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl

azlyrics.o: src/azlyrics.cpp
	$(CXX) src/azlyrics.cpp -c $(LIBFLAGS) $(CXXFLAGS) -lcurl

ui.o: src/ui.cpp
	$(CXX) src/ui.cpp -c $(LIBFLAGS) $(CXXFLAGS)

utils.o: src/utils.cpp
	$(CXX) src/utils.cpp -c $(LIBFLAGS) $(CXXFLAGS)

config_dialog.o: src/config_dialog.cpp
	$(CXX) src/config_dialog.cpp  -c $(LIBFLAGS) $(CXXFLAGS)

main.o: src/main.c
	$(CC) $(CFLAGS) src/main.c -c `pkg-config --cflags $(GTK)`

install:
	install -d $(prefix)/lib/deadbeef
	install -d $(prefix)/share/locale/ru/LC_MESSAGES
	install -m 666 -D *.so $(prefix)/lib/deadbeef
	msgfmt gettext/ru/deadbeef-lyricbar.po -o $(prefix)/share/locale/ru/LC_MESSAGES/deadbeef-lyricbar.mo

clean:
	rm -f *.o *.so


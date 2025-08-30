Based on loskutov https://github.com/loskutov/deadbeef-lyricbar

# DeaDBeeF Lyricbar Plugin
Plugin for [DeaDBeeF audio player](https://github.com/DeaDBeeF-Player/deadbeef) that fetches and shows the song’s synclyrics using various website, also from metadata or lrc/txt file (same name on same folder as track).

![GIF](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/2506a8cb-2c94-4a73-99c7-33b7aa22e26e)


## Dependencies
To use this plugin, you need to have [gtkmm](http://www.gtkmm.org/) and [gtk3](https://www.gtk.org/) installed. (also: libcurl and libtag)

You need deadbeef.h file to build this plugin. The file /usr/include/deadbeef/deadbeef.h should've been installed with the player itself. If not -- look for deadbeef-plugin-dev package, or something like this. Or get the file from a source tarball.

## Installation
Just download compiled file _ddb_lyricbar_gtk3.so_, and copy it to ~/.local/lib/deadbeef or:

Clone this repository and perform the following:
```sh
make [gtk3]
sudo install
# OR, to install for the current user only
mkdir -p ~/.local/lib/deadbeef && cp *.so ~/.local/lib/deadbeef # depends on where deadbeef is installed
sudo cp ./gettext/[your language]/*.mo /usr/share/locale/[your language]/LC_MESSAGES/ 
```

## Usage
Activate Design Mode (View → Design mode) and add Lyricbar somewhere. Disable Design Mode back and edit appareance if you want in config options:

![Config](https://github.com/deadbeef-lyricbar/assets/fefa231b-7241-4bb9-b0ce-f76ccd69e8b8)

Lyrics will be stored on tags "LYRICS" for synced, and "UNSYNCEDLYRICS" for non-sync. SYLT tags will be removed if exists.

Automatic download if no tag or file is found. Also you can manually search for lyrics with a right-click on plugin. ~~To be able to get lyrics from Spoti-fy you will need to input your account's SP-DC on Edit/Preferences/Plugins/Lyricbar (One easy way to know your SP-DC: Install [Cookie-editor](https://cookie-editor.com/) browser extension and login into your account).~~ (Not anymore because of Spotify changes).

~~Same to use AZlyrics with the last part of a manual search URL.~~ (Trying to find another way).
![Search](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/03b1ade0-11da-4c69-b85b-cb3f26ed8b65)

Right now, working sites are:
- LRCLIB.
- RCLyricsBand.
- Megalobiz.
- Music 163.


There is also a window to edit lyrics.
![Edit](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/5e2c30b6-e21b-483e-abe6-c0d12ed13d84)

In addition, if you want to traslate to your language you only need copy deadbeef-lyricbar/gettext/deadbeef-lyricbar.pot, rename to deadbeef-lyricbar.po, edit it and put /gettext/[your language]/ before compiling. Also if you're not satisfied with lyrics providers you can help adding another ones :D.

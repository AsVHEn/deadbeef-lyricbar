Based on loskutov https://github.com/loskutov/deadbeef-lyricbar

# DeaDBeeF Lyricbar Plugin
Plugin for DeaDBeeF audio player that fetches and shows the song’s synclyrics using various website, also from metadata or lrc/txt file (same name on same folder as track).

![GIF](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/2506a8cb-2c94-4a73-99c7-33b7aa22e26e)


## Dependencies
To use this plugin, you need to have [gtkmm](http://www.gtkmm.org/) and [gtk3](https://www.gtk.org/) installed.

You need deadbeef.h file to build this plugin. The file /usr/include/deadbeef/deadbeef.h should've been installed with the player itself. If not -- look for deadbeef-plugin-dev package, or something like this. Or get the file from a source tarball.

## Installation
Just download compiled file _ddb_lyricbar_gtk3.so_ and panels.glade, and copy them to ~/.local/lib/deadbeef or:

Clone this repository and perform the following:
```sh
make [gtk3]
sudo cp *.so /usr/lib/deadbeef # depends on where deadbeef is installed
sudo cp *.glade ~/.local/lib/deadbeef/ # glade file MUST be in this path
# OR, to install for the current user only
mkdir -p ~/.local/lib/deadbeef && cp *.so *.glade ~/.local/lib/deadbeef
```

## Usage
Activate Design Mode (View → Design mode) and add Lyricbar somewhere. Disable Design Mode back and edit appareance if you want in config options:
![Options](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/a4fe1043-3b8c-417d-877f-06826bb2eb71)

Lyrics will be stored on tags "LYRICS" for synced, and "UNSYNCEDLYRICS" for non-sync. SYLT tags will be removed if exists.

Automatic download if no tag or file is found. Also you can manually search for lyrics with a right-click on plugin. To be able to get lyrics from Spoti-fy you will need to input your account's SP-DC on Edit/Preferences/Plugins/Lyricbar.
Same to use AZlyrics with the last part of a manual search URL.
![Search](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/cc7e38c9-8046-423f-ab2b-aebd957fd8be)

There is also a window to edit lyrics.
![Edit](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/85eed130-f9bb-44f9-83de-8a74f9f5aad3)

In addition, if you're not satisfied with lyrics providers you can help adding another ones :D.



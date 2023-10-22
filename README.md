Based on loskutov https://github.com/loskutov/deadbeef-lyricbar

# DeaDBeeF Lyricbar Plugin
Plugin for [DeaDBeeF audio player](https://github.com/DeaDBeeF-Player/deadbeef) that fetches and shows the song’s synclyrics using various website, also from metadata or lrc/txt file (same name on same folder as track).

![GIF](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/2506a8cb-2c94-4a73-99c7-33b7aa22e26e)


## Dependencies
To use this plugin, you need to have [gtkmm](http://www.gtkmm.org/) and [gtk3](https://www.gtk.org/) installed.

You need deadbeef.h file to build this plugin. The file /usr/include/deadbeef/deadbeef.h should've been installed with the player itself. If not -- look for deadbeef-plugin-dev package, or something like this. Or get the file from a source tarball.

## Installation
Just download compiled file _ddb_lyricbar_gtk3.so_, and copy it to ~/.local/lib/deadbeef or:

Clone this repository and perform the following:
```sh
make [gtk3]
sudo cp *.so /usr/lib/deadbeef # depends on where deadbeef is installed
# OR, to install for the current user only
mkdir -p ~/.local/lib/deadbeef && cp *.so ~/.local/lib/deadbeef
```

## Usage
Activate Design Mode (View → Design mode) and add Lyricbar somewhere. Disable Design Mode back and edit appareance if you want in config options:

![Config](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/5cbf1cba-9eff-4694-ad34-d814b8abe84f)

Lyrics will be stored on tags "LYRICS" for synced, and "UNSYNCEDLYRICS" for non-sync. SYLT tags will be removed if exists.

Automatic download if no tag or file is found. Also you can manually search for lyrics with a right-click on plugin. To be able to get lyrics from Spoti-fy you will need to input your account's SP-DC on Edit/Preferences/Plugins/Lyricbar.
Same to use AZlyrics with the last part of a manual search URL.
![Search](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/03b1ade0-11da-4c69-b85b-cb3f26ed8b65)

There is also a window to edit lyrics.
![Edit](https://github.com/AsVHEn/deadbeef-lyricbar/assets/4272271/5e2c30b6-e21b-483e-abe6-c0d12ed13d84)

In addition, if you're not satisfied with lyrics providers you can help adding another ones :D.



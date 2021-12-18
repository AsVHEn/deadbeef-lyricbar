Based on loskutov https://github.com/loskutov/deadbeef-lyricbar

Totally WIP status!!!!!

# DeaDBeeF Lyricbar Plugin
Plugin for DeaDBeeF audio player that fetches and shows the song’s synclyrics using syair website (also from metadata or lrc file).

![MainWindow](https://user-images.githubusercontent.com/4272271/129489072-7d3b6b56-a8da-4230-b476-3cee2c3ca1bb.png)


## Dependencies
To use this plugin, you need to have [gtkmm](http://www.gtkmm.org/) installed.

You need deadbeef.h file to build this plugin. The file /usr/include/deadbeef/deadbeef.h should've been installed with the player itself. If not -- look for deadbeef-plugin-dev package, or something like this. Or get the file from a source tarball.

## Installation
Just download compiled file _ddb_lyricbar_gtk3.so_ and copy to ~/.local/lib/deadbeef or:

Clone this repository and perform the following:
```sh
make [gtk2 or gtk3]
sudo cp *.so /usr/lib/deadbeef # depends on where deadbeef is installed
# OR, to install for the current user only
mkdir -p ~/.local/lib/deadbeef && cp *.so ~/.local/lib/deadbeef
```

## Usage
Activate Design Mode (View → Design mode) and add Lyricbar somewhere. Disable Design Mode back and enjoy the music :)

In addition, if you're not satisfied with LyricWiki, external lyrics providers can be used (see plugin preferences, the script launch command can use the whole [DeaDBeeF title formatting](https://github.com/DeaDBeeF-Player/deadbeef/wiki/Title-formatting-2.0) power, it's supposed to output the lyrics to stdout).

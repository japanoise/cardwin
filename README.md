# CardWin

CardWin is a modern replacement for the `cardfile.exe` program distributed with
very old (pre-98) versions of Windows. It is capable of opening from and saving
to the same file format. It uses the relatively up-to-date GTK+ toolkit for a
cross-platform GUI, and should work on both 32-bit and 64-bit systems.

## Status

CardWin is in "minimum viable product" (MVP) status:

* You can open cardfiles which are in MGC format with a high degree of accuracy
* You can view, edit, and save text in MGC format just as you'd expect
* CardWin will not clobber any existing bitmap data - it'll copy it into memory
  but leave it alone, i.e. will not display or edit it.
* Cardfiles produced will be byte-for-byte identical with cardfiles produced by
  the original executable
* Cardfiles produced should be compatible with and usable in the original
  executable

You can check if your files are compatible with CardWin by checking if the first
three bytes of the file are MGC. If they are RRG, DKO or something else, they're
probably not compatible - please open an issue and send me the cardfile if you
can, so I can improve the code.

ToDo:

* Improve the GUI. Modernize it a bit, make it resizable, port it to GTK3 (but
  not GTK4. I hate GTK4) or even to qt or an imgui or something.
* A way to search cards. Full text search is possible and trivial, would just
  need the interface wiring up.
* Displaying bitmaps. Simple enough - just read the data and construct a gtk
  image when displaying a card that has bmpsize > 0
* Once bitmaps can be displayed, a way to CRUD them or export them will be
  needed.
* Deal with RRG & DKO format. Needs:
  1. Sample data. I have 1 RRG cardfile and no DKO cardfiles.
  2. A better documentation of the OLE format
  3. Ideally, for the bitmap functionality to be ready

## Building

CardWin uses a GNU makefile. Linux users that have GTK2 installed should just be
able to `make`. Windows users should look into getting GTK+ development files,
probably using MSYS2, and go from there. I will work on a better build system
once the program is stable and if interest is generated I will also provide
pre-compiled binaries.

You can optionally run `make crd2json` to build a simple C program that converts
a CRD file to a human-readable JSON file, which may be useful as an intermediate
format to convert to other formats. I'm half-planning to write a CRD -> HTML
(maybe even TiddlyWiki) converter using this functionality...  Note that this
program uses a very stupid solution to convert non-ASCII bytes to UTF-8, and the
operator may need to make a manual transformation to preserve non-ASCII
characters, as the locale information is not preserved in the CRD file format.

## Copying

No copyright notice was present in the source code itself; however, the
sourceforge page lists it as licensed GPLv2. I will take it on good faith that
it is Copyright 2001 Cam Menard, licensed under the GPLv2. My modifications are
Copyright 2023 japanoise, licensed under the GPLv2.

## History

The initial version of this program was created by Cam Menard in late 2001. Cam
half-finished it then evidently left it to languish in obscurity until I found
it. Cam's version is a 796-line GTK application which, while based off an old,
obsolete, and undocumented version of GTK, only requires some small
modifications to compile. GTK is a very bad and poorly-documented API
(apparently little has changed in that regard since 2001) and is made worse by
having to use it from C when it really wants to be in an OO language such as
C++. However, it has future-proofed Cam's old code to some extent. Only a
handful of features were deprecated since the 2001 release.

I've rewritten the code, but anyone familiar with one version will note the
broad strokes are the same. Some functions are line-by-line the same or almost
the same. But we have some advantages!

1. It compiles with a (relatively) recent version of GTK that's not hard to get
2. It can save your changes
3. It has a "dirty buffer" indicator that works
4. It can at least attempt to read RRG-style cardfiles

As for me, I'm a retro computing enthusiast and I've heard good things about
Windows 3.11's `cardfile.exe`. Having used it I am honestly more fond of
e.g. Joplin or even MacOS's default Notes program, but I can see the appeal and
it would be nice to have a modern program to open and edit these legacy files.
There seems to be a real hunger for it on the internet - at least as much a
hunger a very old Windows program can generate - and it lacks any other obvious
successor.

The original sources:

* [sourceforge project page](https://sourceforge.net/projects/cardwin/)
* [sourceforge website](https://cardwin.sourceforge.net/)

Useful links (especially for developers):

* [Cardfile at Just Solve The File Format
  Problem](http://fileformats.archiveteam.org/wiki/Cardfile)
* [Q99340: Windows 3.1 Card File
  Format](https://jeffpar.github.io/kbarchive/kb/099/Q99340/)
* [GTK2 API Documentation](https://developer-old.gnome.org/gtk2/stable/) (such
  as it is...)
* [Some samples of the file format can be found
  here](https://web.archive.org/web/20220323222741/https://telparia.com/fileFormatSamples/document/cardfile/)

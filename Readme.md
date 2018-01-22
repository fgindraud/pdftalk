PDFTalk - PDF presentation tool
===============================

[![Build Status](https://travis-ci.org/lereldarion/pdftalk.svg?branch=master)](https://travis-ci.org/lereldarion/pdftalk)
[![Localshare License](https://img.shields.io/badge/license-GPL3-blue.svg)](#license)
[![Latest release](https://img.shields.io/github/release/lereldarion/pdftalk.svg)](https://github.com/lereldarion/qt-localshare/releases/latest)

pdfpc replacement built using Qt5.

Setup
-----

Compilation:
```
qmake
make
```

Requires Qt >= 5.3, C++11 compiler support, and poppler (including Qt5 bindings).
Details about dependencies can be found in the `build/*/requirement.sh` files.

Usage
-----

Starting the presentation tool is simple (no options for now):
```
pdftalk <pdf_document>
```

It generates 2 windows, one for the spectators with the current slide, and one for the presenter with neighbouring slides, a timer, and slide numbering.
The windows can be placed on the two screens (use `s` key to swap them), and can be made fullscreen (`f` key).
Navigation is standard (`→` `←` `space` keys).
The timer can be paused/resumed with `p`, and resetted with `r`.

The presenter window can show text annotations.
It follows the pdfpc model: a text file named `<pdf_file_name>.pdfpc` in the same directory as the pdf file.
The text file can easily be generated using the [pdfpc-latex-notes](https://github.com/cebe/pdfpc-latex-notes) package.

Status
------

Beta stage: working, lacking features and maturity

Todo:
* PDF rendering
	* Done: threadpool rendering, prefetching the next pages on render
	* Improved prefetch
		* better pattern: currently, just prerender the next few pages
		* add role information to render system
		* add weights to roles (public screen = highest)
		* settings for cache size
		* backwards prefetching
* Go to page/slide n functionnality

Maybe Todo:
* Auto spread windows on monitors
* Disable screensaver

Unlikely Todo:
* Support for durations (no animations for now)
* Support for Poppler Rotation flags (don't know when it matters)
* Movies
* Transitions / animations

License
-------

```
PDFTalk - PDF presentation tool
Copyright (C) 2016 - 2018 Francois Gindraud

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```

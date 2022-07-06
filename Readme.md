PDFTalk - PDF presentation tool
===============================

[![CI](https://github.com/lereldarion/pdftalk/actions/workflows/ci.yml/badge.svg)](https://github.com/lereldarion/pdftalk/actions)
[![Localshare License](https://img.shields.io/badge/license-GPL3-blue.svg)](#license)
[![Latest release](https://img.shields.io/github/release/lereldarion/pdftalk.svg)](https://github.com/lereldarion/pdftalk/releases/latest)

Pdf document viewer specialised for doing presentations with _beamer_.
This has been heavily inspired by [pdfpc](https://github.com/pdfpc/pdfpc), but built using Qt5.
Key features :
* One window for public, one for presenter with additional information (slide numbering, timer, text annotations, next slide).
* Pdf render _cache_ and _prefect_ system, to enable very fast slide changes.

PdfTalk was initially developped to fit my use case : simple, fast, and tiling-window-manager-friendly.
_pdfpc_ has more features and is more actively supported.

Setup
-----

Compilation:
```
qmake
make
```

Requirements:
- C++11 compiler support
- Qt >= 5.3
- poppler library with Qt5 bindings

Usage
-----

Starting the presentation tool is simple:
```
pdftalk <pdf_document>
```

PdfTalk creates 2 windows : one for the spectators with the current slide, and one for the presenter with neighbouring slides, a timer, and slide numbering.
The windows can be placed on the two screens (use `s` key to swap them), and can be made fullscreen (`f` key).
Navigation is standard (`→` `←` `space` `home` `end` keys).
The timer can be paused/resumed with `p`, and resetted with `r`.

A summary of slides timing can ge written to a file after the presentation using `t`.
For each visited slides, it indicates when the slide was first reached, and the cumulated time spent on the slide.

The presenter window can show text annotations.
It follows the **old** pdfpc model: a text file named `<pdf_file_name>.pdfpc` in the same directory as the pdf file.
The text file can easily be generated using the [pdfpc-latex-notes](https://github.com/cebe/pdfpc-latex-notes) package.
A copy can be found in `test/`.

Roadmap
-------

FIXME annotation handling:
* pdfpc file format has changed over the years. Most recent seems to be JSON. [related issue](https://github.com/pdfpc/pdfpc/issues/605).
* there is a [pdfpc.sty](https://www.ctan.org/tex-archive/macros/latex/contrib/pdfpc) package in latex-extra. Seems to default to PDF internal annotations.
* revisit using pdf annotations ? [pdfpc handling here](https://github.com/pdfpc/pdfpc/blob/master/src/classes/metadata/pdf.vala). And/or old commit when I tried using them.

Some additional functionnality would be useful:
* Go to page/slide n
* Presentation slide overview mode ? Need to choose page sizes. Toggle with 'o' ? Could replace "go to n".

Maybe useful:
* Disable screensaver ; no standard way to do this.
* Spread windows on multiple screens on startup ?

Will not support:
* Support for durations (no animations for now)
* Support for Poppler Rotation flags (don't know when it matters)
* Movies
* Transitions / animations

License
-------

```
PDFTalk - PDF presentation tool
Copyright (C) 2016 - 2022 Francois Gindraud

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

PDFTalk - PDF presentation tool
===============================

[![Build Status](https://travis-ci.org/lereldarion/pdftalk.svg?branch=master)](https://travis-ci.org/lereldarion/pdftalk)
[![Localshare License](https://img.shields.io/badge/license-GPL3-blue.svg)](#license)
[![Latest release](https://img.shields.io/github/release/lereldarion/pdftalk.svg)](https://github.com/lereldarion/qt-localshare/releases/latest)

A small presentation tool like pdfpc, but using Qt.

Setup
-----

Compilation:
```
qmake
make
```

Requires Qt >= 5.2, c++11 compiler support, and poppler (including Qt5 bindings).
Details about dependencies can be found in the `build/*/requirement.sh` files.

Status
------

This is only a partial prototype...

Todo:
* PDF rendering
	* Basic on demand one for now
	* Add prerendering, threading, etc...
* Support for durations
* Showing comments & such
* Auto spread windows on monitors

License
-------

```
PDFTalk - PDF presenation tool
Copyright (C) 2016 Francois Gindraud

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

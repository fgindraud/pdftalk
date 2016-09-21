/* PDFTalk - PDF presentation tool
 * Copyright (C) 2016 Francois Gindraud
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef PRESENTATION_WINDOW_H
#define PRESENTATION_WINDOW_H

#include "pixmap_label.h"

#include <QPalette>

class PresentationWindow : public PixmapLabel {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
	Q_OBJECT

public:
	explicit PresentationWindow (QWidget * parent = nullptr) : PixmapLabel (parent) {
		// Title
		setWindowTitle (tr ("Presentation screen"));
		// Black background
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		setPalette (p);
		setAutoFillBackground (true);
	}
};

#endif

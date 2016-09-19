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

#include <QLabel>
#include <QPalette>
#include <QResizeEvent>

class PresentationWindow : public QLabel {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
	Q_OBJECT

public:
	PresentationWindow (QWidget * parent = nullptr) : QLabel (parent) {
		// Title
		setWindowTitle (tr ("Presentation screen"));
		// Center
		setAlignment (Qt::AlignCenter);
		// Black background
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		setPalette (p);
		setAutoFillBackground (true);
	}

signals:
	void size_changed (QSize new_size);

private:
	void resizeEvent (QResizeEvent * event) Q_DECL_OVERRIDE {
		QLabel::resizeEvent (event);
		emit size_changed (size ());
	}
};

#endif

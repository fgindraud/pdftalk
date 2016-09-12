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
#ifndef MAIN_H
#define MAIN_H

#include "document.h"

#include <QDebug>
#include <QLabel>
#include <QPalette>
#include <QResizeEvent>
#include <QShortcut>

class PresentationWindow : public QLabel {
	Q_OBJECT

private:
	const Document & document_;
	int current_page_{0};

public:
	PresentationWindow (const Document & document) : document_ (document) {
		{
			auto sc = new QShortcut (QKeySequence (tr ("Right")), this);
			connect (sc, &QShortcut::activated, this, &PresentationWindow::next_slide);
		}
		{
			auto sc = new QShortcut (QKeySequence (tr ("Left")), this);
			connect (sc, &QShortcut::activated, this, &PresentationWindow::prev_slide);
		}

		// Title
		setWindowTitle (tr ("Presentation screen"));
		// Center
		setAlignment (Qt::AlignCenter);
		// Black background
		QPalette p (palette ());
		p.setColor (QPalette::Background, Qt::black);
		setPalette (p);
		setAutoFillBackground (true);
	}

private slots:
	void next_slide (void) {
		current_page_ = std::min (current_page_ + 1, document_.nb_pages () - 1);
		render ();
	}
	void prev_slide (void) {
		current_page_ = std::max (current_page_ - 1, 0);
		render ();
	}

private:
	void resizeEvent (QResizeEvent * event) Q_DECL_OVERRIDE {
		QLabel::resizeEvent (event);
		render ();
	}

	void render (void) { setPixmap (document_.render (current_page_, size ())); }
};

#endif

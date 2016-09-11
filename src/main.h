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

#include <memory> // unique_ptr
#include <poppler-qt5.h>

// Rendering
#include <QDebug>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QResizeEvent>
#include <QShortcut>
#include <QShowEvent>

class PresentationWindow : public QLabel {
	Q_OBJECT

private:
	std::unique_ptr<Poppler::Document> document;
	int current_page{0};

public:
	PresentationWindow (const QString & filename) : document (Poppler::Document::load (filename)) {
		if (!document || document->isLocked ()) {
			qFatal ("Unable to open pdf");
		}
		if (document->numPages () <= 0) {
			qFatal ("Document has no content !");
		}
		// Antialiasing is necessary
		document->setRenderHint (Poppler::Document::Antialiasing, true);
		document->setRenderHint (Poppler::Document::TextAntialiasing, true);

		{
			auto sc = new QShortcut (QKeySequence (tr ("Right")), this);
			connect (sc, &QShortcut::activated, this, &PresentationWindow::next_slide);
		}
		{
			auto sc = new QShortcut (QKeySequence (tr ("Left")), this);
			connect (sc, &QShortcut::activated, this, &PresentationWindow::prev_slide);
		}

		// Title
		setWindowTitle (tr ("Presentation screen - %1").arg (filename));
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
		current_page = std::min (current_page + 1, document->numPages () - 1);
		render ();
	}
	void prev_slide (void) {
		current_page = std::max (current_page - 1, 0);
		render ();
	}

private:
	void resizeEvent (QResizeEvent * event) Q_DECL_OVERRIDE {
		QLabel::resizeEvent (event);
		render ();
	}

	void render (void) {
		QSize s = size ();
		// Render current page, must fit in size
		std::unique_ptr<Poppler::Page> page (document->page (current_page));
		Q_ASSERT (page);
		// Determine render size with aspect ratio
		const auto page_size_dots = page->pageSizeF ();
		const qreal dpi =
		    std::min (static_cast<qreal> (s.width ()) / (page_size_dots.width () / 72.0),
		              static_cast<qreal> (s.height ()) / (page_size_dots.height () / 72.0));
		//qDebug () << s << dpi;
		auto image = page->renderToImage (dpi, dpi);
		Q_ASSERT (!image.isNull ());
		auto pixmap = QPixmap::fromImage (std::move (image));
		setPixmap (pixmap);
	}
};

#endif

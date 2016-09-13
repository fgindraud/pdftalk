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
#ifndef PRESENTATION_H
#define PRESENTATION_H

#include "document.h"

#include <QPixmap>
#include <QShortcut>
class QWidget;

class Presentation : public QObject {
	/* Manage a presentation state.
	 * Coordinates the two views, and get renders and data from the Document.
	 */
	Q_OBJECT

private:
	const Document document_;
	int current_page_{0};
	QSize presentation_window_size_;
	QPixmap current_pixmap_;

public:
	Presentation (const QString & filename) : document_ (filename) {}

signals:
	void new_presentation_pixmap (QPixmap pixmap);
	void new_text_TEST (QString);

public slots:
	void go_to_next_page (void) {
		if (current_page_ + 1 < document_.nb_pages ()) {
			current_page_++;
			render ();
		}
	}
	void go_to_prev_page (void) {
		if (current_page_ > 0) {
			current_page_--;
			render ();
		}
	}

	void presentation_window_size_changed (QSize new_size) {
		if (new_size != presentation_window_size_) {
			presentation_window_size_ = new_size;
			render ();
		}
	}

private:
	void render (void) {
		current_pixmap_ = document_.render (current_page_, presentation_window_size_);
		emit new_presentation_pixmap (current_pixmap_);
		emit new_text_TEST (QString ("Slide %1/%2")
		                        .arg (document_.slide_index_of_page (current_page_) + 1)
		                        .arg (document_.nb_slides ()));
	}
};

inline void add_presentation_shortcuts_to_widget (Presentation & presentation, QWidget * widget) {
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Right", "next_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &presentation, &Presentation::go_to_next_page);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Left", "prev_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &presentation, &Presentation::go_to_prev_page);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Space", "next_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &presentation, &Presentation::go_to_next_page);
	}
}

#endif

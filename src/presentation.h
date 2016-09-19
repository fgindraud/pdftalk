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

#include <QDebug>

#include <QBasicTimer>
#include <QPixmap>
#include <QShortcut>
#include <QTime>
#include <QTimerEvent>
class QWidget;

class Timing : public QObject {
	/* Timer for a presentation.
	 * Can be paused, restarted, resetted.
	 * Tracks the time spent in the presentation between pauses.
	 * Emits periodic signals to update the gui.
	 */
	Q_OBJECT

private:
	QBasicTimer timer_;
	QTime accumulated_; // Accumulated time until last pause
	QTime last_resume_; // Time of last resume

signals:
	void update (bool paused, QString time_text);

public slots:
	void start (void) {
		if (!timer_.isActive ())
			start_or_resume_timing ();
	}
	void toggle_pause (void) {
		if (timer_.isActive ()) {
			timer_.stop ();
			accumulated_ = accumulated_.addMSecs (last_resume_.elapsed ());
			emit_update ();
		} else {
			start_or_resume_timing ();
		}
	}
	void reset (void) {
		timer_.stop ();
		accumulated_ = QTime{0, 0, 0};
		emit_update (); // Everything changed
	}

private:
	void emit_update (void) {
		QTime total = accumulated_;
		if (timer_.isActive ())
			total = total.addMSecs (last_resume_.elapsed ());
		emit update (!timer_.isActive (), total.toString (tr ("HH:mm:ss")));
	}
	void timerEvent (QTimerEvent * event) Q_DECL_OVERRIDE {
		event->accept ();
		emit_update ();
	}
	void start_or_resume_timing (void) {
		timer_.start (1000, this); // FIXME can cause misticks if too small...
		last_resume_.start ();
		emit_update (); // Pause status changed
	}
};

class Presentation : public QObject {
	/* Manage a presentation state.
	 * Coordinates the two views, and get renders and data from the Document.
	 */
	Q_OBJECT

private:
	const Document document_;
	int current_page_{0};   // Main iterator over document
	int current_slide_{-1}; // Cached from document
	QSize presentation_window_size_;
	Timing timer_;
	QPixmap current_pixmap_;

public:
	Presentation (const QString & filename) : document_ (filename) {
		connect (&timer_, &Timing::update, this, &Presentation::time_changed);
	}
	int nb_slides (void) const { return document_.nb_slides (); }

signals:
	void presentation_pixmap_changed (QPixmap pixmap);

	void slide_changed (int new_slide_number);
	void time_changed (bool paused, QString new_time_text);

public slots:
	// Page navigation
	void go_to_next_page (void) {
		if (current_page_ + 1 < document_.nb_pages ()) {
			current_page_++;
			timer_start ();
			render ();
		}
	}
	void go_to_prev_page (void) {
		if (current_page_ > 0) {
			current_page_--;
			timer_start ();
			render ();
		}
	}

	// Timer control
	void timer_start (void) { timer_.start (); }
	void timer_toggle_pause (void) { timer_.toggle_pause (); }
	void timer_reset (void) { timer_.reset (); }

	// Interface changes
	void presentation_window_size_changed (QSize new_size) {
		if (new_size != presentation_window_size_) {
			presentation_window_size_ = new_size;
			render ();
		}
	}

private:
	void render (void) {
		current_pixmap_ = document_.render (current_page_, presentation_window_size_);
		emit presentation_pixmap_changed (current_pixmap_);
		{
			auto slide = document_.slide_index_of_page (current_page_);
			if (slide != current_slide_) {
				current_slide_ = slide;
				emit slide_changed (slide);
			}
		}
	}
};

inline void add_presentation_shortcuts_to_widget (Presentation & presentation, QWidget * widget) {
	// Page navigation
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
	// 'g' for goto page prompt
	// Timer control
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("P", "timer_toggle_pause key")), widget);
		QObject::connect (sc, &QShortcut::activated, &presentation, &Presentation::timer_toggle_pause);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("R", "timer_reset key")), widget);
		QObject::connect (sc, &QShortcut::activated, &presentation, &Presentation::timer_reset);
	}
}

#endif

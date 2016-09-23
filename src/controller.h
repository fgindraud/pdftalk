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
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "cache.h"
#include "document.h"

#include <QBasicTimer>
#include <QPixmap>
#include <QShortcut>
#include <QTime>
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
	void timerEvent (QTimerEvent *) Q_DECL_OVERRIDE { emit_update (); }
	void start_or_resume_timing (void) {
		timer_.start (1000, this); // FIXME can cause misticks if too small...
		last_resume_.start ();
		emit_update (); // Pause status changed
	}
};

class Controller : public QObject {
	/* Manage a presentation state.
	 * Coordinates the two views, and get renders and data from the Document.
	 */
	Q_OBJECT

private:
	const Document & document_;
	PageIndex current_page_{0}; // Main iterator over document
	Timing timer_;

public:
	explicit Controller (const Document & document) : document_ (document) {
		connect (&timer_, &Timing::update, this, &Controller::time_changed);
	}

signals:
	void current_page_changed (PageIndex new_page);
	void next_slide_page_changed (PageIndex new_page);
	void next_transition_page_changed (PageIndex new_page);
	void previous_transition_page_changed (PageIndex new_page);

	void slide_changed (SlideIndex new_slide_number);
	void time_changed (bool paused, QString new_time_text);

public slots:
	// Page navigation
	void go_to_next_page (void) {
		if (current_page_ + 1 < document_.nb_pages ()) {
			current_page_++;
			timer_start ();
			update_views ();
		}
	}
	void go_to_previous_page (void) {
		if (current_page_ > 0) {
			current_page_--;
			timer_start ();
			update_views ();
		}
	}

	// Timer control
	void timer_start (void) { timer_.start (); }
	void timer_toggle_pause (void) { timer_.toggle_pause (); }
	void timer_reset (void) { timer_.reset (); }

private:
	void update_views (void) {
		emit current_page_changed (current_page_);
		emit slide_changed (document_.slide_index_of_page (current_page_));
		// TODO add other signals
	}
};

inline void add_shortcuts_to_widget (Controller & c, QWidget * widget) {
	// Page navigation
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Right", "next_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::go_to_next_page);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Left", "prev_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::go_to_previous_page);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Space", "next_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::go_to_next_page);
	}
	// 'g' for goto page prompt

	// Timer control
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("P", "timer_toggle_pause key")), widget);
		sc->setAutoRepeat (false);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::timer_toggle_pause);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("R", "timer_reset key")), widget);
		sc->setAutoRepeat (false);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::timer_reset);
	}
}

#endif

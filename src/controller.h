/* PDFTalk - PDF presentation tool
 * Copyright (C) 2016 - 2018 Francois Gindraud
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

#include "action.h"
#include "document.h"

#include <QBasicTimer>
#include <QPixmap>
#include <QShortcut>
#include <QTime>
class QWidget;

#include <QDebug>

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
	/* Manage a presentation state (which slide is currently viewed).
	 * Sends page updates to slide viewers depending on user input.
	 */
	Q_OBJECT

private:
	const Document & document_;
	int current_page_{0}; // Main iterator over document
	Timing timer_;

public:
	explicit Controller (const Document & document) : document_ (document) {
		connect (&timer_, &Timing::update, this, &Controller::time_changed);
	}

signals:
	void current_page_changed (const PageInfo * new_page);
	void next_slide_first_page_changed (const PageInfo * new_page);
	void next_transition_page_changed (const PageInfo * new_page);
	void previous_transition_page_changed (const PageInfo * new_page);

	void slide_changed (int new_slide_number);
	void time_changed (bool paused, QString new_time_text);
	void annotations_changed (QString new_annotations);

public slots:
	// Page navigation
	void go_to_page_index (int index) {
		if (0 <= index && index < document_.nb_pages () && current_page_ != index) {
			current_page_ = index;
			qDebug () << "current   " << document_.page (current_page_);
			timer_start ();
			update_views ();
		}
	}
	void go_to_next_page (void) { go_to_page_index (current_page_ + 1); }
	void go_to_previous_page (void) { go_to_page_index (current_page_ - 1); }
	void go_to_first_page (void) { go_to_page_index (0); }
	void go_to_last_page (void) { go_to_page_index (document_.nb_pages () - 1); }

	// Timer control
	void timer_start (void) { timer_.start (); }
	void timer_toggle_pause (void) { timer_.toggle_pause (); }
	void timer_reset (void) { timer_.reset (); }

	// Action
	void execute_action (const Action::Base * action) { action->execute (*this); }

	// Full reset, also used for init
	void reset (void) {
		current_page_ = 0;
		update_views ();
		timer_reset ();
	}

private:
	void update_views (void) {
		const auto & page = document_.page (current_page_);
		emit current_page_changed (&page);
		emit next_slide_first_page_changed (page.next_slide_first_page ());
		emit next_transition_page_changed (page.next_transition_page ());
		emit previous_transition_page_changed (page.previous_transition_page ());
		emit slide_changed (page.slide_index ());
		const auto & slide = document_.slide (page.slide_index ());
		emit annotations_changed (slide.annotations () + page.annotations ());
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

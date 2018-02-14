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

class Document;
class PageInfo;
namespace Action {
class Base;
}

#include <QBasicTimer>
#include <QPixmap>
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
	// Periodically fires to indicate timer status (time in text format)
	void update (bool paused, QString time_text);

public slots:
	void start ();
	void toggle_pause ();
	void reset ();

private:
	// Internal primitives
	void emit_update ();
	void timerEvent (QTimerEvent *) Q_DECL_FINAL;
	void start_or_resume_timing ();
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
	explicit Controller (const Document & document);

signals:
	void current_page_changed (const PageInfo * new_page);
	void next_slide_first_page_changed (const PageInfo * new_page);
	void next_transition_page_changed (const PageInfo * new_page);
	void previous_transition_page_changed (const PageInfo * new_page);

	void slide_changed (int new_slide_number);
	void time_changed (bool paused, QString new_time_text);
	void annotations_changed (QString new_annotations);

public slots:
	// Page navigation (no effect if out of bounds)
	void go_to_page_index (int index);
	void go_to_next_page ();
	void go_to_previous_page ();
	void go_to_first_page ();
	void go_to_last_page ();

	// Timer control
	void timer_start () { timer_.start (); }
	void timer_toggle_pause () { timer_.toggle_pause (); }
	void timer_reset () { timer_.reset (); }

	// Action
	void execute_action (const Action::Base * action);

	// Full reset, also used for init
	void reset ();

private:
	// Impl detail, updates all views
	void update_views ();
};

// Sets keyboard shortcuts for the controller in a QWidget.
void add_shortcuts_to_widget (Controller & c, QWidget * widget);

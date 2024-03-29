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

#include <QBasicTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QPixmap>
#include <QTime>
#include <vector>
class QWidget;

class Document;
class PageInfo;
namespace Action {
class Base;
}

/* View role.
 *
 * Used to identify which page a view will show, related to the current page.
 * The current page is the page currently shown to the public.
 * This role is used by the views, and in the renderer system (prefetching strategy).
 */
enum class ViewRole {
	CurrentPublic,
	CurrentPresenter,
	NextSlide,
	NextTransition,
	PrevTransition,
	Unknown
};
QDebug operator<< (QDebug d, ViewRole role);

/* Page to show in the given role for the given current page.
 * nullptr indicates no page.
 */
const PageInfo * page_for_role (const PageInfo * current_page, ViewRole role);

/* Redraw cause.
 *
 * Reason for a render request (what triggered it).
 * Can be a window resize, or a page change from the controller.
 */
enum class RedrawCause { Resize, ForwardMove, BackwardMove, RandomMove, Unknown };
QDebug operator<< (QDebug d, RedrawCause cause);

// Timer which tracks the time spent in the presentation / slide between pauses.
// Accumulates time spent in "spans" between pauses.
class TimeTracker {
	QTime cumulated_spans_{0, 0};
	QElapsedTimer current_span_start_;

public:
	QTime current_duration () const;
	void reset ();
	void start_span ();
	void end_span ();
	void flush_duration_to (QTime & destination);
};

/* Manage a presentation state (which slide/page is currently viewed).
 * Sends signals to indicate changes in timer, current page.
 * Views will decide what to show from the current_page and their selected roles.
 */
class Controller : public QObject {
	Q_OBJECT

private:
	const Document & document_;
	int current_page_{0}; // Main iterator over document

	QBasicTimer timer_; // Generate periodic timerEvent
	TimeTracker presentation_duration_;
	TimeTracker current_slide_duration_;

	// Track timing information by slide, for presentation training
	struct SlideTimingInfo {
		bool reached = false;
		QTime slide_reached_at{0, 0};
		QTime time_spent_in_slide{0, 0};
	};
	std::vector<SlideTimingInfo> timing_by_slide_;
	QWidget & presenter_view_;

public:
	Controller (const Document & document, QWidget & presenter_view);

signals:
	void current_page_changed (const PageInfo * new_current_page, RedrawCause cause);
	void timer_changed (bool paused, QString new_time_text);

public slots:
	// Page navigation (no effect if out of bounds)
	void go_to_page_index (int index);
	void go_to_next_page ();
	void go_to_previous_page ();
	void go_to_first_page ();
	void go_to_last_page ();

	// Timer control
	void timer_toggle_pause ();
	void timer_reset ();
	void output_timing_table ();

	// Action
	void execute_action (const Action::Base * action);

	// Perform a full reset to initialize everything
	void bootstrap ();

private:
	void navigation_change_page (int new_page_index, RedrawCause cause);

	void timerEvent (QTimerEvent *) Q_DECL_FINAL { generate_timer_status_update (); }
	void generate_timer_status_update ();
};

// Sets keyboard shortcuts for the controller in a QWidget.
void add_shortcuts_to_widget (Controller & c, QWidget * widget);

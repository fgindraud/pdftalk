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
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QTextStream>
#include <QtDebug>

#include "action.h"
#include "controller.h"
#include "document.h"

// ViewRole

QDebug operator<< (QDebug d, ViewRole role) {
	auto select_str = [] (ViewRole role) -> const char * {
		switch (role) {
		case ViewRole::CurrentPublic:
			return "CurrentPublic";
		case ViewRole::CurrentPresenter:
			return "CurrentPresenter";
		case ViewRole::NextSlide:
			return "NextSlide";
		case ViewRole::NextTransition:
			return "NextTransition";
		case ViewRole::PrevTransition:
			return "PrevTransition";
		default:
			return "Unknown";
		}
	};
	d << select_str (role);
	return d;
}

const PageInfo * page_for_role (const PageInfo * current_page, ViewRole role) {
	if (current_page == nullptr)
		return nullptr;

	switch (role) {
	case ViewRole::NextSlide: {
		auto * next_slide = current_page->slide ()->next_slide ();
		return next_slide != nullptr ? next_slide->first_page () : nullptr;
	}
	case ViewRole::NextTransition: {
		if (current_page == current_page->slide ()->last_page ()) {
			return nullptr;
		} else {
			return current_page->next_page ();
		}
	}
	case ViewRole::PrevTransition: {
		if (current_page == current_page->slide ()->first_page ()) {
			return nullptr;
		} else {
			return current_page->previous_page ();
		}
	}
	default:
		return current_page;
	}
}

// RedrawCause

QDebug operator<< (QDebug d, RedrawCause cause) {
	auto select_str = [] (RedrawCause cause) -> const char * {
		switch (cause) {
		case RedrawCause::Resize:
			return "Resize";
		case RedrawCause::ForwardMove:
			return "ForwardMove";
		case RedrawCause::BackwardMove:
			return "BackwardMove";
		case RedrawCause::RandomMove:
			return "RandomMove";
		default:
			return "Unknown";
		}
	};
	d << select_str (cause);
	return d;
}

// TimeTracker

QTime TimeTracker::current_duration () const {
	if (current_span_start_.isValid ()) {
		return cumulated_spans_.addMSecs (current_span_start_.elapsed ());
	} else {
		return cumulated_spans_;
	}
}
void TimeTracker::reset () {
	cumulated_spans_ = QTime (0, 0);
	current_span_start_.invalidate ();
}
void TimeTracker::start_span () {
	if (!current_span_start_.isValid ()) {
		current_span_start_.start ();
	}
}
void TimeTracker::end_span () {
	if (current_span_start_.isValid ()) {
		cumulated_spans_ = cumulated_spans_.addMSecs (current_span_start_.elapsed ());
		current_span_start_.invalidate ();
	}
}
void TimeTracker::flush_duration_to (QTime & destination) {
	destination = destination.addMSecs (current_duration ().msecsSinceStartOfDay ());
	cumulated_spans_ = QTime (0, 0);
	if (current_span_start_.isValid ()) {
		current_span_start_.start ();
	}
}

// Controller

Controller::Controller (const Document & document, QWidget & presenter_view)
    : document_ (document),
      timing_by_slide_ (document.nb_slides ()),
      presenter_view_ (presenter_view) {}

void Controller::go_to_page_index (int index) {
	navigation_change_page (index, RedrawCause::RandomMove);
}
void Controller::go_to_next_page () {
	navigation_change_page (current_page_ + 1, RedrawCause::ForwardMove);
}
void Controller::go_to_previous_page () {
	navigation_change_page (current_page_ - 1, RedrawCause::BackwardMove);
}
void Controller::go_to_first_page () {
	go_to_page_index (0);
}
void Controller::go_to_last_page () {
	go_to_page_index (document_.nb_pages () - 1);
}

void Controller::timer_toggle_pause () {
	if (timer_.isActive ()) {
		timer_.stop ();
		presentation_duration_.end_span ();
		current_slide_duration_.end_span ();
		generate_timer_status_update ();
	} else {
		timer_.start (1000, this); // can cause misticks if too small
		presentation_duration_.start_span ();
		current_slide_duration_.start_span ();
		generate_timer_status_update (); // Pause status changed
	}
}
void Controller::timer_reset () {
	timer_.stop ();
	presentation_duration_.reset ();
	current_slide_duration_.reset ();
	generate_timer_status_update ();

	// Reset slide timing info
	for (auto & info : timing_by_slide_) {
		info = {};
	}
	timing_by_slide_[current_page_].reached = true;
}

void Controller::execute_action (const Action::Base * action) {
	action->execute (*this);
}

void Controller::bootstrap () {
	// Does not start timer !
	current_page_ = 0;
	qDebug () << "### reset ###";
	emit current_page_changed (document_.page (current_page_), RedrawCause::RandomMove);
	timer_reset ();
}

void Controller::navigation_change_page (int index, RedrawCause cause) {
	if (0 <= index && index < document_.nb_pages () && current_page_ != index) {
		// Track time
		current_slide_duration_.flush_duration_to (timing_by_slide_[current_page_].time_spent_in_slide);
		auto & index_timings = timing_by_slide_[index];
		if (!index_timings.reached) {
			index_timings.reached = true;
			index_timings.slide_reached_at = presentation_duration_.current_duration ();
		}
		// Change page
		current_page_ = index;
		auto * page = document_.page (current_page_);
		qDebug () << "# current  " << page;
		emit current_page_changed (page, cause);
		// Auto start timers if navigating
		if (!timer_.isActive ()) {
			timer_.start (1000, this);
			presentation_duration_.start_span ();
			current_slide_duration_.start_span ();
			generate_timer_status_update (); // Pause status changed
		}
	}
}

void Controller::generate_timer_status_update () {
	emit timer_changed (!timer_.isActive (),
	                    presentation_duration_.current_duration ().toString (tr ("HH:mm:ss")));
}

void Controller::output_timing_table () {
	current_slide_duration_.flush_duration_to (timing_by_slide_[current_page_].time_spent_in_slide);

	QString filename =
	    QFileDialog::getSaveFileName (&presenter_view_, QObject::tr ("Save slide timings to..."));

	QFile file (filename);
	if (!file.open (QIODevice::WriteOnly | QIODevice::Text)) {
		QString msg = QObject::tr ("Could not write to file : ");
		msg.append (filename);
		QMessageBox::critical (&presenter_view_, QObject::tr ("Error"), msg);
		return;
	}
	QTextStream stream (&file);
	QString time_format = tr ("HH:mm:ss");
	stream << "slide\treached_at\ttime_spent\n";
	for (std::size_t i = 0; i < timing_by_slide_.size (); i += 1) {
		const auto & timing = timing_by_slide_[i];
		if (timing.reached) {
			stream << (i + 1) << "\t" << timing.slide_reached_at.toString (time_format) << "\t"
			       << timing.time_spent_in_slide.toString (time_format) << "\n";
		} else {
			stream << (i + 1) << "\tnever\t" << timing.time_spent_in_slide.toString (time_format) << "\n";
		}
	}
}

// Shortcuts

void add_shortcuts_to_widget (Controller & c, QWidget * widget) {
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
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("Home", "first_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::go_to_first_page);
	}
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("End", "last_page key")), widget);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::go_to_last_page);
	}
	// 'g' for goto page prompt TODO

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
	{
		auto sc = new QShortcut (QKeySequence (QObject::tr ("T", "timing_output key")), widget);
		sc->setAutoRepeat (false);
		QObject::connect (sc, &QShortcut::activated, &c, &Controller::output_timing_table);
	}
}

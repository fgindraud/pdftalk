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
#include "controller.h"
#include "action.h"
#include "document.h"

#include <QShortcut>
#include <QtDebug>

// Timer

void Timing::start () {
	if (!timer_.isActive ())
		start_or_resume_timing ();
}
void Timing::toggle_pause () {
	if (timer_.isActive ()) {
		timer_.stop ();
		accumulated_ = accumulated_.addMSecs (last_resume_.elapsed ());
		emit_update ();
	} else {
		start_or_resume_timing ();
	}
}
void Timing::reset () {
	timer_.stop ();
	accumulated_ = QTime{0, 0, 0};
	emit_update (); // Everything changed
}

void Timing::emit_update () {
	QTime total = accumulated_;
	if (timer_.isActive ())
		total = total.addMSecs (last_resume_.elapsed ());
	emit update (!timer_.isActive (), total.toString (tr ("HH:mm:ss")));
}
void Timing::timerEvent (QTimerEvent *) {
	emit_update ();
}
void Timing::start_or_resume_timing () {
	timer_.start (1000, this); // FIXME can cause misticks if too small...
	last_resume_.start ();
	emit_update (); // Pause status changed
}

// ViewRole

QDebug operator<< (QDebug d, ViewRole role) {
	auto select_str = [](ViewRole role) -> const char * {
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
		return next_slide ? next_slide->first_page () : nullptr;
	}
	case ViewRole::NextTransition:
		return current_page->next_transition_page ();
	case ViewRole::PrevTransition:
		return current_page->previous_transition_page ();
	default:
		return current_page;
	}
}

// RedrawCause

QDebug operator<< (QDebug d, RedrawCause cause) {
	auto select_str = [](RedrawCause cause) -> const char * {
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

// Controller

Controller::Controller (const Document & document) : document_ (document) {
	connect (&timer_, &Timing::update, this, &Controller::time_changed);
}

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

void Controller::execute_action (const Action::Base * action) {
	action->execute (*this);
}

void Controller::reset () {
	// Does not start timer !
	current_page_ = 0;
	qDebug () << "### reset ###";
	update_gui (RedrawCause::RandomMove);
	timer_reset ();
}

void Controller::navigation_change_page (int index, RedrawCause cause) {
	if (0 <= index && index < document_.nb_pages () && current_page_ != index) {
		current_page_ = index;
		qDebug () << "# current  " << document_.page (current_page_);
		update_gui (cause);
		timer_start ();
	}
}

void Controller::update_gui (RedrawCause cause) {
	const auto * page = document_.page (current_page_);
	const auto * slide = page->slide ();
	emit current_page_changed (page, cause);
	emit slide_changed (slide->index ());
	emit annotations_changed (slide->annotations ());
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
}

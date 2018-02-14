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
#include "render.h"

#include <QDebug>
#include <QShortcut>

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

// Controller

Controller::Controller (const Document & document) : document_ (document) {
	connect (&timer_, &Timing::update, this, &Controller::time_changed);
}

void Controller::go_to_page_index (int index) {
	change_page (index, Render::Cause::RandomMove);
}
void Controller::go_to_next_page () {
	change_page (current_page_ + 1, Render::Cause::ForwardMove);
}
void Controller::go_to_previous_page () {
	change_page (current_page_ - 1, Render::Cause::BackwardMove);
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
	update_views (Render::Cause::RandomMove);
	timer_reset ();
}

void Controller::update_views (Render::Cause cause) {
	const auto & page = document_.page (current_page_);
	emit current_page_changed (&page, cause);
	emit next_slide_first_page_changed (page.next_slide_first_page (), cause);
	emit next_transition_page_changed (page.next_transition_page (), cause);
	emit previous_transition_page_changed (page.previous_transition_page (), cause);
	emit slide_changed (page.slide_index ());
	const auto & slide = document_.slide (page.slide_index ());
	emit annotations_changed (slide.annotations ());
}

void Controller::change_page (int index, Render::Cause cause) {
	if (0 <= index && index < document_.nb_pages () && current_page_ != index) {
		current_page_ = index;
		qDebug () << "# current  " << document_.page (current_page_);
		timer_start ();
		update_views (cause);
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

/* Copyright (C) 2016 Francois Gindraud
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
#include "action.h"
#include "controller.h"

#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

namespace Action {
void Quit::execute (Controller &) const {
	QApplication::quit ();
}
void Browser::execute (Controller &) const {
	QDesktopServices::openUrl (QUrl (url_));
}
void PageNext::execute (Controller & controller) const {
	controller.go_to_next_page ();
}
void PagePrevious::execute (Controller & controller) const {
	controller.go_to_previous_page ();
}
void PageFirst::execute (Controller & controller) const {
	controller.go_to_first_page ();
}
void PageLast::execute (Controller & controller) const {
	controller.go_to_last_page ();
}
void PageIndex::execute (Controller & controller) const {
	controller.go_to_page_index (index_);
}
}

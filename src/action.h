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
#ifndef ACTION_H
#define ACTION_H

#include <QRect>
#include <QString>

class Controller;

namespace Action {
// TODO add Goto, PageFirst, PageLast navigation (controller, Action, init_actions)

class Base {
private:
	QRectF rect_; // In relative [0,1] coordinates
public:
	virtual ~Base () = default;
	virtual void execute (Controller & controller) const = 0;

	void set_rect (const QRectF & rect) { rect_ = rect; }
	bool activated (const QPointF & point) const { return rect_.contains (point); }
};

class Quit : public Base {
public:
	void execute (Controller &) const Q_DECL_OVERRIDE;
};

class Browser : public Base {
private:
	QString url_;

public:
	Browser (const QString & url) : url_ (url) {}
	void execute (Controller &) const Q_DECL_OVERRIDE;
};

// Navigation
class PageNext : public Base {
public:
	void execute (Controller & controller) const Q_DECL_OVERRIDE;
};
class PagePrevious : public Base {
public:
	void execute (Controller & controller) const Q_DECL_OVERRIDE;
};
}

#endif

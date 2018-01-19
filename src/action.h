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

#include <QRect>
#include <QString>
class Controller;

namespace Action {

class Base {
	/* All clickable actions derive from Base.
	 * Base provides a common rect field to identify the clickable rectangle.
	 * Sub-classes can then overload execute, which is called on a click.
	 *
	 * Actions are extracted from the document in document.cpp.
	 */
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
class PageFirst : public Base {
public:
	void execute (Controller & controller) const Q_DECL_OVERRIDE;
};
class PageLast : public Base {
public:
	void execute (Controller & controller) const Q_DECL_OVERRIDE;
};
class PageIndex : public Base {
private:
	int index_; // [0, nb_pages[

public:
	PageIndex (int index) : index_ (index) {}
	void execute (Controller & controller) const Q_DECL_OVERRIDE;
};
} // namespace Action

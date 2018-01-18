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
#ifndef RENDER_H
#define RENDER_H

#include "document.h"

#include <QHash>
#include <QPixmap>
#include <QSize>

namespace Render {

// Represent a render request (page + size)
struct Request {
	const PageInfo * page;
	QSize size;
};
inline bool operator== (const Request & a, const Request & b) {
	return a.page == b.page && a.size == b.size;
}
inline uint qHash (const Request & r, uint seed = 0) {
	using ::qHash; // Have access to Qt's basic qHash
	return qHash (r.page, seed) ^ qHash (r.size.width (), seed) ^ qHash (r.size.height (), seed);
}

class SystemPrivate;

class System : public QObject {
	/* Global rendering system.
	 * Classes (viewers) can request a render by signaling request_render().
	 * After some time, new_render will return the requested pixmap.
	 * QObject * requester and the request struct let viewers identify their answers.
	 *
	 * Internally, the cost of rendering is reduced by caching (see render_internal.h).
	 * Additionally, the pages next to the current one are pre-rendered.
	 * cache_size_bytes sets the size of the cache in bytes.
	 * prefetch_window controls the pre-rendering window (1 = next, ...)
	 */
	Q_OBJECT

private:
	SystemPrivate * d_;

public:
	System (int cache_size_bytes, int prefetch_window);

signals:
	void new_render (const QObject * requester, const Request & request, QPixmap render);

public slots:
	void request_render (const Request & request);
};
}

Q_DECLARE_METATYPE (Render::Request);

#endif

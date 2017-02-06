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
	 * cache_size_bytes sets the size of the cache in bytes.
	 */
	Q_OBJECT

private:
	SystemPrivate * d_;

public:
	System (int cache_size_bytes);

signals:
	void new_render (const QObject * requester, const Request & request, QPixmap render);

public slots:
	void request_render (const Request & request);
};

// To let Qt know about some internal types used.
void register_metatypes (void);
}

Q_DECLARE_METATYPE (Render::Request);

#endif

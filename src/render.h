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

#include "document.h"

#include <QDebug>
#include <QHash>
#include <QPixmap>
#include <QSize>

namespace Render {

/* Info represent a render metadata.
 * It is composed of a render size, and the selected page.
 *
 * The Info constructor accept any size: it will be shrunk to the biggest fitting render size.
 * Info is comparable / hashable to enable use as a hash table key (render system cache).
 */
class Info {
private:
	const PageInfo * page_{nullptr};
	QSize size_{};

public:
	Info () = default;
	Info (const PageInfo * p, const QSize & box);

	const PageInfo * page () const noexcept { return page_; }
	const QSize & size () const noexcept { return size_; }
	bool isNull () const noexcept { return page () == nullptr; } // Undef render
};
bool operator== (const Info & a, const Info & b);
uint qHash (const Info & info, uint seed = 0);
QDebug operator<< (QDebug d, const Info & render_info);

// Represent a render request (page + size)
struct Request : public Info {
	using Info::Info; // FIXME Temporary
	Request (const Info & i) : Info (i) {}
};

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
	void new_render (const QObject * requester, const Info & render_info, QPixmap render_data);

public slots:
	void request_render (const Request & request);
};
} // namespace Render

Q_DECLARE_METATYPE (Render::Info);
Q_DECLARE_METATYPE (Render::Request);

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

#include "render.h"

#include <QByteArray>
#include <QCache>
#include <QImage>
#include <QPixmap>
#include <QRunnable>
#include <QSet>
#include <utility>

/* Internal header of the rendering system.
 *
 * My analysis of PDFpc rendering strategy:
 * PDFpc prerenders pages and then compress the bitmap data.
 * When displaying, it thus needs to uncompress the data and recreate a "pixmap" to display.
 * No idea which pixmap caching is done, or how pixmap are resized for the different uses.
 *
 * In PDFTalk, window sizes are expected to change from program launch to presentation running.
 * No total prerendering is done.
 * Instead we use a LRU cache (bounded by a memory usage) of renders (indexed by page x size).
 * Rendering is done on demand (when pages are requested).
 * When a page is rendered (QImage), we store a qCompressed version in the cache (Compressed).
 * Page requests are fulfilled from the Compressed if available, or from a render.
 *
 * Pre-rendering TODO
 */
namespace Render {

// Stores data for a Compressed render
struct Compressed {
	QByteArray data;
	QSize size;
	int bytes_per_line;
	QImage::Format image_format;
};

/* Renders the page at the selected size.
 * Returns both the pixmap and a Compressed version.
 * The pixmap can be given to the requesting view.
 * The Compressed version can be stored in the render cache.
 *
 * Compressed renders are transmitted as owning raw pointers.
 * Signals cannot handle unique_ptr<Compressed> (move only unsupported).
 * And QCache requires an 'operator new' allocated object.
 */
std::pair<Compressed *, QPixmap> make_render (const Info & render_info);

/* Recreate a pixmap from a Compressed render.
 */
QPixmap make_pixmap_from_compressed_render (const Compressed & render);

class Task : public QObject, public QRunnable {
	// "Render a page" task for QThreadPool.
	Q_OBJECT

private:
	const QObject * requester_;
	const Info render_info_;

public:
	Task (const QObject * requester, const Info & render_info)
	    : requester_ (requester), render_info_ (render_info) {}

signals:
	// "Render::Info" as Qt is not very namespace friendly
	void finished_rendering (const QObject * requester, Render::Info render_info,
	                         Compressed * compressed, QPixmap pixmap);

public:
	void run () Q_DECL_OVERRIDE {
		auto result = make_render (render_info_);
		emit finished_rendering (requester_, render_info_, result.first, result.second);
	}
};

class SystemPrivate : public QObject {
	/* Caching system (internals).
	 * Stores compressed renders in a cache to avoid rerendering stuff later.
	 * Rendering is done through Tasks in a QThreadPool.
	 *
	 *
	 * being_rendered tracks ongoing renders, preventing double rendering.
	 */
	Q_OBJECT

private:
	System * parent_;
	QCache<Info, Compressed> cache_;
	QSet<Info> being_rendered_;
	const int prefetch_window_;

public:
	SystemPrivate (int cache_size_bytes, int prefetch_window, System * parent)
	    : QObject (parent),
	      parent_ (parent),
	      cache_ (cache_size_bytes),
	      prefetch_window_ (prefetch_window) {}

	void request_render (const QObject * requester, const Request & request);

private slots:
	// "Render::Info" as Qt is not very namespace friendly
	void rendering_finished (const QObject * requester, Render::Info render_info,
	                         Compressed * compressed, QPixmap pixmap);

private:
	void launch_render (const QObject * requester, const Info & render_info);
};
} // namespace Render

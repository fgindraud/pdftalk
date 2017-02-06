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
#ifndef RENDER_INTERNAL_H
#define RENDER_INTERNAL_H

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

// Rendering, Compressing / Uncompressing primitives
std::pair<Compressed *, QPixmap> make_render (const Request & request);
QPixmap make_pixmap_from_compressed_render (const Compressed & render);

class Task : public QObject, public QRunnable {
	// "Render a page" task for QThreadPool.
	Q_OBJECT

private:
	const QObject * requester_;
	const Request request_;

public:
	Task (const QObject * requester, const Request & request)
	    : requester_ (requester), request_ (request) {}

signals:
	// "Render::Request" as Qt is not very namespace friendly
	void finished_rendering (const QObject * requester, Render::Request request,
	                         Compressed * compressed, QPixmap pixmap);

public:
	void run (void) Q_DECL_OVERRIDE {
		auto result = make_render (request_);
		emit finished_rendering (requester_, request_, result.first, result.second);
	}
};

class SystemPrivate : public QObject {
	/* Caching system (internals).
	 * Stores compressed renders in a cache to avoid rerendering stuff later.
	 * Rendering is done through Tasks in a QThreadPool.
	 * 
	 * being_rendered tracks ongoing renders, preventing double rendering.
	 */
	Q_OBJECT

private:
	System * parent_;
	QCache<Request, Compressed> cache;
	QSet<Request> being_rendered;
	int prefetch_window_;

public:
	SystemPrivate (int cache_size_bytes, int prefetch_window, System * parent)
	    : QObject (parent),
	      parent_ (parent),
	      cache (cache_size_bytes),
	      prefetch_window_ (prefetch_window) {}

	void request_render (const QObject * requester, const Request & request);

private slots:
	// "Render::Request" as Qt is not very namespace friendly
	void rendering_finished (const QObject * requester, Render::Request request,
	                         Compressed * compressed, QPixmap pixmap);
};
}

#endif

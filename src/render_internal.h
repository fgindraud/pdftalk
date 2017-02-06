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
#include <utility>

/* Internal header of the rendering system.
 *
 * My analysis of PDFpc rendering strategy:
 * PDFpc prerenders pages and then compress the bitmap data.
 * When displaying, it thus needs to uncompress the data and recreate a "pixmap" to display.
 * No idea which pixmap caching is done, or how pixmap are resized for the different uses.
 *
 * My strategy is a more basic caching, as window sizes are expected to change.
 * TODO explanations
 * Renders (QImages) are stored as compressed data (metadata + qCompressed' QByteArray of data).
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

/*
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
	void finished_rendering (const QObject * requester, Request request, Compressed compressed,
	                         QPixmap pixmap);

public:
	void run (void) Q_DECL_OVERRIDE {
		auto result = make_render (request_);
		emit finished_rendering (requester_, request_, result.first, result.second);
	}
};*/

class SystemPrivate : public QObject {
	/* Caching system (internals).
	 * Stores compressed renders in a cache to avoid rerendering stuff later.
	 */
	Q_OBJECT

private:
	System * parent_;
	QCache<Request, Compressed> cache;

public:
	SystemPrivate (int cache_size_bytes, System * parent)
	    : QObject (parent), parent_ (parent), cache (cache_size_bytes) {}

	void request_render (const QObject * requester, const Request & request);
};
}
Q_DECLARE_METATYPE (Render::Compressed);

#endif

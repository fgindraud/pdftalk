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
#include "render.h"
#include "render_internal.h"

#include <QDebug>
#include <QMetaType>
#include <QThreadPool>

namespace Render {
// Rendering, Compressing / Uncompressing primitives

std::pair<Compressed *, QPixmap> make_render (const Request & request) {
	// Renders, and returns both the pixmap and the compressed image
	QImage image = request.page->render (request.size);
	auto compressed_data = qCompress (image.bits (), image.byteCount ());
	auto compressed_render =
	    new Compressed{compressed_data, image.size (), image.bytesPerLine (), image.format ()};
	return {compressed_render, QPixmap::fromImage (std::move (image))};
}

static void qbytearray_deleter (void * p) {
	delete static_cast<QByteArray *> (p);
}
QPixmap make_pixmap_from_compressed_render (const Compressed & render) {
	// Recreate an image and then a pixmap from compressed data
	// Try to avoid any useless copy by using the non-owning QImage constructor
	auto uncompressed_data = new QByteArray;
	*uncompressed_data = qUncompress (render.data);
	QImage image (reinterpret_cast<uchar *> (uncompressed_data->data ()), render.size.width (),
	              render.size.height (), render.bytes_per_line, render.image_format,
	              &qbytearray_deleter, uncompressed_data);
	return QPixmap::fromImage (std::move (image));
}

// System impl

System::System (int cache_size_bytes, int prefetch_window)
    : d_ (new SystemPrivate (cache_size_bytes, prefetch_window, this)) {
	qRegisterMetaType<Request> (); // Request registration (once before use in connect)
}

void System::request_render (const Request & request) {
	d_->request_render (sender (), request);
}

void SystemPrivate::request_render (const QObject * requester, const Request & request) {
	// Handle request
	Compressed * compressed_render = cache_.object (request);
	if (compressed_render != nullptr) {
		// Serve request from cache
		qDebug () << "from_cache" << *request.page << request.size;
		emit parent_->new_render (requester, request,
		                          make_pixmap_from_compressed_render (*compressed_render));
	} else {
		// Make new render (spawn a Task in a QThreadPool)
		launch_render (requester, request);
	}
	/* Prefetch pages around the request.
	 * We only need to launch renders, no need to give an answer as there is no request.
	 */
	// FIXME disable prefetch, look at weird renders requests when going backwards... (show page/role
	// in renders)
	int window_remaining = prefetch_window_;
	const PageInfo * forward = request.page->next_transition_page ();
	while (window_remaining > 0 && forward != nullptr) {
		qDebug () << "prefetch" << *forward << request.size;
		Request prefetch{forward, request.size};
		if (!cache_.contains (prefetch))
			launch_render (requester, prefetch);
		forward = forward->next_transition_page ();
		window_remaining--;
	}
}

void SystemPrivate::rendering_finished (const QObject * requester, Request request,
                                        Compressed * compressed, QPixmap pixmap) {
	// When rendering has finished: store compressed, untrack, give pixmap
	cache_.insert (request, compressed, compressed->data.size ());
	being_rendered_.remove (request);
	emit parent_->new_render (requester, request, pixmap);
}

void SystemPrivate::launch_render (const QObject * requester, const Request & request) {
	// If not tracked: track, schedule render
	if (!being_rendered_.contains (request)) {
		qDebug () << "render" << *request.page << request.size;
		being_rendered_.insert (request);
		auto task = new Task (requester, request);
		connect (task, &Task::finished_rendering, this, &SystemPrivate::rendering_finished);
		QThreadPool::globalInstance ()->start (task);
	}
}
} // namespace Render

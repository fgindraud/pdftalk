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
#include "document.h"
#include "render_internal.h"

#include <QDebug>
#include <QHash>
#include <QMetaType>
#include <QThreadPool>

namespace Render {
// Render Info

Info::Info (const PageInfo * p, const QSize & box) : page_ (p) {
	if (p != nullptr)
		size_ = p->render_size (box);
}

bool operator== (const Info & a, const Info & b) {
	return a.page () == b.page () && a.size () == b.size ();
}
uint qHash (const Info & info, uint seed) {
	using ::qHash; // Have access to Qt's basic qHash
	return qHash (info.page (), seed) ^ qHash (info.size ().width (), seed) ^
	       qHash (info.size ().height (), seed);
}

QDebug operator<< (QDebug d, const Info & render_info) {
	if (!render_info.isNull ()) {
		d << *render_info.page () << render_info.size ();
	} else {
		d << "Render::Info()";
	}
	return d;
}

// Render Role

QDebug operator<< (QDebug d, const Role & role) {
	auto select_str = [](const Role & role) -> const char * {
		switch (role) {
		case Role::CurrentPublic:
			return "CurrentPublic";
		case Role::CurrentPresenter:
			return "CurrentPresenter";
		case Role::NextSlide:
			return "NextSlide";
		case Role::Transition:
			return "Transition";
		default:
			return "NoRole";
		}
	};
	d << select_str (role);
	return d;
}

// Render Request

Request::Request (const Info & info, const QSize & box, const Role & role)
    : Info (info), box_size_ (box), role_ (role) {
	Q_ASSERT (!info.isNull ());
	Q_ASSERT (info.size () == info.page ()->render_size (box));
}

// Rendering, Compressing / Uncompressing primitives

std::pair<Compressed *, QPixmap> make_render (const Info & render_info) {
	// Renders, and returns both the pixmap and the compressed image
	QImage image = render_info.page ()->render (render_info.size ());
	auto compressed_data = qCompress (image.bits (), image.byteCount ());
	auto * compressed_render =
	    new Compressed{compressed_data, image.size (), image.bytesPerLine (), image.format ()};
	return {compressed_render, QPixmap::fromImage (std::move (image))};
}

static void qbytearray_deleter (void * p) {
	delete static_cast<QByteArray *> (p);
}
QPixmap make_pixmap_from_compressed_render (const Compressed & render) {
	// Recreate an image and then a pixmap from compressed data
	// Try to avoid any useless copy by using the non-owning QImage constructor
	auto * uncompressed_data = new QByteArray;
	*uncompressed_data = qUncompress (render.data);
	QImage image (reinterpret_cast<uchar *> (uncompressed_data->data ()), render.size.width (),
	              render.size.height (), render.bytes_per_line, render.image_format,
	              &qbytearray_deleter, uncompressed_data);
	return QPixmap::fromImage (std::move (image));
}

// System impl

System::System (int cache_size_bytes, int prefetch_window)
    : d_ (new SystemPrivate (cache_size_bytes, prefetch_window, this)) {
	// Request registration (once before use in connect)
	qRegisterMetaType<Info> ();
	qRegisterMetaType<Request> ();
}

void System::request_render (const Request & request) {
	d_->request_render (sender (), request);
}

void SystemPrivate::request_render (const QObject * requester, const Request & request) {
	qDebug () << "request    " << request << request.role ();

	// Handle request
	const Compressed * compressed_render = cache_.object (request);
	if (compressed_render != nullptr) {
		// Serve request from cache
		qDebug () << "-> cached  " << request;
		emit parent_->new_render (requester, request,
		                          make_pixmap_from_compressed_render (*compressed_render));
	} else {
		// Make new render (spawn a Task in a QThreadPool)
		launch_render (requester, request);
	}
	/* Prefetch pages around the request.
	 * We only need to launch renders, no need to give an answer as there is no request.
	 * FIXME fix, improve, use role and change context to decide prefetching
	 * We have the box (Request, not just Info) to pre render the next slides.
	 */
	/*int window_remaining = prefetch_window_;
	const PageInfo * forward = request.page->next_page ();
	while (window_remaining > 0 && forward != nullptr) {
	  qDebug () << "prefetch  " << *forward << request.size;
	  Request prefetch{forward, request.size};
	  if (!cache_.contains (prefetch))
	    launch_render (requester, prefetch);
	  forward = forward->next_page ();
	  window_remaining--;
	}*/
}

void SystemPrivate::rendering_finished (const QObject * requester, Info render_info,
                                        Compressed * compressed, QPixmap pixmap) {
	// When rendering has finished: store compressed, untrack, give pixmap
	cache_.insert (render_info, compressed, compressed->data.size ());
	being_rendered_.remove (render_info);
	emit parent_->new_render (requester, render_info, pixmap);
}

void SystemPrivate::launch_render (const QObject * requester, const Info & render_info) {
	// If not tracked: track, schedule render
	if (!being_rendered_.contains (render_info)) {
		qDebug () << "-> render  " << render_info;
		being_rendered_.insert (render_info);
		auto * task = new Task (requester, render_info);
		connect (task, &Task::finished_rendering, this, &SystemPrivate::rendering_finished);
		QThreadPool::globalInstance ()->start (task);
	}
}
} // namespace Render

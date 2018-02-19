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

#include <QCoreApplication>
#include <QHash>
#include <QLocale>
#include <QMetaType>
#include <QThreadPool>
#include <QtDebug>

// Byte size conversion

QString size_in_bytes_to_string (int size) {
	qreal num = size;
	qreal increment = 1024.0;
	static const char * suffixes[] = {QT_TR_NOOP ("B"),
	                                  QT_TR_NOOP ("KiB"),
	                                  QT_TR_NOOP ("MiB"),
	                                  QT_TR_NOOP ("GiB"),
	                                  QT_TR_NOOP ("TiB"),
	                                  QT_TR_NOOP ("PiB"),
	                                  nullptr};
	int unit_idx = 0;
	while (num >= increment && suffixes[unit_idx + 1] != nullptr) {
		unit_idx++;
		num /= increment;
	}
	return QLocale ().toString (num, 'f', 2) +
	       qApp->translate ("byte_size_conversion", suffixes[unit_idx]);
}
int string_to_size_in_bytes (QString size_str) {
	int factor = 1;

	// Remove suffix if present. Order in the array is important, first match wins.
	struct SuffixWithFactor {
		const char * suffix;
		int factor;
	};
	static const SuffixWithFactor suffixes[] = {
	    {QT_TR_NOOP ("G"), 1000000000}, {QT_TR_NOOP ("GB"), 1000000000},
	    {QT_TR_NOOP ("GiB"), 1 << 30},  {QT_TR_NOOP ("M"), 1000000},
	    {QT_TR_NOOP ("MB"), 1000000},   {QT_TR_NOOP ("MiB"), 1 << 20},
	    {QT_TR_NOOP ("K"), 1000},       {QT_TR_NOOP ("KB"), 1000},
	    {QT_TR_NOOP ("KiB"), 1 << 10},  {QT_TR_NOOP ("B"), 1},
	};
	for (const auto & s : suffixes) {
		auto suffix = qApp->translate ("byte_size_conversion", s.suffix);
		if (size_str.endsWith (suffix, Qt::CaseInsensitive)) {
			factor = s.factor;
			size_str.chop (suffix.size ());
			break;
		}
	}

	// Parse value (fails if there is anything but the number)
	bool ok;
	auto raw_value = QLocale ().toDouble (size_str, &ok);
	if (ok) {
		return static_cast<int> (raw_value * factor);
	} else {
		return -1;
	}
}

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
		d << render_info.page () << render_info.size ();
	} else {
		d << "Render::Info()";
	}
	return d;
}

// Render Request

Request::Request (const Info & info, const QSize & box, ViewRole role, RedrawCause cause)
    : Info (info), box_size_ (box), role_ (role), cause_ (cause) {
	Q_ASSERT (!info.isNull ());
	Q_ASSERT (info.size () == info.page ()->render_size (box));
	Q_ASSERT (role != ViewRole::Unknown);
	Q_ASSERT (cause != RedrawCause::Unknown);
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
    : d_ (new SystemPrivate (cache_size_bytes, prefetch_window, this)) {}

void System::request_render (const Request & request) {
	d_->request_render (request);
}

SystemPrivate::~SystemPrivate () {
	qDebug () << QString ("Render cache: used %1 out of %2")
	                 .arg (size_in_bytes_to_string (cache_.totalCost ()),
	                       size_in_bytes_to_string (cache_.maxCost ()));
}

void SystemPrivate::request_render (const Request & request) {
	qDebug () << "request    " << request << request.role () << request.cause ();

	// Handle request
	const Compressed * compressed_render = cache_.object (request);
	if (compressed_render != nullptr) {
		// Serve request from cache
		qDebug () << "-> cached  " << request;
		emit parent_->new_render (request, make_pixmap_from_compressed_render (*compressed_render));
	} else {
		// Make new render (spawn a Task in a QThreadPool)
		launch_render (request);
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

void SystemPrivate::rendering_finished (Info render_info, Compressed * compressed, QPixmap pixmap) {
	// When rendering has finished: store compressed, untrack, give pixmap
	cache_.insert (render_info, compressed, compressed->data.size ());
	being_rendered_.remove (render_info);
	emit parent_->new_render (render_info, pixmap);
}

void SystemPrivate::launch_render (const Info & render_info) {
	// If not tracked: track, schedule render
	if (!being_rendered_.contains (render_info)) {
		qDebug () << "-> render  " << render_info;
		being_rendered_.insert (render_info);
		auto * task = new Task (render_info);
		connect (task, &Task::finished_rendering, this, &SystemPrivate::rendering_finished);
		QThreadPool::globalInstance ()->start (task);
	}
}
} // namespace Render

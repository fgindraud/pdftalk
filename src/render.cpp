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
bool operator!= (const Info & a, const Info & b) {
	return !(a == b);
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

Request::Request (const PageInfo * current_page, const QSize & box, ViewRole role,
                  RedrawCause cause)
    : current_page_ (current_page), box_size_ (box), role_ (role), cause_ (cause) {
	Q_ASSERT (role != ViewRole::Unknown);
	Q_ASSERT (cause != RedrawCause::Unknown);
}

// PrefetchStrategy

PrefetchStrategy::PrefetchStrategy (const QString & name) : name_ (name) {}

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

System::System (int cache_size_bytes, PrefetchStrategy * strategy)
    : d_ (new SystemPrivate (cache_size_bytes, strategy, this)) {}

void System::request_render (const Request & request) {
	d_->request_render (request);
}

SystemPrivate::SystemPrivate (int cache_size_bytes, PrefetchStrategy * strategy, System * parent)
    : QObject (parent),
      parent_ (parent),
      cache_ (cache_size_bytes),
      prefetch_strategy_ (strategy),
      prefetch_render_lambda_ ([this](const Info & render_info) {
	      qDebug () << "prefetch   " << render_info;
	      this->perform_render (render_info, RenderType::Prefetch);
      }) {}

SystemPrivate::~SystemPrivate () {
	qDebug () << QString ("Render cache: used %1 out of %2")
	                 .arg (size_in_bytes_to_string (cache_.totalCost ()),
	                       size_in_bytes_to_string (cache_.maxCost ()));
}

void SystemPrivate::request_render (const Request & request) {
	auto current_render = request.requested_render ();
	qDebug () << "request    " << current_render << request.role () << request.cause ();
	perform_render (current_render, RenderType::Requested);
	if (prefetch_strategy_ != nullptr) {
		prefetch_strategy_->prefetch (request, prefetch_render_lambda_);
	}
}

void SystemPrivate::rendering_finished (Info render_info, Compressed * compressed, QPixmap pixmap) {
	// When rendering has finished: store compressed, untrack, give pixmap only if the render was
	// requested.
	cache_.insert (render_info, compressed, compressed->data.size ());
	Q_ASSERT (being_rendered_.contains (render_info));
	auto type = being_rendered_.take (render_info);
	if (type == RenderType::Requested) {
		emit parent_->new_render (render_info, pixmap);
	}
}

void SystemPrivate::perform_render (const Info & render_info, RenderType type) {
	// Ignore bad renders (null, too small).
	static constexpr int pixmap_size_limit_px = 10;
	if (render_info.isNull () || render_info.size ().width () < pixmap_size_limit_px ||
	    render_info.size ().height () < pixmap_size_limit_px) {
		qDebug () << "-> ignored " << render_info;
		return;
	}

	// Take the render from the cache is present.
	const Compressed * compressed_render = cache_.object (render_info);
	if (compressed_render != nullptr) {
		qDebug () << "-> cached  " << render_info;
		// Only serve if actually requested
		if (type == RenderType::Requested) {
			emit parent_->new_render (render_info,
			                          make_pixmap_from_compressed_render (*compressed_render));
		}
		return;
	}

	// If a similar render is running, do nothing: it will answer the request for us.
	auto it = being_rendered_.find (render_info);
	if (it != being_rendered_.end ()) {
		qDebug () << "-> running " << render_info;
		// Mark the render as requested now, if it was only a prefetch render.
		if (type == RenderType::Requested) {
			it.value () = RenderType::Requested;
		}
		return;
	}

	// No render running, launch our own
	qDebug () << "-> launch  " << render_info;
	being_rendered_.insert (render_info, type);
	auto * task = new Task (render_info);
	connect (task, &Task::finished_rendering, this, &SystemPrivate::rendering_finished);
	QThreadPool::globalInstance ()->start (task);
}
} // namespace Render

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
#include "render.h"
#include "render_internal.h"

#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QThreadPool>
#include <vector>

#include <QDebug>

namespace Render {
// Rendering, Compressing / Uncompressing primitives

std::pair<Compressed, QPixmap> make_render (const Request & request) {
	// Renders, and returns both the pixmap and the compressed image
	QImage image = request.page->render (request.size);
	auto compressed_data = qCompress (image.bits (), image.byteCount ());
	auto compressed_render =
	    Compressed{compressed_data, image.size (), image.bytesPerLine (), image.format ()};
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

// Cache impl

void Cache::request_render (Request request) {
	const QObject * requester = sender ();
	auto p = QPixmap::fromImage (request.page->render (request.size));
	emit new_render (requester, request, p);
}

void register_metatypes (void) {
	qRegisterMetaType<Request> ();
	qRegisterMetaType<Compressed> ();
}
}

#if 0
class RenderCache : public QObject {
	/* Stores CompressedRender objects, and uncompress them as needed.
	 */
	Q_OBJECT

private:
	const Document & document_;
	std::vector<std::vector<CompressedRender>> renders_by_page_;

public:
	explicit RenderCache (const Document & document)
	    : document_ (document), renders_by_page_ (document.nb_pages ()) {}

signals:
	void new_pixmap (const QObject * requester, int page_index, QPixmap pixmap);

public slots:
	void request_page (const QObject * requester, int page_index, QSize box) {
		auto render_size = document_.page (page_index).render_size (box);
		if (render_size.isEmpty ()) {
			// Early return if invalid size
			emit new_pixmap (requester, page_index, QPixmap ());
			return;
		}
		qDebug () << "Requested" << page_index << render_size;
		// Determine cached render ith the closest size ratio to our request
		const CompressedRender * best = nullptr;
		qreal smallest_size_diff_ratio = 1.0;
		for (const auto & compressed_render : renders_by_page_.at (page_index)) {
			qreal size_diff_ratio = qAbs (static_cast<qreal> (compressed_render.size.width ()) /
			                                  static_cast<qreal> (render_size.width ()) -
			                              1.0);
			if (size_diff_ratio < smallest_size_diff_ratio) {
				best = &compressed_render;
				smallest_size_diff_ratio = size_diff_ratio;
			}
			qDebug () << "Option" << compressed_render.size << size_diff_ratio;
		}
		/* If a perfect size render is found, return it.
		 * If a good enough size (less than 20% difference) is found, return it and launch a render.
		 * If no size is good, launch a render.
		 */
		if (best == nullptr || best->size != render_size) {
			qDebug () << "Start rendering" << render_size;
			// Launch render
			auto task = new RenderTask (requester, document_, page_index, box);
			connect (task, &RenderTask::finished_rendering, this, &RenderCache::render_finished);
			QThreadPool::globalInstance ()->start (task);
		}
		if (best != nullptr && smallest_size_diff_ratio < 0.2) {
			qDebug () << "Selected" << best->size << smallest_size_diff_ratio;
			// Return saved
			emit new_pixmap (requester, page_index, make_pixmap_from_compressed_render (*best));
		}
	}

private slots:
	void render_finished (const QObject * requester, int page_index,
	                      CompressedRender compressed_render, QPixmap pixmap) {
		renders_by_page_.at (page_index).push_back (compressed_render);
		emit new_pixmap (requester, page_index, pixmap);
	}
};
#endif

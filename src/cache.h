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
#ifndef CACHE_H
#define CACHE_H

#include "document.h"

#include <QByteArray>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <utility>
#include <vector>

#include <QDebug>

/* My analysis of PDFpc rendering strategy:
 * PDFpc prerenders pages and then compress the bitmap data.
 * When displaying, it thus needs to uncompress the data and recreate a "pixmap" to display.
 * No idea which pixmap caching is done, or how pixmap are resized for the different uses.
 *
 * My strategy is similar:
 * Renders (QImages) are stored as compressed data (metadata + qCompressed' QByteArray of data).
 */
struct CompressedRender {
	// Compressed render info
	QByteArray data;
	QSize size;
	int bytes_per_line;
	QImage::Format image_format;
};

inline std::pair<CompressedRender, QImage> make_render (const Document & document, int page_index,
                                                        const QSize & box) {
	QImage image = document.render (page_index, box);
	auto compressed_data = qCompress (image.bits (), image.byteCount ());
	return {CompressedRender{compressed_data, image.size (), image.bytesPerLine (), image.format ()},
	        image};
}

inline void qbytearray_deleter (void * p) {
	qDebug () << "Deleting QByteArray" << p;
	delete static_cast<QByteArray *> (p);
}
inline QPixmap make_pixmap_from_compressed_render (const CompressedRender & render) {
	// Recreate an image and then a pixmap from compressed data
	// Try to avoid any useless copy by using the non-owning QImage constructor
	auto uncompressed_data = new QByteArray;
	*uncompressed_data = qUncompress (render.data);
	QImage image (reinterpret_cast<uchar *> (uncompressed_data->data ()), render.size.width (),
	              render.size.height (), render.bytes_per_line, render.image_format,
	              &qbytearray_deleter, uncompressed_data);
	return QPixmap::fromImage (std::move (image));
}

class RenderCache : public QObject {
	/* Stores CompressedRender objects, and uncompress them as needed.
	 */
	Q_OBJECT

private:
	const Document & document_;
	std::vector<int> renders_by_page_;

public:
	explicit RenderCache (const Document & document)
	    : document_ (document), renders_by_page_ (document.nb_pages ()) {}
};

#endif

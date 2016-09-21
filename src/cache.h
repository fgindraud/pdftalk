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

#include <QDebug>
// Experimental for now
// pdfpc prerenders every page, and compresses the resulting images
// no idea of the rendering size, or which compression system is used
// QT stuff available:
// rendering gives a QImage, which is a userspace buffer
// qCompress can take it and return a QBytearray
// I can recreate a QImage from it (and then a QPixmap...)
// Might be a better strategy than keeping QPixmaps

struct CompressedRender {
	// Compressed render info
	const QByteArray data;
	const QSize size;
	const int bytes_per_line;
	const QImage::Format image_format;
};

inline CompressedRender make_render (const Document & document, int page_nb,
	                                            const QSize & box) {
	// TODO
}

inline void qbytearray_deleter (void * p) {
	qDebug () << "Deleting QByteArray" << p;
	delete static_cast<QByteArray *> (p);
}

inline QPixmap pixmap_from_compressed_render (const CompressedRender & render) {
	auto uncompressed_data = new QByteArray;
	*uncompressed_data = qUncompress (render.data);
	QImage image (reinterpret_cast<uchar *> (uncompressed_data->data ()), render.size.width (),
	              render.size.height (), render.bytes_per_line, render.image_format,
	              &qbytearray_deleter, uncompressed_data);
	return QPixmap::fromImage (std::move (image));
}

class RenderCache : public QObject {
	Q_OBJECT

private:
};

#endif

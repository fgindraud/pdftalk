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
#ifndef RENDER_H
#define RENDER_H

/* My analysis of PDFpc rendering strategy:
 * PDFpc prerenders pages and then compress the bitmap data.
 * When displaying, it thus needs to uncompress the data and recreate a "pixmap" to display.
 * No idea which pixmap caching is done, or how pixmap are resized for the different uses.
 *
 * My strategy is similar:
 * Renders (QImages) are stored as compressed data (metadata + qCompressed' QByteArray of data).
 */
#include "document.h"

#include <QPixmap>
#include <QSize>

namespace Render {

// Represent a render request (page + size)
struct Request {
	const PageInfo * page;
	QSize size;
};

class Cache : public QObject {
	/* Global render caching system.
	 * TODO implem (use QCache for internal ?)
	 */
	Q_OBJECT

signals:
	void new_render (const QObject * requester, Request request, QPixmap render);

public slots:
	void request_render (Request request);
};

void register_metatypes (void);
}
Q_DECLARE_METATYPE (Render::Request);

#endif

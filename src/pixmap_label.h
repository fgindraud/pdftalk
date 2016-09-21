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
#ifndef PIXMAP_LABEL_H
#define PIXMAP_LABEL_H

#include <QLabel>
#include <QPixmap>

class PixmapLabel : public QLabel {
	/* Label that holds a pixmap and resizes it while keeping aspect ratio, at the center.
	 * It also sends a signal when resized.
	 */
	Q_OBJECT

private:
	QPixmap pixmap_; // Origin sized pixmap

public:
	explicit PixmapLabel (QWidget * parent = nullptr) : QLabel (parent) {
		setScaledContents (false);
		setAlignment (Qt::AlignCenter);
	}

	QSize minimumSizeHint (void) const Q_DECL_OVERRIDE { return {1, 1}; }
	int heightForWidth (int w) const Q_DECL_OVERRIDE {
		if (pixmap_.isNull ()) {
			return QLabel::heightForWidth (w);
		} else {
			return static_cast<qreal> (pixmap_.height ()) * static_cast<qreal> (w) /
			       static_cast<qreal> (pixmap_.width ());
		}
	}
	QSize sizeHint (void) const Q_DECL_OVERRIDE {
		return QSize (width (), heightForWidth (width ()));
	}

	void resizeEvent (QResizeEvent *) Q_DECL_OVERRIDE {
		QLabel::setPixmap (make_scaled_pixmap ());
		emit size_changed (size ());
	}

signals:
	void size_changed (QSize new_size);

public slots:
	void setPixmap (const QPixmap & pixmap) {
		pixmap_ = pixmap;
		QLabel::setPixmap (make_scaled_pixmap ());
	}

private:
	QPixmap make_scaled_pixmap (void) const {
		if (pixmap_.isNull () || size () == pixmap_.size ()) {
			return pixmap_;
		} else {
			return pixmap_.scaled (size (), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		}
	}
};

#endif

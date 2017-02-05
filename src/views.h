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
#ifndef VIEWS_H
#define VIEWS_H

#include "document.h"

#include <QLabel>
#include <QWidget>

#include <QPixmap>

class PageViewer : public QLabel {
	/* This widget will show a PDF page (using a QLabel).
	 * It is shown maximized (keeping aspect ratio), and centered.
	 *
	 * The PDF page is indicated by a pointer to a PageInfo structure.
	 * It will be updated by the Controller during transitions (change_page).
	 *
	 * For now, the PDF page is rendered after every change of page or size.
	 * TODO use caching...
	 *
	 * This widget also catches click events and will activate the page actions accordingly.
	 */
	Q_OBJECT

private:
	const PageInfo * page_{nullptr};

public:
	explicit PageViewer (QWidget * parent = nullptr);

	// Layouting info
	int heightForWidth (int w) const Q_DECL_OVERRIDE;
	QSize sizeHint (void) const Q_DECL_OVERRIDE { return {width (), heightForWidth (width ())}; }

	void resizeEvent (QResizeEvent *) Q_DECL_OVERRIDE { update_label (); }
	void mouseReleaseEvent (QMouseEvent * event) Q_DECL_OVERRIDE;

signals:
	void action_activated (const Action::Base * action);

	void request_pixmap (const QObject * requester, int page_index, QSize box);

public slots:
	void change_page (const PageInfo * new_page) {
		if (new_page != page_) {
			page_ = new_page;
			update_label ();
		}
	}
	void receive_pixmap (const QObject * requester, int page_index, QPixmap pixmap) {}

private:
	void update_label (void) {
		static constexpr int pixmap_size_limit_px = 10;
		if (page_ == nullptr || width () < pixmap_size_limit_px || height () < pixmap_size_limit_px) {
			clear ();
		} else {
			auto pix = QPixmap::fromImage (page_->render (size ()));
			setPixmap (pix);
		}
	}
	//		to_show = to_show.scaled (size (), Qt::KeepAspectRatio, Qt::SmoothTransformation);
};

class PresentationView : public PageViewer {
	// Just one PageViewer, but also set a black background.
	Q_OBJECT

public:
	explicit PresentationView (QWidget * parent = nullptr);
};

class PresenterView : public QWidget {
	//
	Q_OBJECT

private:
	static constexpr qreal bottom_bar_text_point_size_factor = 2.0;

	const int nb_slides_;
	PageViewer * current_page_;
	PageViewer * previous_transition_page_;
	PageViewer * next_transition_page_;
	PageViewer * next_slide_first_page_;
	QLabel * annotations_;
	QLabel * slide_number_label_;
	QLabel * timer_label_;

public:
	explicit PresenterView (int nb_slides, QWidget * parent = nullptr);

	PageViewer * current_page_viewer (void) const { return current_page_; }
	PageViewer * next_slide_first_page_viewer (void) const { return next_slide_first_page_; }
	PageViewer * next_transition_page_viewer (void) const { return next_transition_page_; }
	PageViewer * previous_transition_page_viewer (void) const { return previous_transition_page_; }

public slots:

	void change_slide (int new_slide_number) {
		slide_number_label_->setText (tr ("%1/%2").arg (new_slide_number + 1).arg (nb_slides_));
	}
	void change_time (bool paused, const QString & new_time_text);
	void change_annotations (QString new_annotations) { annotations_->setText (new_annotations); }
};

#endif

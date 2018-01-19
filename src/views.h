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
#pragma once

#include "document.h"
#include "render.h"

#include <QLabel>
#include <QWidget>

#include <QPixmap>

class PageViewer : public QLabel {
	/* This widget will show a PDF page (using a QLabel).
	 * It is shown maximized (keeping aspect ratio), and centered.
	 *
	 * The current pixmap is indicated by a Render::Info structure.
	 * This struct indicates which page is shown, and at which rendered size.
	 * It will be updated by the Controller during transitions (change_page).
	 *
	 * Requests for Pixmaps will go through the Rendering system.
	 *
	 * This widget also catches click events and will activate the page actions accordingly.
	 */
	Q_OBJECT

private:
	Render::Info render_{};

public:
	explicit PageViewer (QWidget * parent = nullptr);

	// Layouting info
	int heightForWidth (int w) const Q_DECL_OVERRIDE;
	QSize sizeHint () const Q_DECL_OVERRIDE;

	void resizeEvent (QResizeEvent *) Q_DECL_OVERRIDE;
	void mouseReleaseEvent (QMouseEvent * event) Q_DECL_OVERRIDE;

signals:
	void action_activated (const Action::Base * action);
	void request_render (Render::Request request);

public slots:
	void change_page (const PageInfo * new_page);
	void receive_pixmap (const QObject * requester, const Render::Info & render_info, QPixmap pixmap);

private:
	void update_label (const PageInfo * new_page);
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

	PageViewer * current_page_viewer () const { return current_page_; }
	PageViewer * next_slide_first_page_viewer () const { return next_slide_first_page_; }
	PageViewer * next_transition_page_viewer () const { return next_transition_page_; }
	PageViewer * previous_transition_page_viewer () const { return previous_transition_page_; }

public slots:

	void change_slide (int new_slide_number) {
		slide_number_label_->setText (tr ("%1/%2").arg (new_slide_number + 1).arg (nb_slides_));
	}
	void change_time (bool paused, const QString & new_time_text);
	void change_annotations (QString new_annotations) { annotations_->setText (new_annotations); }
};

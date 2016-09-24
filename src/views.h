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

#include "controller.h"
#include "document.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

#include <QDebug>

class PageViewer : public QLabel {
	/* Show a pdf page.
	 * Label that holds a pixmap and resizes it while keeping aspect ratio, at the center.
	 * The page shown is selected by asking for a PageIndex.
	 * Then a request to the rendering cache is emitted, and shown on answer.
	 * TODO remake doc + use cache
	 */
	Q_OBJECT

private:
	const PageInfo * page_{nullptr};

public:
	explicit PageViewer (QWidget * parent = nullptr) : QLabel (parent) {
		setScaledContents (false);
		setAlignment (Qt::AlignCenter);
		setMinimumSize (1, 1); // To prevent nil QLabel when no pixmap is available
		QSizePolicy policy{QSizePolicy::Expanding, QSizePolicy::Expanding};
		policy.setHeightForWidth (true);
		setSizePolicy (policy);
	}

	int heightForWidth (int w) const Q_DECL_OVERRIDE {
		if (page_ == nullptr) {
			return QLabel::heightForWidth (w);
		} else {
			return page_->height_for_width_ratio () * w;
		}
	}
	QSize sizeHint (void) const Q_DECL_OVERRIDE { return {width (), heightForWidth (width ())}; }

	void resizeEvent (QResizeEvent *) Q_DECL_OVERRIDE { update_label (); }

signals:
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
		qDebug () << objectName () << size ();
		static constexpr int pixmap_size_limit_px = 10;
		if (page_ == nullptr || width () < pixmap_size_limit_px || height () < pixmap_size_limit_px) {
			clear ();
		} else {
			setPixmap (QPixmap::fromImage (page_->render (size ())));
		}
	}
	//		to_show = to_show.scaled (size (), Qt::KeepAspectRatio, Qt::SmoothTransformation);
};

class PresentationView : public PageViewer {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
	Q_OBJECT

public:
	explicit PresentationView (QWidget * parent = nullptr) : PageViewer (parent) {
		// Title
		setWindowTitle (tr ("Presentation screen"));
		// Black background
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		setPalette (p);
		setAutoFillBackground (true);
		// Debug identification
		setObjectName ("presentation/current");
	}
};

class PresenterView : public QWidget {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
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
	explicit PresenterView (int nb_slides, QWidget * parent = nullptr)
	    : QWidget (parent), nb_slides_ (nb_slides) {
		// Title
		setWindowTitle (tr ("Presenter screen"));
		// Black background, white text, big bold font for childrens
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		p.setColor (QPalette::WindowText, Qt::white);
		setPalette (p);
		setAutoFillBackground (true);

		// View structure
		auto window_structure = new QVBoxLayout;
		setLayout (window_structure);
		{
			auto slide_panels = new QHBoxLayout;
			window_structure->addLayout (slide_panels, 1);
			{
				// Current slide preview
				auto current_slide_panel = new QVBoxLayout;
				slide_panels->addLayout (current_slide_panel, 6); // 60% screen width

				current_page_ = new PageViewer;
				current_page_->setObjectName ("presenter/current");
				current_slide_panel->addWidget (current_page_, 7); // 70% screen height

				auto transition_box = new QHBoxLayout;
				current_slide_panel->addLayout (transition_box, 3); // 30% screen height
				{
					previous_transition_page_ = new PageViewer;
					previous_transition_page_->setObjectName ("presenter/prev_transition");
					transition_box->addWidget (previous_transition_page_);

					transition_box->addStretch ();

					next_transition_page_ = new PageViewer;
					next_transition_page_->setObjectName ("presenter/next_transition");
					transition_box->addWidget (next_transition_page_);
				}

				current_slide_panel->addStretch (); // Pad
			}
			{
				// Next slide preview, and annotations
				auto next_slide_and_comment_panel = new QVBoxLayout;
				slide_panels->addLayout (next_slide_and_comment_panel, 4); // 40% screen width

				next_slide_first_page_ = new PageViewer;
				next_slide_first_page_->setObjectName ("presenter/next_slide");
				next_slide_and_comment_panel->addWidget (next_slide_first_page_);

				annotations_ = new QLabel;
				next_slide_and_comment_panel->addWidget (annotations_);

				next_slide_and_comment_panel->addStretch (); // Pad
			}
		}
		{
			// Bottom bar with slide number and time
			auto bottom_bar = new QHBoxLayout;
			window_structure->addLayout (bottom_bar);
			{
				slide_number_label_ = new QLabel;
				slide_number_label_->setAlignment (Qt::AlignCenter);
				QFont f (slide_number_label_->font ());
				f.setPointSizeF (bottom_bar_text_point_size_factor * f.pointSizeF ());
				slide_number_label_->setFont (f);
				bottom_bar->addWidget (slide_number_label_);
			}
			{
				timer_label_ = new QLabel;
				timer_label_->setAlignment (Qt::AlignCenter);
				QFont f (timer_label_->font ());
				f.setPointSizeF (bottom_bar_text_point_size_factor * f.pointSizeF ());
				timer_label_->setFont (f);
				bottom_bar->addWidget (timer_label_);
			}
		}
	}

	PageViewer * current_page_viewer (void) { return current_page_; }
	PageViewer * next_slide_first_page_viewer (void) { return next_slide_first_page_; }
	PageViewer * next_transition_page_viewer (void) { return next_transition_page_; }
	PageViewer * previous_transition_page_viewer (void) { return previous_transition_page_; }

public slots:

	void slide_changed (int new_slide_number) {
		slide_number_label_->setText (tr ("%1/%2").arg (new_slide_number + 1).arg (nb_slides_));
	}
	void time_changed (bool paused, QString new_time_text) {
		// Set text as colored if paused
		auto color = Qt::white;
		if (paused)
			color = Qt::cyan;
		QPalette p (timer_label_->palette ());
		p.setColor (QPalette::WindowText, color);
		timer_label_->setPalette (p);
		timer_label_->setText (new_time_text);
	}
};

#endif

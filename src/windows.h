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
#ifndef WINDOWS_H
#define WINDOWS_H

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

class SlideViewer : public QLabel {
	/* Label that holds a pixmap and resizes it while keeping aspect ratio, at the center.
	 * It also sends a signal when resized.
	 */
	Q_OBJECT

private:
	QPixmap pixmap_; // Origin sized pixmap

public:
	explicit SlideViewer (QWidget * parent = nullptr) : QLabel (parent) {
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

class PresentationWindow : public SlideViewer {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
	Q_OBJECT

public:
	explicit PresentationWindow (QWidget * parent = nullptr) : SlideViewer (parent) {
		// Title
		setWindowTitle (tr ("Presentation screen"));
		// Black background
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		setPalette (p);
		setAutoFillBackground (true);
	}
};

class PresenterWindow : public QWidget {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
	Q_OBJECT

private:
	static constexpr qreal bottom_bar_text_point_size_factor = 2.0;

	int nb_slides_;

	SlideViewer * current_page_;
	SlideViewer * previous_transition_page_;
	SlideViewer * next_transition_page_;
	SlideViewer * next_slide_page_;
	QLabel * annotations_;

	QLabel * slide_number_label_;
	QLabel * timer_label_;

public:
	explicit PresenterWindow (int nb_slides, QWidget * parent = nullptr)
	    : QWidget (parent), nb_slides_ (nb_slides) {
		// Title
		setWindowTitle (tr ("Presenter screen"));
		// Black background, white text, big bold font for childrens
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		p.setColor (QPalette::WindowText, Qt::white);
		setPalette (p);
		setAutoFillBackground (true);

		// Window structure
		auto window_structure = new QVBoxLayout;
		setLayout (window_structure);
		{
			auto slide_panels = new QHBoxLayout;
			window_structure->addLayout (slide_panels, 1);
			{
				// Current slide preview
				auto current_slide_panel = new QVBoxLayout;
				slide_panels->addLayout (current_slide_panel, 6); // 60%

				current_page_ = new SlideViewer;
				current_slide_panel->addWidget (current_page_);

				auto transition_box = new QHBoxLayout;
				current_slide_panel->addLayout (transition_box);
				{
					previous_transition_page_ = new SlideViewer;
					transition_box->addWidget (previous_transition_page_);

					transition_box->addStretch ();

					next_transition_page_ = new SlideViewer;
					transition_box->addWidget (next_transition_page_);
				}

				current_slide_panel->addStretch (); // Pad
			}
			{
				// Next slide preview, and annotations
				auto next_slide_and_comment_panel = new QVBoxLayout;
				slide_panels->addLayout (next_slide_and_comment_panel, 4); // 40%

				next_slide_page_ = new SlideViewer;
				next_slide_and_comment_panel->addWidget (next_slide_page_);

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

public slots:
	void current_page_changed (const QPixmap & new_pixmap) {
		current_page_->setPixmap (new_pixmap);
		next_slide_page_->setPixmap (new_pixmap);
	}

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

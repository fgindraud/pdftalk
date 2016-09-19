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
#ifndef PRESENTER_WINDOW_H
#define PRESENTER_WINDOW_H

#include <QDebug>

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QWidget>

class PresenterWindow : public QWidget {
	/* Presentation window.
	 * Just show the full size pdf page, keeping aspect ratio, with black borders.
	 * setPixmap slot from QLabel is reused as it is.
	 */
	Q_OBJECT

private:
	int nb_slides_;

	QLabel * slide_number_label;
	QLabel * timer_label;

public:
	PresenterWindow (int nb_slides, QWidget * parent = nullptr)
	    : QWidget (parent), nb_slides_ (nb_slides) {
		// Title
		setWindowTitle (tr ("Presenter screen"));
		// Black background, white text, big bold font for childrens
		QPalette p (palette ());
		p.setColor (QPalette::Window, Qt::black);
		p.setColor (QPalette::WindowText, Qt::white);
		setPalette (p);
		QFont f (font ());
		f.setPointSize (20);
		setFont (f);
		setAutoFillBackground (true);

		// Window structure
		auto bottom_bar = new QHBoxLayout;
		setLayout (bottom_bar);

		slide_number_label = new QLabel;
		slide_number_label->setAlignment (Qt::AlignCenter);
		bottom_bar->addWidget (slide_number_label);

		timer_label = new QLabel;
		timer_label->setAlignment (Qt::AlignCenter);
		bottom_bar->addWidget (timer_label);
	}

public slots:
	void slide_changed (int new_slide_number) {
		slide_number_label->setText (tr ("%1/%2").arg (new_slide_number + 1).arg (nb_slides_));
	}

	void time_changed (bool paused, QString new_time_text) {
		// Set text as colored if paused
		auto color = Qt::white;
		if (paused)
			color = Qt::cyan;
		QPalette p (timer_label->palette ());
		p.setColor (QPalette::WindowText, color);
		timer_label->setPalette (p);
		timer_label->setText (new_time_text);
	}
};

#endif

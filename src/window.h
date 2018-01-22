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

#include "utils.h"

#include <QApplication>
#include <QKeySequence>
#include <QMainWindow>
#include <QShortcut>
#include <memory>
#include <vector>

/* Simple container for presentation widgets.
 *
 * Detect close event to quit application.
 * Toggle fullscreen on 'f' key.
 */
class Window : public QMainWindow {
	Q_OBJECT

public:
	Window () {
		// Add fullscreen shortcut
		auto * sc = new QShortcut (QKeySequence (tr ("f", "fullscreen key")), this);
		sc->setAutoRepeat (false);
		connect (sc, &QShortcut::activated, this, &Window::toogle_fullscreen);
	}

private slots:
	void toogle_fullscreen () { setWindowState (windowState () ^ Qt::WindowFullScreen); }

private:
	void closeEvent (QCloseEvent *) Q_DECL_OVERRIDE { QApplication::quit (); }
};

/* This class takes content QWidgets, and place them in Window widgets.
 * WindowShifter takes ownership of the QWidgets (Qt parent ownership).
 *
 * Pushing the 'f' key on a window will put it in fullscreen.
 * Pushing the 's' key will rotate content between windows.
 * The windowTitle property of content widgets is propagated to their associated window.
 *
 * TODO place on different monitors by default
 * QWidget::window() returns the QWindow
 * QWindow::setScreen()/QWindow::screen()
 * QScreen: get screens and such
 */
class WindowShifter : public QObject {
	Q_OBJECT

private:
	std::vector<QWidget *> widgets_;
	std::vector<std::unique_ptr<Window>> windows_;
	std::size_t current_shift_{0};

public:
	explicit WindowShifter (std::initializer_list<QWidget *> widgets) : widgets_ (widgets) {
		// Create one window for each widget
		for (std::size_t i = 0; i < nb_widgets (); ++i) {
			auto w = make_unique<Window> ();
			{
				// Add swap shortcut
				auto * sc = new QShortcut (QKeySequence (tr ("s", "swap key")), w.get ());
				sc->setAutoRepeat (false);
				connect (sc, &QShortcut::activated, this, &WindowShifter::shift_content);
			}
			windows_.emplace_back (std::move (w));
		}
		set_content_position ();
		for (auto & w : windows_)
			w->show ();
	}

private slots:
	void shift_content () {
		current_shift_ = (current_shift_ + 1) % nb_widgets ();
		set_content_position ();
	}

private:
	std::size_t nb_widgets () const { return widgets_.size (); }

	// Set widgets to their windows, according to the current shift.
	void set_content_position () {
		// De-parent contents
		for (auto * c : widgets_) {
			if (c->parentWidget () != nullptr) {
				disconnect (c, &QWidget::windowTitleChanged, c->parentWidget (), &QWidget::setWindowTitle);
				c->setParent (nullptr);
			}
		}
		// Re-set MainWindow widgets
		for (std::size_t i = 0; i < nb_widgets (); ++i) {
			auto * w = windows_[(i + current_shift_) % nb_widgets ()].get ();
			auto * child = widgets_[i];
			w->setCentralWidget (child);
			w->setWindowTitle (child->windowTitle ());
			connect (child, &QWidget::windowTitleChanged, w, &QWidget::setWindowTitle);
		}
	}
};

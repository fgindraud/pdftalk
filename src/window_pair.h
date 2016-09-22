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
#ifndef WINDOW_PAIR_H
#define WINDOW_PAIR_H

#include <QApplication>
#include <QKeySequence>
#include <QMainWindow>
#include <QShortcut>
#include <array>

class WindowPairElement : public QMainWindow {
	/* Detect close event to quit application.
	 * Toggle fullscreen on 'f' key.
	 */
	Q_OBJECT

public:
	WindowPairElement () {
		auto sc = new QShortcut (QKeySequence (tr ("f", "fullscreen key")), this);
		sc->setAutoRepeat (false);
		connect (sc, &QShortcut::activated, this, &WindowPairElement::toogle_fullscreen);
	}

private slots:
	void toogle_fullscreen (void) { setWindowState (windowState () ^ Qt::WindowFullScreen); }

private:
	void closeEvent (QCloseEvent *) Q_DECL_OVERRIDE { QApplication::quit (); }
};

class WindowPair : public QObject {
	/* This class takes nb_window content QWidgets, and place them in nb_window windows.
	 * Pushing the 'f' key on a window will put it in fullscreen.
	 * Pushing the 's' key will rotate content between windows.
	 * The windowTitle property of content widgets is propagated to their associated window.
	 *
	 * TODO place on different monitors by default
	 * QWidget::window() returns the QWindow
	 * QWindow::setScreen()/QWindow::screen()
	 * QScreen: get screens and such
	 */
	Q_OBJECT

private:
	static constexpr int nb_window = 2; // cannot be template due to Q_OBJECT...
	std::array<WindowPairElement, nb_window> windows_;
	std::array<QWidget *, nb_window> contents_;
	int current_shift_{0};

public:
	template <typename... Args>
	explicit WindowPair (Args &&... args) : contents_{std::forward<Args> (args)...} {
		for (auto & w : windows_) {
			// Swap shortcut
			auto sc = new QShortcut (QKeySequence (tr ("s", "swap key")), &w);
			sc->setAutoRepeat (false);
			connect (sc, &QShortcut::activated, this, &WindowPair::shift_content);
		}
		set_content_position ();
		for (auto & w : windows_)
			w.show ();
	}

private slots:
	void shift_content (void) {
		current_shift_ = (current_shift_ + 1) % nb_window;
		set_content_position ();
	}

private:
	void set_content_position (void) {
		// De-parent contents
		for (auto & c : contents_) {
			if (c->parentWidget () != nullptr) {
				disconnect (c, &QWidget::windowTitleChanged, c->parentWidget (), &QWidget::setWindowTitle);
				c->setParent (nullptr);
			}
		}
		// Re-set MainWindow widgets
		for (int i = 0; i < nb_window; ++i) {
			auto w = &windows_[(i + current_shift_) % nb_window];
			auto child = contents_[i];
			w->setCentralWidget (child);
			w->setWindowTitle (child->windowTitle ());
			connect (child, &QWidget::windowTitleChanged, w, &QWidget::setWindowTitle);
		}
	}
};

#endif

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
#include "cache.h"
#include "controller.h"
#include "window_pair.h"
#include "windows.h"

#include <QApplication>
#include <QCommandLineParser>

int main (int argc, char * argv[]) {
	QApplication app (argc, argv);
	QCoreApplication::setApplicationName ("pdftalk");
#define XSTR(x) #x
#define STR(x) XSTR (x)
	QCoreApplication::setApplicationVersion (STR (PDFTALK_VERSION));
#undef STR
#undef XSTR
	QApplication::setApplicationDisplayName ("PDFTalk");

	qRegisterMetaType<CompressedRender> ();

	QCommandLineParser parser;
	parser.setApplicationDescription ("PDF presentation tool");
	parser.addHelpOption ();
	parser.addPositionalArgument ("pdf_file", QApplication::translate ("main", "PDF file to open"));
	parser.process (app);

	auto arguments = parser.positionalArguments ();
	if (arguments.size () != 1) {
		parser.showHelp (EXIT_FAILURE);
		Q_UNREACHABLE ();
	}

	const Document document (arguments[0]);
	Controller control (document);
	RenderCache cache (document);

	// Setup windows
	auto presentation_window = new PresentationWindow;
	auto presenter_window = new PresenterWindow (document.nb_slides ());
	add_shortcuts_to_widget (control, presentation_window);
	add_shortcuts_to_widget (control, presenter_window);

	// Link viewers to controller
	QObject::connect (&control, &Controller::slide_changed, presenter_window,
	                  &PresenterWindow::slide_changed);
	QObject::connect (&control, &Controller::time_changed, presenter_window,
	                  &PresenterWindow::time_changed);

	QObject::connect (&control, &Controller::current_page_changed, presentation_window,
	                  &SlideViewer::change_page);
	QObject::connect (&control, &Controller::current_page_changed,
	                  presenter_window->current_page_viewer (), &SlideViewer::change_page);
	// TODO missing viewers

	// Link viewers to caching system
	SlideViewer * viewers[] = {presentation_window, presenter_window->current_page_viewer (),
	                           presenter_window->next_slide_page_viewer (),
	                           presenter_window->next_transition_page_viewer (),
	                           presenter_window->previous_transition_page_viewer ()};
	for (auto v : viewers) {
		QObject::connect (v, &SlideViewer::request_pixmap, &cache, &RenderCache::request_page);
		QObject::connect (&cache, &RenderCache::new_pixmap, v, &SlideViewer::receive_pixmap);
	}

	// Setup window swapping system
	WindowPair windows{presentation_window, presenter_window};

	// Initialise timer text in interface
	control.timer_reset (); // TODO reset controller to give first page orders
	return app.exec ();
}

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
#include "presentation.h"
#include "windows.h"
#include "window_pair.h"

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

	Presentation presentation (arguments[0]);

	// Setup windows and connect to presentation controller object
	auto presentation_window = new PresentationWindow;
	add_presentation_shortcuts_to_widget (presentation, presentation_window);
	QObject::connect (presentation_window, &PresentationWindow::size_changed, &presentation,
	                  &Presentation::presentation_window_size_changed);
	QObject::connect (&presentation, &Presentation::presentation_pixmap_changed, presentation_window,
	                  &PresentationWindow::setPixmap);

	auto presenter_window = new PresenterWindow (presentation.nb_slides ());
	add_presentation_shortcuts_to_widget (presentation, presenter_window);
	QObject::connect (&presentation, &Presentation::presentation_pixmap_changed, presenter_window,
	                  &PresenterWindow::current_page_changed);
	QObject::connect (&presentation, &Presentation::slide_changed, presenter_window,
	                  &PresenterWindow::slide_changed);
	QObject::connect (&presentation, &Presentation::time_changed, presenter_window,
	                  &PresenterWindow::time_changed);

	WindowPair windows{presentation_window, presenter_window};

	// Initialise timer text in interface
	presentation.timer_reset ();
	return app.exec ();
}

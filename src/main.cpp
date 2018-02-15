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
#include "action.h"
#include "controller.h"
#include "document.h"
#include "render.h"
#include "views.h"
#include "window.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

/* Main components of PDFTalk:
 *
 * Document: stores the PDF information (pages, organization, rendering with poppler).
 * PageViewer widgets: show a single image (one rendered PDF page).
 * PresentationView/PresenterView: composed of PageViewers, provide the window layouts.
 * WindowShifter: create OS windows, contains PresentationView/PresenterView widgets.
 * Render::System: provider of page renders, answers to requests, caching / prefetching.
 * Controller: store the presentation state, controls all PageViewers.
 *
 * User input (shortcuts, clicks) are detected at widget level.
 * Window changes (fullscreen, swap) are handled directly by WindowShifter/Window container.
 * All other actions are redirected to the controller.
 *
 * The controller updates it internal state using presentation structure from the PDF document.
 * It then sends signals to PageViewers to indicate the new shown page for each.
 * It also updates the annotation / slide number / timer widgets.
 *
 * PageViewers will then request new images (page + render size) from the render system.
 * The render system will answer later with rendered pages.
 * The render system performs caching and pre-rendering of pages.
 * This is done according to request data : widget size, current page, role, movement.
 * The renderer only interacts with PageViewers (not the controller).
 */

int main (int argc, char * argv[]) {
	// Qt setup
	QApplication app (argc, argv);
	QCoreApplication::setApplicationName ("pdftalk");
#define XSTR(x) #x
#define STR(x) XSTR (x)
	QCoreApplication::setApplicationVersion (STR (PDFTALK_VERSION));
#undef STR
#undef XSTR
	QApplication::setApplicationDisplayName ("PDFTalk");

	// Type registration (once before use in connect)
	qRegisterMetaType<Render::Info> ();
	qRegisterMetaType<Render::Request> ();

	// Command line parsing
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

	// TODO add nicer error detection on pdf opening...
	const Document document (arguments[0]);
	Controller control (document);
	Render::System renderer (10000000, 1); // Cache size, 10Mo, TODO program arg

	// Setup windows
	auto presentation_view = new PresentationView;
	auto presenter_view = new PresenterView (document.nb_slides ());
	add_shortcuts_to_widget (control, presentation_view);
	add_shortcuts_to_widget (control, presenter_view);

	// Link non slide widgets to controller.
	QObject::connect (&control, &Controller::slide_changed, presenter_view,
	                  &PresenterView::change_slide);
	QObject::connect (&control, &Controller::time_changed, presenter_view,
	                  &PresenterView::change_time);
	QObject::connect (&control, &Controller::annotations_changed, presenter_view,
	                  &PresenterView::change_annotations);

	// Link slide viewers to controller, actions, caching system
	auto viewers =
	    std::initializer_list<PageViewer *>{presentation_view, presenter_view->current_page_viewer (),
	                                        presenter_view->next_slide_first_page_viewer (),
	                                        presenter_view->next_transition_page_viewer (),
	                                        presenter_view->previous_transition_page_viewer ()};
	for (auto v : viewers) {
		QObject::connect (&control, &Controller::current_page_changed, v,
		                  &PageViewer::change_current_page);

		QObject::connect (v, &PageViewer::action_activated, &control, &Controller::execute_action);

		QObject::connect (v, &PageViewer::request_render, &renderer, &Render::System::request_render);
		QObject::connect (&renderer, &Render::System::new_render, v, &PageViewer::receive_pixmap);
	}

	// Setup window swapping system
	WindowShifter windows{presentation_view, presenter_view};

	// Init system
	QTimer::singleShot (0, &control, SLOT (reset ()));
	return app.exec ();
}

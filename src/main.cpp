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
#include <cstdio>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QStringList>
#include <QTextStream>
#include <QTimer>

#include "action.h"
#include "controller.h"
#include "document.h"
#include "render.h"
#include "views.h"
#include "window.h"

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

	auto tr = [&app](const char * s) { return app.translate ("main", s); };

	// Type registration (once before use in connect)
	qRegisterMetaType<Render::Info> ();
	qRegisterMetaType<Render::Request> ();

	int render_cache_size = 10 * (1 << 20); // 10MB default
	auto * prefetch_strategy = Render::default_prefetch_strategy ();

	// Command line parsing
	QCommandLineParser parser;
	parser.setApplicationDescription (tr ("PDF presentation tool"));
	parser.addHelpOption ();
	parser.addPositionalArgument (tr ("file.pdf"), tr ("PDF file to open"));
	QCommandLineOption render_cache_size_option (
	    QStringList () << "c"
	                   << "cache",
	    tr ("Render cache size (default = %1)").arg (size_in_bytes_to_string (render_cache_size)),
	    tr ("size"));
	parser.addOption (render_cache_size_option);
	QCommandLineOption pdfpc_filename_option (QStringList () << "a"
	                                                         << "annotations",
	                                          tr ("Annotation file name (default = file.pdfpc)"),
	                                          tr ("file"));
	parser.addOption (pdfpc_filename_option);
	QCommandLineOption prefetch_strategy_option (
	    QStringList () << "p"
	                   << "prefetch",
	    tr ("Prefetch strategy (%1)").arg (Render::list_of_prefetch_strategy_names ().join (',')),
	    tr ("name"));
	parser.addOption (prefetch_strategy_option);
	parser.process (app);

	auto arguments = parser.positionalArguments ();
	if (arguments.size () != 1) {
		parser.showHelp (EXIT_FAILURE);
		Q_UNREACHABLE ();
	}
	QString filename = arguments[0];

	if (parser.isSet (render_cache_size_option)) {
		auto size_str = parser.value (render_cache_size_option);
		int size = string_to_size_in_bytes (size_str);
		if (size >= 0) {
			render_cache_size = size;
		} else {
			QTextStream (stderr) << tr ("Error: Invalid cache size: %1 (from \"%2\"), using default\n")
			                            .arg (size)
			                            .arg (size_str);
		}
	}

	QString pdfpc_filename = filename + "pc";
	if (parser.isSet (pdfpc_filename_option)) {
		pdfpc_filename = parser.value (pdfpc_filename_option);
	}

	if (parser.isSet (prefetch_strategy_option)) {
		auto name = parser.value (prefetch_strategy_option);
		auto * strategy = Render::select_prefetch_strategy_by_name (name);
		if (strategy != nullptr) {
			prefetch_strategy = strategy;
		} else {
			QTextStream (stderr)
			    << tr ("Warning: prefetch strategy \"%1\" not found, falling back to default\n")
			           .arg (name);
		}
	}

	auto document = Document::open (filename, pdfpc_filename);
	if (!document) {
		return EXIT_FAILURE;
	}

	Controller control (*document);
	Render::System renderer (render_cache_size, prefetch_strategy);

	// Setup windows
	auto presentation_view = new PresentationView;
	auto presenter_view = new PresenterView (document->nb_slides ());
	add_shortcuts_to_widget (control, presentation_view);
	add_shortcuts_to_widget (control, presenter_view);

	// Link non slide widgets to controller.
	QObject::connect (&control, &Controller::current_page_changed, presenter_view,
	                  &PresenterView::change_slide_info);
	QObject::connect (&control, &Controller::time_changed, presenter_view,
	                  &PresenterView::change_time);

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

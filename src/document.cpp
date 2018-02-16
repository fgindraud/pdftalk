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
#include "document.h"
#include "action.h"
#include "utils.h"

#include <QCoreApplication>
#include <QDebugStateSaver>
#include <QFile>
#include <QImage>
#include <QTextStream>
#include <algorithm>
#include <cstdio>
#include <poppler-qt5.h>

// PageInfo

void add_page_actions (std::vector<std::unique_ptr<Action::Base>> & actions,
                       const Poppler::Page & page) {
	for (const auto * link : page.links ()) {
		std::unique_ptr<Action::Base> new_action{nullptr};
		// Build an action if it matches the supported types
		using PL = Poppler::Link;
		switch (link->linkType ()) {
		case PL::Goto: {
			auto * p = dynamic_cast<const Poppler::LinkGoto *> (link);
			if (!p->isExternal ()) {
				auto page_index = p->destination ().pageNumber () - 1;
				new_action = make_unique<Action::PageIndex> (page_index);
			}
		} break;
		case PL::Action: {
			using PA = Poppler::LinkAction;
			auto * p = dynamic_cast<const PA *> (link);
			switch (p->actionType ()) {
			case PA::Quit:
			case PA::EndPresentation:
			case PA::Close:
				new_action = make_unique<Action::Quit> ();
				break;
			case PA::PageNext:
				new_action = make_unique<Action::PageNext> ();
				break;
			case PA::PagePrev:
				new_action = make_unique<Action::PagePrevious> ();
				break;
			case PA::PageFirst:
				new_action = make_unique<Action::PageFirst> ();
				break;
			case PA::PageLast:
				new_action = make_unique<Action::PageLast> ();
				break;
			default:
				// Not handled: History{Forward/Back}, GoToPage, Find, Print
				// TODO handle gotopage ? (with a hotkey as well)
				break;
			}
		} break;
		case PL::Browse: {
			auto * p = dynamic_cast<const Poppler::LinkBrowse *> (link);
			new_action = make_unique<Action::Browser> (p->url ());
		} break;
		default:
			// Not handled: Execute, Sound, Movie, Rendition, JavaScript
			break;
		}
		// If we build one, add it to list
		if (new_action) {
			new_action->set_rect (link->linkArea ().normalized ());
			actions.emplace_back (std::move (new_action));
		}
		/* Documentation of links() does not say that we get ownership of the Link* objects.
		 * Testing with valgrind show memory leaks if not deleted.
		 * So I assume links() does give ownership of Link* objects.
		 */
		delete link;
	}
}

PageInfo::PageInfo (std::unique_ptr<Poppler::Page> page, int index)
    : poppler_page_ (std::move (page)), page_index_ (index) {
	// precompute height_for_width_ratio
	auto page_size_dots = poppler_page_->pageSizeF ();
	if (!page_size_dots.isEmpty ())
		height_for_width_ratio_ = page_size_dots.height () / page_size_dots.width ();

	add_page_actions (actions_, *poppler_page_);
}

QString PageInfo::label () const {
	return poppler_page_->label ();
}

QSize PageInfo::render_size (const QSize & box) const {
	// Computes the size we can render page in the given box
	const auto page_size_dots = poppler_page_->pageSizeF ();
	if (page_size_dots.isEmpty ())
		return QSize ();
	const qreal pix_dots_ratio =
	    std::min (static_cast<qreal> (box.width ()) / page_size_dots.width (),
	              static_cast<qreal> (box.height ()) / page_size_dots.height ());
	return (page_size_dots * pix_dots_ratio).toSize ();
}

QImage PageInfo::render (const QSize & box) const {
	// Render the page in the box
	const auto page_size_dots = poppler_page_->pageSizeF ();
	if (page_size_dots.isEmpty ())
		return QImage ();
	const qreal pix_dots_ratio =
	    std::min (static_cast<qreal> (box.width ()) / page_size_dots.width (),
	              static_cast<qreal> (box.height ()) / page_size_dots.height ());
	const qreal dpi = pix_dots_ratio * 72.0;
	return poppler_page_->renderToImage (dpi, dpi);
}

const Action::Base * PageInfo::on_click (const QPointF & coord) const {
	for (const auto & action : actions_) {
		if (action->activated (coord))
			return action.get ();
	}
	return nullptr;
}

QDebug operator<< (QDebug d, const PageInfo & page) {
	QDebugStateSaver saver (d);
	d.nospace () << "Page(p=" << page.page_index () << ", s=" << page.slide_index () << ")";
	return d;
}

// SlideInfo

// Document

Document::Document (const QString & filename, std::unique_ptr<Poppler::Document> document)
    : filename_ (filename), document_ (std::move (document)) {}
Document::~Document () = default;

std::unique_ptr<const Document> Document::open (const QString & filename,
                                                const QString & pdfpc_filename) {
	auto tr = [](const char * str) { return qApp->translate ("Document::open", str); };

	auto poppler_doc = std::unique_ptr<Poppler::Document> (Poppler::Document::load (filename));

	// Check document has been opened
	if (!poppler_doc) {
		QTextStream (stderr) << tr ("Error: Poppler: unable to open document \"%1\"\n").arg (filename);
		return nullptr;
	}
	if (poppler_doc->isLocked ()) {
		QTextStream (stderr) << tr ("Error: Poppler: document is locked \"%1\"\n").arg (filename);
		return nullptr;
	}

	// Enable antialiasing, it is better looking
	poppler_doc->setRenderHint (Poppler::Document::Antialiasing, true);
	poppler_doc->setRenderHint (Poppler::Document::TextAntialiasing, true);

	// Document creation and staged init
	auto document = std::unique_ptr<Document>{new Document (filename, std::move (poppler_doc))};

	if (!document->discover_document_structure ()) {
		return nullptr;
	}

	document->read_annotations_from_file (pdfpc_filename);

	return std::move (document);
}

bool Document::discover_document_structure () {
	auto tr = [](const char * str) { return qApp->translate ("discover_document_structure", str); };

	// Create raw PageInfo structs
	const auto nb_pages = static_cast<int> (document_->numPages ());
	if (nb_pages <= 0) {
		QTextStream (stderr)
		    << tr ("Error: Poppler: no pages in the PDF document \"%1\"").arg (filename_);
		return false;
	}
	pages_.reserve (nb_pages);
	for (int i = 0; i < nb_pages; ++i) {
		auto p = std::unique_ptr<Poppler::Page>{document_->page (i)};
		if (!p) {
			QTextStream (stderr) << tr ("Error: Poppler: unable to load page %1 in document \"%2\"")
			                            .arg (i)
			                            .arg (filename_);
			return false;
		}
		pages_.emplace_back (make_unique<PageInfo> (std::move (p), i));
	}

	// Parse all pages, and create SlideInfo structures each time we "change of slide"
	QString current_slide_label = pages_[0]->label ();
	slides_.emplace_back (make_unique<SlideInfo> (0));
	for (int page_index = 0; page_index < nb_pages; ++page_index) {
		auto & current = *pages_[page_index];
		// Link to previous page if it exists
		if (page_index > 0) {
			auto & prev = *pages_[page_index - 1];
			current.set_previous_page (&prev);
			prev.set_next_page (&current);
		}
		// If label changed since last page, this is the first page of a new slide
		auto label = current.label ();
		if (label != current_slide_label) {
			// Set "next first page of slide" links of previous pages
			for (int index = slides_.back ()->first_page_index (); index < page_index; ++index)
				pages_[index]->set_next_slide_first_page (&current);
			// Append to slide list
			slides_.emplace_back (make_unique<SlideInfo> (page_index));
			current_slide_label = label;
		}
		// Update slide numbering
		int current_slide_index = slides_.size () - 1;
		current.set_slide_index (current_slide_index);
		// Update transition links with previous page
		if (page_index > 0) {
			auto & prev = *pages_[page_index - 1];
			if (prev.slide_index () == current_slide_index) {
				prev.set_next_transition_page (&current);
				current.set_previous_transition_page (&prev);
			}
		}
	}
	return true;
}

bool Document::read_annotations_from_file (const QString & pdfpc_filename) {
	auto tr = [](const char * str) { return qApp->translate ("read_annotations_from_file", str); };
	QFile pdfpc_file (pdfpc_filename);
	if (!pdfpc_file.open (QFile::ReadOnly | QFile::Text)) {
		QTextStream (stderr) << tr ("Warning: Unable to open pdfpc file \"%1\", no pdfpc annotations\n")
		                            .arg (pdfpc_filename);
		return false;
	}
	QTextStream stream (&pdfpc_file);
	QString line;
	int line_index = 0;
	bool notes_marker_seen = false;
	int current_slide_index = -1;
	while (!(line = stream.readLine ()).isNull ()) {
		line_index++;
		if (!notes_marker_seen) {
			// Look for [notes] marker
			if (line.trimmed () == "[notes]")
				notes_marker_seen = true;
		} else {
			if (line.startsWith ("###")) {
				// Slide number change
				line.remove (0, 3);
				bool int_parsed = false;
				current_slide_index = line.toInt (&int_parsed, 10) - 1;
				if (!int_parsed || current_slide_index < 0 || current_slide_index >= nb_slides ()) {
					QTextStream (stderr)
					    << tr ("Warning: Malformed slide number in pdfpc file \"%1\", line %2\n")
					           .arg (pdfpc_filename)
					           .arg (line_index);
					return false;
				}
			} else {
				// Annotation line
				if (current_slide_index < 0 || current_slide_index >= nb_slides ()) {
					QTextStream (stderr) << tr ("Warning: Current slide number out of bounds (%1/%2), in "
					                            "pdfpc file \"%3\", line %4\n")
					                            .arg (current_slide_index)
					                            .arg (nb_slides ())
					                            .arg (pdfpc_filename)
					                            .arg (line_index);
					return false;
				}
				slides_[current_slide_index]->append_annotation (line);
			}
		}
	}
	return true;
}

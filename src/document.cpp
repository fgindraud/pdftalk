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

#include <QDebugStateSaver>
#include <QFile>
#include <QImage>
#include <QTextStream>
#include <algorithm>

// PageInfo

void add_page_actions (std::vector<std::unique_ptr<Action::Base>> & actions,
                       const Poppler::Page & page) {
	// Links does not say that we get ownership of the Link* objects... do not delete
	for (const auto * link : page.links ()) {
		std::unique_ptr<Action::Base> new_action{nullptr};
		// Build an action if it matches the supported types
		using PL = Poppler::Link;
		switch (link->linkType ()) {
		case PL::Goto: {
			auto p = dynamic_cast<const Poppler::LinkGoto *> (link);
			if (!p->isExternal ()) {
				auto page_index = p->destination ().pageNumber () - 1;
				new_action = make_unique<Action::PageIndex> (page_index);
			}
		} break;
		case PL::Action: {
			using PA = Poppler::LinkAction;
			auto p = dynamic_cast<const PA *> (link);
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
			auto p = dynamic_cast<const Poppler::LinkBrowse *> (link);
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

QSize PageInfo::render_size (QSize box) const {
	// Computes the size we can render page in the given box
	const auto page_size_dots = poppler_page_->pageSizeF ();
	if (page_size_dots.isEmpty ())
		return QSize ();
	const qreal pix_dots_ratio =
	    std::min (static_cast<qreal> (box.width ()) / page_size_dots.width (),
	              static_cast<qreal> (box.height ()) / page_size_dots.height ());
	return (page_size_dots * pix_dots_ratio).toSize ();
}

QImage PageInfo::render (QSize box) const {
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

Document::Document (const QString & filename) : document_ (Poppler::Document::load (filename)) {
	// Check document has been opened
	if (!document_)
		qFatal ("Poppler: unable to open document \%s\"", qPrintable (filename));
	if (document_->isLocked ())
		qFatal ("Poppler: document is locked \"%s\"", qPrintable (filename));

	// Enable antialiasing, it is better looking
	document_->setRenderHint (Poppler::Document::Antialiasing, true);
	document_->setRenderHint (Poppler::Document::TextAntialiasing, true);

	// Create raw PageInfo structs
	auto nb_pages = static_cast<int> (document_->numPages ());
	if (nb_pages <= 0)
		qFatal ("Poppler: no pages in the PDF document \"%s\"", qPrintable (filename));
	pages_.reserve (nb_pages);
	for (int i = 0; i < nb_pages; ++i) {
		auto p = std::unique_ptr<Poppler::Page>{document_->page (i)};
		if (!p)
			qFatal ("Poppler: unable to load page %d in document \"%s\"", i, qPrintable (filename));
		pages_.emplace_back (make_unique<PageInfo> (std::move (p), i));
	}

	discover_document_structure ();
	read_annotations_from_file (filename + "pc"); // TODO config point
}

void Document::discover_document_structure () {
	// Parse all pages, and create SlideInfo structures each time we "change of slide"
	QString current_slide_label = page (0).label ();
	slides_.emplace_back (make_unique<SlideInfo> (0));
	for (int page_index = 0; page_index < nb_pages (); ++page_index) {
		auto & current = page (page_index);
		// Link to previous page if it exists
		if (page_index > 0) {
			auto & prev = page (page_index - 1);
			current.set_previous_page (&prev);
			prev.set_next_page (&current);
		}
		// If label changed since last page, this is the first page of a new slide
		auto label = current.label ();
		if (label != current_slide_label) {
			// Set "next first page of slide" links of previous pages
			for (int index = slides_.back ()->first_page_index (); index < page_index; ++index)
				page (index).set_next_slide_first_page (&current);
			// Append to slide list
			slides_.emplace_back (make_unique<SlideInfo> (page_index));
			current_slide_label = label;
		}
		// Update slide numbering
		int current_slide_index = slides_.size () - 1;
		current.set_slide_index (current_slide_index);
		// Update transition links with previous page
		if (page_index > 0) {
			auto & prev = page (page_index - 1);
			if (prev.slide_index () == current_slide_index) {
				prev.set_next_transition_page (&current);
				current.set_previous_transition_page (&prev);
			}
		}
	}
}

void Document::read_annotations_from_file (const QString & pdfpc_filename) {
	QFile pdfpc_file (pdfpc_filename);
	if (!pdfpc_file.open (QFile::ReadOnly | QFile::Text)) {
		qWarning ("Unable to open pdfpc file \"%s\", no pdfpc annotations",
		          qPrintable (pdfpc_filename));
		return;
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
					qWarning ("Malformed slide number in pdfpc file \"%s\", line %d",
					          qPrintable (pdfpc_filename), line_index);
					return;
				}
			} else {
				// Annotation line
				if (current_slide_index < 0 || current_slide_index >= nb_slides ()) {
					qWarning ("Current slide number out of bounds (%d/%d), in pdfpc file \"%s\", line %d",
					          current_slide_index, nb_slides (), qPrintable (pdfpc_filename), line_index);
					return;
				}
				slide (current_slide_index).append_annotation (line);
			}
		}
	}
}

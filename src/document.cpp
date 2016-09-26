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
#include "document.h"
#include "action.h"

#include <QFile>
#include <QImage>
#include <QTextStream>
#include <algorithm>

#include <QDebug>

// PageInfo

PageInfo::PageInfo (Poppler::Page * page) : poppler_page_ (page) {
	// precompute height_for_width_ratio
	auto page_size_dots = poppler_page_->pageSizeF ();
	if (!page_size_dots.isEmpty ())
		height_for_width_ratio_ = page_size_dots.height () / page_size_dots.width ();

	init_annotations ();
	init_actions ();
}

void PageInfo::init_annotations (void) {
	// TODO add two runtime options:
	// - bool: hide annotations or not ?
	// - bool: use text annotations in presenter screen ?
	auto list_of_annotations = poppler_page_->annotations ();
	for (auto a : list_of_annotations) {
		if (a->subType () == Poppler::Annotation::AText) {
			auto text = a->contents ();
			annotations_ += text;
			if (!text.endsWith ('\n'))
				annotations_ += '\n';
		}
		// prevent poppler from rendering annotation stuff
		a->setFlags (a->flags () | Poppler::Annotation::Hidden);
		delete a;
	}
}

void PageInfo::init_actions (void) {
	for (auto link : poppler_page_->links ()) {
		Action::Base * new_action = nullptr;
		// Build an action if it matches the supported types
		using PL = Poppler::Link;
		switch (link->linkType ()) {
		case PL::Goto: {
			qDebug () << "Action::Goto";
		} break;
		case PL::Action: {
			using PA = Poppler::LinkAction;
			auto p = dynamic_cast<const PA *> (link);
			switch (p->actionType ()) {
			case PA::Quit:
			case PA::EndPresentation:
			case PA::Close:
				qDebug () << "Action::Quit";
				new_action = new Action::Quit;
				break;
			case PA::PageNext:
				qDebug () << "Action::PageNext";
				new_action = new Action::PageNext;
				break;
			case PA::PagePrev:
				qDebug () << "Action::PagePrevious";
				new_action = new Action::PagePrevious;
				break;
			default:
				break;
			}
		} break;
		case PL::Browse: {
			auto p = dynamic_cast<const Poppler::LinkBrowse *> (link);
			qDebug () << "Action::Browser" << p->url ();
			new_action = new Action::Browser (p->url ());
		} break;
		default:
			break;
		}
		// If we build one, add it to list
		if (new_action != nullptr) {
			new_action->set_rect (link->linkArea ());
			actions_.emplace_back (new_action);
		}
		delete link;
	}
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
	auto it =
	    std::find_if (actions_.begin (), actions_.end (),
	                  [&](const std::unique_ptr<Action::Base> & a) { return a->activated (coord); });
	if (it != actions_.end ()) {
		return it->get ();
	} else {
		return nullptr;
	}
}

// SlideInfo

// Document

Document::Document (const QString & filename) : document_ (Poppler::Document::load (filename)) {
	// Check document has been opened
	if (document_ == nullptr)
		qFatal ("Poppler: unable to open document \%s\"", qPrintable (filename));
	if (document_->isLocked ())
		qFatal ("Poppler: document is locked \"%s\"", qPrintable (filename));

	// Enable antialiasing, it is better looking
	document_->setRenderHint (Poppler::Document::Antialiasing, true);
	document_->setRenderHint (Poppler::Document::TextAntialiasing, true);

	auto nb_pages = document_->numPages ();
	if (nb_pages <= 0)
		qFatal ("Poppler: no pages in the PDF document \"%s\"", qPrintable (filename));
	pages_.reserve (nb_pages);
	for (int i = 0; i < nb_pages; ++i) {
		auto p = document_->page (i);
		if (p == nullptr)
			qFatal ("Poppler: unable to load page %d in document \"%s\"", i, qPrintable (filename));
		qDebug () << "Page" << i;
		pages_.emplace_back (p);
	}

	discover_document_structure ();
	read_annotations_from_file (filename + "pc");
}

void Document::discover_document_structure (void) {
	// Parse all pages, and create SlideInfo structures each time we "change of slide"
	QString current_slide_label = pages_[0].poppler_page_->label ();
	slides_.emplace_back (0);
	for (int page_index = 0; page_index < nb_pages (); ++page_index) {
		auto & page = pages_[page_index];
		// If label changed since last page, this is the first page of a new slide
		auto label = page.poppler_page_->label ();
		if (label != current_slide_label) {
			// Set "next first page of slide" links of previous pages
			for (int index = slides_.back ().first_page_index (); index < page_index; ++index)
				pages_[index].set_next_slide_first_page (&page);
			// Append to slide list
			slides_.emplace_back (page_index);
			current_slide_label = label;
		}
		// Update slide numbering
		int current_slide_index = slides_.size () - 1;
		page.set_slide_index (current_slide_index);
		// Update transition links with previous page
		if (page_index > 0) {
			auto & previous_page = pages_[page_index - 1];
			if (previous_page.slide_index () == current_slide_index) {
				previous_page.set_next_transition_page (&page);
				page.set_previous_transition_page (&previous_page);
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
				slides_[current_slide_index].append_annotation (line);
			}
		}
	}
}

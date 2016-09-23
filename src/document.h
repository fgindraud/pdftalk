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
#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QImage>
#include <memory> // unique_ptr
#include <poppler-qt5.h>
#include <vector>

class PageInfo {
	/* Describes a pdf document page.
	 * Propagated by non-owning pointers (nullptr == no corresponding page)
	 *
	 * A Page is a PDF document page.
	 * A slide (in beamer at least) can contain multiple pages if there are transitions.
	 * The slide_index represents the slide number of a page (counting from 0).
	 */
private:
	// Page info
	std::unique_ptr<const Poppler::Page> poppler_page_;
	qreal height_for_width_ratio_{0};
	int slide_index_{-1}; // User slide index, counting from 0

	// Related page links (used for the presenter view)
	const PageInfo * next_transition_page_{nullptr};
	const PageInfo * previous_transition_page_{nullptr};
	const PageInfo * next_slide_first_page_{nullptr};

	friend class Document; // For link edition
	void set_slide_index (int index) { slide_index_ = index; }
	void set_next_transition_page (const PageInfo * page) { next_transition_page_ = page; }
	void set_previous_transition_page (const PageInfo * page) { previous_transition_page_ = page; }
	void set_next_slide_first_page (const PageInfo * page) { next_slide_first_page_ = page; }

public:
	PageInfo (Poppler::Page * page) : poppler_page_ (page) {
		auto page_size_dots = poppler_page_->pageSizeF ();
		if (!page_size_dots.isEmpty ())
			height_for_width_ratio_ = page_size_dots.height () / page_size_dots.width ();
	}

	int slide_index (void) const { return slide_index_; }
	const PageInfo * next_transition_page (void) const { return next_transition_page_; }
	const PageInfo * previous_transition_page (void) const { return previous_transition_page_; }
	const PageInfo * next_slide_first_page (void) const { return next_slide_first_page_; }

	qreal height_for_width_ratio (void) const { return height_for_width_ratio_; }
	QSize render_size (QSize box) const {
		// Computes the size we can render page in the given box
		const auto page_size_dots = poppler_page_->pageSizeF ();
		if (page_size_dots.isEmpty ())
			return QSize ();
		const qreal pix_dots_ratio =
		    std::min (static_cast<qreal> (box.width ()) / page_size_dots.width (),
		              static_cast<qreal> (box.height ()) / page_size_dots.height ());
		return (page_size_dots * pix_dots_ratio).toSize ();
	}

	QImage render (QSize box) const {
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
};

class Document {
	/* Represents an opened pdf document, and serve rendering requests.
	 * TODO gather annotations to serve requests (pdfpc file, and text type anotation from poppler)
	 */

private:
	std::unique_ptr<Poppler::Document> document_;
	std::vector<PageInfo> pages_;                   // size() gives page number
	std::vector<int> indexes_of_slide_first_pages_; // size() gives slide number

public:
	explicit Document (const QString & filename) : document_ (Poppler::Document::load (filename)) {
		// Check document has been opened
		if (document_ == nullptr)
			qFatal ("Poppler: unable to open document \%s\"", qPrintable (filename));
		if (document_->isLocked ())
			qFatal ("Poppler: document is locked \"%s\"", qPrintable (filename));

		// Enable antialiasing, it is better looking
		document_->setRenderHint (Poppler::Document::Antialiasing, true);
		document_->setRenderHint (Poppler::Document::TextAntialiasing, true);

		// Create page objects
		auto nb_pages = document_->numPages ();
		if (nb_pages <= 0)
			qFatal ("Poppler: no pages in the PDF document \"%s\"", qPrintable (filename));
		pages_.reserve (nb_pages);
		for (int i = 0; i < nb_pages; ++i) {
			auto p = document_->page (i);
			if (p == nullptr)
				qFatal ("Poppler: unable to load page %d in document \"%s\"", i, qPrintable (filename));
			pages_.emplace_back (p);
		}
		// Determine slide/page structure.
		QString current_slide_label = pages_[0].poppler_page_->label ();
		indexes_of_slide_first_pages_.push_back (0);
		for (int page_index = 0; page_index < nb_pages; ++page_index) {
			auto & page = pages_[page_index];
			// If label changed since last page, this is the first page of a new slide
			auto label = page.poppler_page_->label ();
			if (label != current_slide_label) {
				// Set "next first page of slide" links of previous pages
				for (int index = indexes_of_slide_first_pages_.back (); index < page_index; ++index)
					pages_[index].set_next_slide_first_page (&page);
				// Append to slide list
				indexes_of_slide_first_pages_.push_back (page_index);
				current_slide_label = label;
			}
			// Update slide numbering
			int current_slide_index = indexes_of_slide_first_pages_.size () - 1;
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

	int nb_pages (void) const { return pages_.size (); }
	int nb_slides (void) const { return indexes_of_slide_first_pages_.size (); }

	const PageInfo & page (int page_index) const { return pages_.at (page_index); }
	const PageInfo & first_page_of_slide (int slide_index) const {
		return page (indexes_of_slide_first_pages_.at (slide_index));
	}
};

#endif

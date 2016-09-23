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

using PageIndex = int;  // [0, NbPages[, -1 == no page
using SlideIndex = int; // [0, NbSlides[

class Document {
	/* Represents an opened pdf document, and serve rendering requests.
	 * TODO add sequence info (to next slide/prev slide, to slide N, ...)
	 * TODO gather annotations to serve requests (pdfpc file, and text type anotation from poppler)
	 */

private:
	struct Page {
		/* Struct that describes a pdf document page.
		 * Placed in a vector, where its index indicates the page number.
		 * slide_index indicates the slide number as defined by beamer.
		 */
		std::unique_ptr<const Poppler::Page> page;
		SlideIndex slide_index;
		// TODO add index of page inside slide transition ?
		Page (Poppler::Page * page, SlideIndex slide_index) : page (page), slide_index (slide_index) {}
	};
	std::unique_ptr<Poppler::Document> document_;
	std::vector<Page> pages_; // size() method gives the total number of pages
	SlideIndex nb_slides_;

public:
	explicit Document (const QString & filename) : document_ (Poppler::Document::load (filename)) {
		// Check document has been opened
		if (document_ == nullptr)
			qFatal ("Unable to open document");
		if (document_->isLocked ())
			qFatal ("Document is locked");

		// Enable antialiasing, it is better looking
		document_->setRenderHint (Poppler::Document::Antialiasing, true);
		document_->setRenderHint (Poppler::Document::TextAntialiasing, true);

		// Create page objects
		// Count slides by incrementing when label change
		auto nb_pages = document_->numPages ();
		if (nb_pages <= 0)
			qFatal ("No pages in the document");
		pages_.reserve (nb_pages);
		auto current_slide_index = SlideIndex (-1);
		QString current_slide_label;
		for (auto i = 0; i < nb_pages; ++i) {
			auto p = document_->page (i);
			if (p == nullptr)
				qFatal ("Unable to load page");
			QString label = p->label ();
			if (label != current_slide_label) {
				current_slide_index++;
				current_slide_label = label;
			}
			pages_.emplace_back (p, current_slide_index);
		}
		nb_slides_ = current_slide_index + 1;
	}

	PageIndex nb_pages (void) const { return pages_.size (); }
	SlideIndex nb_slides (void) const { return nb_slides_; }

	SlideIndex slide_index_of_page (PageIndex page_index) const {
		return pages_.at (page_index).slide_index;
	}
	// TODO transition, slide first pages

	QSize render_size (PageIndex index, QSize box) const {
		// Computes the maximum (and thus preferred) size we can render page in the given box
		const auto & page = pages_.at (index).page;
		const auto page_size_dots = page->pageSizeF ();
		if (page_size_dots.isEmpty ())
			return QSize ();
		const qreal pix_dots_ratio =
		    std::min (static_cast<qreal> (box.width ()) / page_size_dots.width (),
		              static_cast<qreal> (box.height ()) / page_size_dots.height ());
		return (page_size_dots * pix_dots_ratio).toSize ();
	}

	QImage render (PageIndex index, QSize box) const {
		// Render the page in the box
		const auto & page = pages_.at (index).page;
		const auto page_size_dots = page->pageSizeF ();
		if (page_size_dots.isEmpty ())
			return QImage ();
		const qreal pix_dots_ratio =
		    std::min (static_cast<qreal> (box.width ()) / page_size_dots.width (),
		              static_cast<qreal> (box.height ()) / page_size_dots.height ());
		const qreal dpi = pix_dots_ratio * 72.0;
		return page->renderToImage (dpi, dpi);
	}
};

#endif

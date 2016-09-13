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

#include <QPixmap>
#include <memory> // unique_ptr
#include <poppler-qt5.h>
#include <vector>

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
		int slide_index;
		// TODO add index of page inside slide transition ?
		Page (Poppler::Page * page, int slide_index) : page (page), slide_index (slide_index) {}
	};
	std::unique_ptr<Poppler::Document> document_;
	std::vector<Page> pages_; // size() method gives the total number of pages
	int nb_slides_;

public:
	Document (const QString & filename) : document_ (Poppler::Document::load (filename)) {
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
		int nb_pages = document_->numPages ();
		if (nb_pages <= 0)
			qFatal ("No pages in the document");
		pages_.reserve (nb_pages);
		int current_slide_index = -1;
		QString current_slide_label;
		for (int i = 0; i < nb_pages; ++i) {
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

	int nb_pages (void) const { return pages_.size (); }
	int nb_slides (void) const { return nb_slides_; }
	int slide_index_of_page (int page_index) const { return pages_.at (page_index).slide_index; }

	QPixmap render (int page_index, QSize box) const {
		const auto & page = pages_.at (page_index).page;
		const auto page_size_dots = page->pageSizeF ();
		const qreal dpi =
		    std::min (static_cast<qreal> (box.width ()) / (page_size_dots.width () / 72.0),
		              static_cast<qreal> (box.height ()) / (page_size_dots.height () / 72.0));
		auto image = page->renderToImage (dpi, dpi);
		if (image.isNull ())
			qFatal ("Unable to render page");
		return QPixmap::fromImage (std::move (image));
	}
};

#endif

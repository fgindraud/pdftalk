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

#include <QFile>
#include <QImage>
#include <QTextStream>
#include <memory> // unique_ptr
#include <poppler-qt5.h>
#include <vector>

#include <QDebug>

/* A presentation (in beamer at least) is a pdf document.
 * A pdf document is flat and composed of pages (vector images).
 * The presentation is however composed of slides, which can each contain one or more pages.
 * More than one page corresponds to internal transitions.
 * They should not be counted in the slide numbering.
 *
 * The PageInfo class is used to represent a pdf page.
 * It is used as a page descriptor structure, and always passed as a const pointer.
 * It stores page info (rendering, sizing, slide number).
 * It also stores links to related pages (next/prev transition, next slide).
 * These links are pointers which are nullptr if the related page doesn't exists.
 *
 * The Document class represents a whole document.
 * It creates and owns PageInfo objects.
 * It also stores Slide specific info.
 * Slide and Page numbering counts from 0.
 *
 * Annotations can come from:
 * - the pdf document, per page (stored in the PageInfo)
 * - the pdfpc text file, per slide (stored in Document::SlideInfo)
 */

class PageInfo {
private:
	// Page info
	std::unique_ptr<const Poppler::Page> poppler_page_;
	qreal height_for_width_ratio_{0};
	QString annotations_; // PDF annotations
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
		// precompute height_for_width_ratio
		auto page_size_dots = poppler_page_->pageSizeF ();
		if (!page_size_dots.isEmpty ())
			height_for_width_ratio_ = page_size_dots.height () / page_size_dots.width ();
		// retrieve text annotations
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
		// DEBUG inspect pages
		{
			qDebug () << "label" << page->label ();
			auto list_of_annotations = poppler_page_->annotations ();
			for (auto a : list_of_annotations) {
				if (a->subType () == Poppler::Annotation::AText) {
					qDebug () << "comment" << a->contents ();
				}
				delete a;
			}
		}
	}

	int slide_index (void) const { return slide_index_; }
	const QString & annotations (void) const { return annotations_; }
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

class SlideInfo {
private:
	int first_page_index_;
	QString annotations_; // Annotations from pdfpc

	friend class Document;
	void append_annotation (const QString & text) {
		annotations_ += text;
		if (!text.endsWith ('\n'))
			annotations_ += '\n';
	}

public:
	SlideInfo (int first_page_index) : first_page_index_ (first_page_index) {}
	int first_page_index (void) const { return first_page_index_; }
	const QString & annotations (void) const { return annotations_; }
};

class Document {
	/* Represents an opened pdf document.
	 * The document is represented as a list of PageInfo objects which represents the document pages.
	 *
	 * Additionnaly it stores info for each slide (group of pages):
	 * - first page, to allow seeking in the presentation.
	 * - pdfpc annotations, which are per slide.
	 */

private:
	std::unique_ptr<Poppler::Document> document_;
	std::vector<PageInfo> pages_;   // size() gives page number
	std::vector<SlideInfo> slides_; // size() gives slide number

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

	int nb_pages (void) const { return pages_.size (); }
	int nb_slides (void) const { return slides_.size (); }

	const PageInfo & page (int page_index) const { return pages_.at (page_index); }
	const SlideInfo & slide (int slide_index) const { return slides_.at (slide_index); }

private:
	void discover_document_structure (void) {
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
	void read_annotations_from_file (const QString & pdfpc_filename) {
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
};

#endif

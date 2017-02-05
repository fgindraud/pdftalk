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

#include "action.h"

#include <QString>
#include <memory> // unique_ptr
#include <poppler-qt5.h>
#include <vector>

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
	std::vector<std::unique_ptr<Action::Base>> actions_;

	// Related page links (used for the presenter view)
	const PageInfo * next_transition_page_{nullptr};
	const PageInfo * previous_transition_page_{nullptr};
	const PageInfo * next_slide_first_page_{nullptr};

public:
	PageInfo (Poppler::Page * page);

	int slide_index (void) const { return slide_index_; }
	const QString & annotations (void) const { return annotations_; }
	const PageInfo * next_transition_page (void) const { return next_transition_page_; }
	const PageInfo * previous_transition_page (void) const { return previous_transition_page_; }
	const PageInfo * next_slide_first_page (void) const { return next_slide_first_page_; }

	qreal height_for_width_ratio (void) const { return height_for_width_ratio_; }
	QSize render_size (QSize box) const;
	QImage render (QSize box) const;

	const Action::Base * on_click (const QPointF & coord) const;

private:
	// Related page links editions by document
	friend class Document;
	void set_slide_index (int index) { slide_index_ = index; }
	void set_next_transition_page (const PageInfo * page) { next_transition_page_ = page; }
	void set_previous_transition_page (const PageInfo * page) { previous_transition_page_ = page; }
	void set_next_slide_first_page (const PageInfo * page) { next_slide_first_page_ = page; }

	// Staged init
	void init_annotations (void);
	void init_actions (void);
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
	explicit Document (const QString & filename);

	int nb_pages (void) const { return pages_.size (); }
	int nb_slides (void) const { return slides_.size (); }

	const PageInfo & page (int page_index) const { return pages_.at (page_index); }
	const SlideInfo & slide (int slide_index) const { return slides_.at (slide_index); }

private:
	// Init stuff
	void discover_document_structure (void);
	void read_annotations_from_file (const QString & pdfpc_filename);
};

#endif

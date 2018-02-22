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
#pragma once

#include <memory>
#include <vector>

#include <QDebug>
#include <QString>

namespace Action {
class Base;
}
namespace Poppler {
class Document;
class Page;
} // namespace Poppler
class PageInfo;
class SlideInfo;

/* A presentation (in beamer at least) is a pdf document.
 * A pdf document is flat and composed of pages (vector images).
 * The presentation is however composed of slides, which can each contain one or more pages.
 * More than one page per slide corresponds to internal transitions.
 * They should not be counted in the slide numbering.
 *
 * The presentation structure is represented by PageInfo and SlideInfo structures.
 * These structs are created and owned by the Document class (represents the document).
 * Both PageInfo and SlideInfo are referenced by const pointer (non owning).
 * They contain navigation link to other PageInfo / SlideInfo.
 * Some navigation links may not be defined (nullptr).
 *
 * PageInfo describes a pdf page.
 * It can perform rendering, stores sizing information, label, and actions.
 *
 * SlideInfo describes a slide (sequence of pages).
 * It stores slide-level annotations.
 *
 * The Document class represents a whole document.
 * It creates and owns PageInfo / SlideInfo objects.
 * Slide and Page numbering counts from 0.
 * A document must contain at least one page.
 *
 * Annotations follow the pdfpc model:
 * - must be stored in a text file, pdfpc format
 * - per slide (not page), stored in Document::SlideInfo
 *
 * Annotations internal to the PDF were tried:
 * - was only per page
 * - no way to generate them without a visual element (icon) -> broke slide layout
 * - support was dropped for simplicity
 */
class PageInfo {
private:
	std::unique_ptr<Poppler::Page> poppler_page_;
	qreal height_for_width_ratio_{0}; // Page aspect ratio, used by GUI
	std::vector<std::unique_ptr<Action::Base>> actions_;

	// Navigation (always defined)
	int index_;                        // PDF document page index (from 0)
	const SlideInfo * slide_{nullptr}; // Pointer to SlideInfo for the slide containing the page
	// Navigation (null at start/end)
	const PageInfo * next_page_{nullptr};
	const PageInfo * previous_page_{nullptr};

public:
	PageInfo (std::unique_ptr<Poppler::Page> page, int index);

	// Non copiable / movable, to safely take references on them
	PageInfo (const PageInfo &) = delete;
	PageInfo (PageInfo &&) = delete;
	PageInfo & operator= (const PageInfo &) = delete;
	PageInfo & operator= (PageInfo &&) = delete;

	int index () const noexcept { return index_; }
	const SlideInfo * slide () const noexcept { return slide_; }
	const PageInfo * next_page () const noexcept { return next_page_; }
	const PageInfo * previous_page () const noexcept { return previous_page_; }

	QString label () const;

	qreal height_for_width_ratio () const noexcept { return height_for_width_ratio_; }
	QSize render_size (const QSize & box) const; // Which render size can fit in box
	QImage render (const QSize & box) const;     // Make render in box

	// Which action is triggered by a click at relative [0,1]x[0,1] coords ?
	const Action::Base * on_click (const QPointF & coord) const;

	// Navigation link setup by document
	void set_slide (const SlideInfo * slide);
	void set_next_page (const PageInfo * page);
	void set_previous_page (const PageInfo * page);
};

QDebug operator<< (QDebug d, const PageInfo * page);

class SlideInfo {
private:
	// Navigation (always defined)
	int index_; // User slide index, counting from 0
	const PageInfo * first_page_{nullptr};
	const PageInfo * last_page_{nullptr};
	// Navigation (null at start/end)
	const SlideInfo * next_slide_{nullptr};
	const SlideInfo * previous_slide_{nullptr};

	QString annotations_; // Annotations from pdfpc

public:
	explicit SlideInfo (int index);

	// Non copiable / movable, to safely take references on them
	SlideInfo (const SlideInfo &) = delete;
	SlideInfo (SlideInfo &&) = delete;
	SlideInfo & operator= (const SlideInfo &) = delete;
	SlideInfo & operator= (SlideInfo &&) = delete;

	int index () const { return index_; }
	const PageInfo * first_page () const { return first_page_; }
	const PageInfo * last_page () const { return last_page_; }
	const SlideInfo * next_slide () const { return next_slide_; }
	const SlideInfo * previous_slide () const { return previous_slide_; }
	const QString & annotations () const { return annotations_; }

	// Setup by document
	void append_annotation (const QString & text);
	void set_first_page (const PageInfo * page);
	void set_last_page (const PageInfo * page);
	void set_next_slide (const SlideInfo * slide);
	void set_previous_slide (const SlideInfo * slide);
};

class Document {
private:
	QString filename_;
	std::unique_ptr<Poppler::Document> document_;
	std::vector<std::unique_ptr<PageInfo>> pages_;
	std::vector<std::unique_ptr<SlideInfo>> slides_;

public:
	// Returns nullptr on error, and prints messages to stderr
	static std::unique_ptr<const Document> open (const QString & filename,
	                                             const QString & pdfpc_filename);

	~Document ();

	int nb_pages () const { return pages_.size (); }
	const PageInfo * page (int page_index) const { return pages_.at (page_index).get (); }

	int nb_slides () const { return slides_.size (); }
	const SlideInfo * slide (int slide_index) const { return slides_.at (slide_index).get (); }

private:
	explicit Document (const QString & filename, std::unique_ptr<Poppler::Document> document);

	// Init: returns false if failed
	bool discover_document_structure ();
	bool read_annotations_from_file (const QString & pdfpc_filename);
};

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
#include <QApplication>
#include <QCommandLineParser>
#include <memory>

#include <poppler-qt5.h>

#include <QDebug>
#include <QLabel>
#include <QPixmap>

/* For screen position:
 * QWidget::window() returns the QWindow
 * QWindow::setScreen()/QWindow::screen()
 * QScreen: get screens and such
 *
 * For fullscreen
 * QWidget::setWindowState with flag
 */

int main (int argc, char * argv[]) {
	QApplication app (argc, argv);
	QCoreApplication::setApplicationName ("pdftalk");
#define XSTR(x) #x
#define STR(x) XSTR (x)
	QCoreApplication::setApplicationVersion (STR (PDFTALK_VERSION));
#undef STR
#undef XSTR
	QApplication::setApplicationDisplayName ("PDFTalk");

	QCommandLineParser parser;
	parser.setApplicationDescription ("PDF presentation tool");
	parser.addHelpOption ();
	parser.addPositionalArgument ("pdf_file", QApplication::translate ("main", "PDF file to open"));
	parser.process (app);

	auto arguments = parser.positionalArguments ();
	if (arguments.size () != 1) {
		parser.showHelp (EXIT_FAILURE);
		Q_UNREACHABLE ();
	}
	QString pdf_filename = arguments[0];

	std::unique_ptr<Poppler::Document> document (Poppler::Document::load (pdf_filename));
	if (!document || document->isLocked ()) {
		qFatal ("Unable to open pdf");
	}

	std::unique_ptr<Poppler::Page> page (document->page (0));
	if (!page)
		qFatal ("Unable to get page 0");

	auto image = page->renderToImage (300, 300);
	if (image.isNull ())
		qFatal ("Render failed");

	qDebug () << image;
	auto pixmap = QPixmap::fromImage (std::move (image));
	qDebug () << pixmap;
	QLabel label;
	label.setPixmap (pixmap);
	label.setScaledContents (true);
	label.show ();

	/* TODO
	 * poppler rendering, see code of okular
	 */

	return app.exec ();
}

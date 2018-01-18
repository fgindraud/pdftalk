Test directory
==============

Provides small examples of beamer presentations, compilable with different configurations.
Observations have been written in the different files for my machine.

Presentations:
* `basic`: only simple beamer constructions (slides, transitions, blocks)
* `labels`: same as basic, sets slide labels

Configurations:
* `default-*`: basic beamer `\note` for introducing notes (hidden / show dual page / show modes)
* `pdfpc`: generate small pdfpc text file
* `pdfcomment`: generate PDF annotation using `pdfcomment`

Compile in one of the presentation folders, using `pdflatex ../<config>.tex`.

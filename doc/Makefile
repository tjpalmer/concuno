MAIN = concuno

# Also: ps2pdf -dEPSCrop file.eps (or even ps2pdf14 -dPDFSETTINGS=/prepress -dEPSCrop ?)

all: $(MAIN).pdf

clean:
	rm -f *.aux *.bbl *.blg *.dvi *.log *.ps *.pdf *.ps.gz *~

$(MAIN)_temp.pdf: $(MAIN).tex # others.tex? any.bib?
	pdflatex $(MAIN)
	#bibtex $(MAIN)
	#pdflatex $(MAIN)
	#pdflatex $(MAIN)
	mv $(MAIN).pdf $(MAIN)_temp.pdf

# Use PDF version 1.4 (minimally, for Adobe Reader 5.0 compliance), embed fonts, etc.
$(MAIN).pdf: $(MAIN)_temp.pdf
	ps2pdf14 -dPDFSETTINGS=/prepress $(MAIN)_temp.pdf $(MAIN).pdf

bib:
	bibtex $(MAIN)

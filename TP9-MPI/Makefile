TEX=pdflatex
TEXs=$(wildcard *.tex)
DVIPDF=dvipdf
all: $(TEXs:.tex=.pdf)

%.pdf: %.tex
	$(TEX) $<
#%.pdf: %.dvi
#	$(DVIPDF) $< $@


clean:
	rm -f *~ *.dvi *.aux *.log
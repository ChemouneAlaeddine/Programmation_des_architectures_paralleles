TEX=rubber -d
all: sujet.pdf
%.pdf: %.tex
	$(TEX) $<
%.ps: %.dvi
	dvips $< -o $@
clean:
	rm *.ps *.dvi *.aux *.log

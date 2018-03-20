TEX=rubber -d
TEXs=$(wildcard *.tex)
DVIPS=dvips
all: $(TEXs:.tex=.pdf)

%.pdf:%.tex
	$(TEX) $<
clean:
	rm -f  *.aux *.dvi *.log *~

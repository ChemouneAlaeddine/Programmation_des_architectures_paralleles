TEX=rubber -d
TEXs=$(wildcard *.tex)

CFLAGS=-g -O2 -Wall
#PROGS=temps mat correction
LDLIBS=-lpthread

all: $(TEXs:.tex=.pdf) $(PROGS) 

clean:
	rm -f $(PROGS) *.aux *.dvi *.log *~

%.pdf: %.tex
	$(TEX) $<

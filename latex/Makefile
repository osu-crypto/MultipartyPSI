MAIN = main
PDF = pdflatex
BIB = bibtex
BIBOPTS = -min-crossrefs=10

all: $(MAIN).pdf

$(MAIN).pdf: *.tex *.bib
	$(PDF) $(MAIN) && $(BIB) $(BIBOPTS) $(MAIN) && $(PDF) $(MAIN) && $(PDF) $(MAIN)

quick: *.tex
	$(PDF) $(MAIN)

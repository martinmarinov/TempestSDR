LATEX=pdflatex --shell-escape
BIBTEX=bibtex

all: acs-dissertation.pdf

acs-dissertation.pdf: 
	$(LATEX) acs-dissertation
	$(BIBTEX) acs-dissertation
	$(LATEX) acs-dissertation
	$(LATEX) acs-dissertation
	$(RM) -f CUni3-eps-converted-to.pdf

clean:
	$(RM) -f acs-dissertation.pdf acs-dissertation.ps acs-dissertation.dvi 
	$(RM) -f *.log *.aux *.toc *.bbl *.lot *.lof *.blg acs-dissertation.synctex.gz


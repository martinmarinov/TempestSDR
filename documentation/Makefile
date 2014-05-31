LATEX=pdflatex --shell-escape
BIBTEX=bibtex

all: acs-dissertation.pdf

remake : clean all

acs-dissertation.pdf: 
	texcount -1 -inc -sum acs-dissertation.tex | awk '{printf "%\x27\d", $$1}' > acs-dissertation.sum
	$(LATEX) acs-dissertation
	$(BIBTEX) acs-dissertation
	$(LATEX) acs-dissertation
	$(LATEX) acs-dissertation
	$(RM) -f CUni3-eps-converted-to.pdf acs-dissertation.sum
	@texcount -1 -inc -sum acs-dissertation.tex | awk '{printf "\n\n----------------------------\nTotal words in dissertation:%\x27 d\n\n", $$1}'

clean:
	$(RM) -f acs-dissertation.pdf acs-dissertation.ps acs-dissertation.dvi 
	$(RM) -f *.log *.aux *.toc *.bbl *.lot *.lof *.blg acs-dissertation.synctex.gz *.sum


.PHONY: all clean run

all: main.pdf

run: main.tex
	make clean
	pdflatex main.tex
	bibtex main
	pdflatex main.tex
	pdflatex main.tex

clean:
	rm -f *.aux *.log *.out *.toc *.lof *.lot *.fdb_latexmk *.fls *.synctex.gz *.bbl *.blg *.bcf *.xml *.run.xml *.pdf
	find . -type f \( -name "*.aux" -o -name "*.log" -o -name "*.out" -o -name "*.toc" -o -name "*.lof" -o -name "*.lot" -o -name "*.fdb_latexmk" -o -name "*.fls" -o -name "*.synctex.gz" \) -delete

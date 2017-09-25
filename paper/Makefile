all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = \
	 gen-graph-value-ordering-heuristics.pdf \
	 gen-graph-restarts.pdf \
	 gen-graph-scatter.pdf \
	 gen-graph-scatter-random.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot $(ADJUSTED_HEATMAP_DATA)
	gnuplot $<
	sed -i -e '19s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

clean :
	rm *.aux *.bbl *.blg *.fdb_latexmk *.fls *.log *.pdf *.data.adjusted gen-graph*


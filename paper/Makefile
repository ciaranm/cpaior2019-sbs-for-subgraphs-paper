all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = \
	 gen-graph-value-ordering-heuristics.pdf \
	 gen-graph-restarts.pdf \
	 gen-graph-scatter.pdf \
	 gen-graph-scatter-random.pdf \
	 gen-graph-others.pdf \
	 gen-graph-others-zoom.pdf \
	 gen-graph-scatter-heuristics.pdf \
	 gen-graph-scatter-nogoods.pdf \
	 gen-graph-scatter-overheads.pdf \
	 gen-graph-restart-policies.pdf \
	 gen-graph-scatter-geometric.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot $(ADJUSTED_HEATMAP_DATA)
	gnuplot $<
	sed -i -e '19s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

clean :
	rm *.aux *.bbl *.blg *.fdb_latexmk *.fls *.log *.pdf *.data.adjusted gen-graph*


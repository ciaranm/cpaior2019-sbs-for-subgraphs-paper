all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' talk

TABLES =

GRAPHS = \
	 gen-graph-others.pdf \
	 gen-graph-others-zoom.pdf \
	 gen-graph-value-ordering-heuristics.pdf \
	 gen-graph-scatter-dds.pdf \
	 gen-graph-random-restarts.pdf \
	 gen-graph-scatter-random.pdf \
	 gen-graph-value-ordering-heuristics-biased.pdf \
	 gen-graph-restarts.pdf \
	 gen-graph-scatter-final.pdf \
	 gen-graph-mcs.pdf \
	 gen-graph-scatter-kdown.pdf \
	 gen-graph-scatter-mcsplit.pdf \
	 gen-graph-parallel.pdf \
	 gen-graph-as-parallel.pdf \
	 gen-graph-scatter-parallel.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot common.gnuplot
	gnuplot $<
	sed -i -e '19,20s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*


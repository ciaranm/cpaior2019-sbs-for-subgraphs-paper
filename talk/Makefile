all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' talk

TABLES =

GRAPHS = \
	gen-graph-value-ordering.pdf \
	gen-graph-value-ordering-unsat.pdf \
	gen-graph-value-ordering-dds.pdf \
	gen-graph-value-ordering-dds-unsat.pdf \
	gen-graph-value-ordering-dds-scatter.pdf \
	gen-graph-sbs.pdf \
	gen-graph-sbs-unsat.pdf \
	gen-graph-sbs-scatter.pdf \
	gen-graph-par.pdf \
	gen-graph-par-scatter.pdf \
	gen-graph-dist.pdf \
	gen-graph-bias-scatter.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.tex : graph-%.gnuplot
	gnuplot $<

gen-graph-%.pdf : graph-%.gnuplot $(DATA)
	gnuplot $<
	sed -i -e '5,20s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*


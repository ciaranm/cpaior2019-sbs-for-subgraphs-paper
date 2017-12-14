all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = \
	 gen-graph-value-ordering-heuristics.pdf \
	 gen-graph-restarts.pdf \
	 gen-graph-scatter-dds.pdf \
	 gen-graph-scatter-random.pdf \
	 gen-graph-others.pdf \
	 gen-graph-others-zoom.pdf \
	 gen-graph-scatter-heuristics.pdf \
	 gen-graph-scatter-by-family.pdf \
	 gen-graph-as.pdf \
	 gen-graph-scatter-degree.pdf \
	 gen-graph-scatter-mcsplit.pdf \
	 gen-graph-scatter-kdown.pdf \
	 gen-graph-mcs.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot common.gnuplot scatter.gnuplot optscatter.gnuplot parula.pal runtimes.data searchtimes.data mcsruntimes.data
	gnuplot $<
	sed -i -e '19,20s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

clean :
	rm *.aux *.bbl *.blg *.fdb_latexmk *.fls *.log *.pdf *.data.adjusted gen-graph*


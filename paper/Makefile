all : tables graphs gen-n-sat.tex gen-n-unsat.tex
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = \
	 gen-graph-value-ordering-heuristics.pdf \
	 gen-graph-restarts.pdf \
	 gen-graph-scatter-dds.pdf \
	 gen-graph-dds.pdf \
	 gen-graph-scatter-random.pdf \
	 gen-graph-scatter-random-goods.pdf \
	 gen-graph-others.pdf \
	 gen-graph-others-zoom.pdf \
	 gen-graph-others-induced.pdf \
	 gen-graph-others-induced-zoom.pdf \
	 gen-graph-scatter-heuristics.pdf \
	 gen-graph-scatter-by-family.pdf \
	 gen-graph-as.pdf \
	 gen-graph-scatter-mcsplit.pdf \
	 gen-graph-scatter-kdown.pdf \
	 gen-graph-mcs.pdf \
	 gen-graph-as-mcs.pdf

gen-n-sat.tex : runtimes.data
	cut -d' ' -f3 < runtimes.data | grep --count 1 | sed -e 's#\([0-9][0-9][0-9]\)$$#,\1#'> $@

gen-n-unsat.tex : runtimes.data
	cut -d' ' -f3 < runtimes.data | grep --count 0 | sed -e 's#\([0-9][0-9][0-9]\)$$#,\1#'> $@

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot common.gnuplot scatter.gnuplot optscatter.gnuplot parula.pal runtimes.data searchtimes.data mcsruntimes.data kdownruntimes.data inducedruntimes.data
	gnuplot $<
	sed -i -e '19,20s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

clean :
	rm *.aux *.bbl *.blg *.fdb_latexmk *.fls *.log *.pdf *.data.adjusted gen-graph*


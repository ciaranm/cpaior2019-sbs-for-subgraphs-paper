# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-mcsplit.tex"

load "common.gnuplot"

set xlabel "McSplit Runtime (ms)"
set ylabel "Biased + Restarts McSplit Runtime (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="<grep -v XXX mcsruntimes.data"
famcol="family"
xcol=mcsplit
ycol=mcsplitbiasedrestarts

load "optscatter.gnuplot"


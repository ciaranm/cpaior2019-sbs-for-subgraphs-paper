# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-scatter-parallel.tex"

load "common.gnuplot"

set xlabel "Sequential Search Time (ms)"
set ylabel "Parallel Search Time (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="../paper/searchtimes.data"
satcol="sat"
famcol="family"
xcol=final
ycol=lubypar

load "scatter.gnuplot"

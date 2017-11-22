# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-nogoods.tex"

load "common.gnuplot"

set xlabel "Degree-Biased + Restarts - Nogoods Search Time (ms)"
set ylabel "Degree-Biased + Restarts Search Time (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="searchtimes.data"
satcol=3
famcol=2
xcol=12
ycol=9

load "scatter.gnuplot"


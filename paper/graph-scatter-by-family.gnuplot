# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-by-family.tex"

load "common.gnuplot"

set title "Biased, with nogoods"
set xlabel "Degree Search Time (ms)"
set ylabel "Biased + Restarts Search Time (ms)" offset 2
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 samplen 1 width -1
set size square

plotfile="searchtimes.data"
satcol="sat"
famcol="family"
xcol=norestarts
ycol=final

load "scatter.gnuplot"

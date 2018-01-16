# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-scatter-dds.tex"

load "common.gnuplot"

set xlabel "Degree Search Time (ms)"
set ylabel "Degree + DDS Search Time (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="../paper/searchtimes.data"
satcol="sat"
famcol="family"
xcol=norestarts
ycol=dds

load "scatter.gnuplot"


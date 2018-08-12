# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-degree.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Random Search Time (ms)"
set ylabel "Degree Search Time (ms)" offset 2
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 samplen 1
set size square

plotfile="searchtimes.data"
satcol="sat"
famcol="family"
xcol=random
ycol=norestarts

load "scatter.gnuplot"

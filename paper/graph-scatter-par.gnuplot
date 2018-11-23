# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.63cm,6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-par.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Sequential Search Time (ms)"
set ylabel "Parallel Search Time (ms)" offset 0.5
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key off
set size square

plotfile="searchtimes.data"
satcol="sat"
famcol="family"
xcol=final
ycol="glasgowbiasedtimer100triggeredthreads36v1"

load "scatter.gnuplot"


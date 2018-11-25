# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.63cm,6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-par.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "SDS (Timer) Search Time (ms)"
set ylabel "32 Threads (Timer) Search Time (ms)" offset 0.5
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key off
set size square

plotfile="searchtimes.data"
satcol="sat"
famcol="family"
xcol=timer
ycol=partimer

load "scatter.gnuplot"


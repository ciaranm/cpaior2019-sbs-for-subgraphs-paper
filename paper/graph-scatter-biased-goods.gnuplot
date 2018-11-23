# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.63cm,6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-biased-goods.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "SBS Search Time (ms)"
set ylabel "SBS $-$ Nogoods Search Time (ms)" offset 0.5
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key off
set size square

plotfile="searchtimes.data"
satcol="sat"
famcol="family"
xcol=norestarts
ycol=biasedrestartsgoods

load "scatter.gnuplot"


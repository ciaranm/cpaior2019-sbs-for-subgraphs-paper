# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.63cm,6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-parconst-nodes.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Parallel C Search Nodes"
set ylabel "Parallel Search Nodes" offset 1.5
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key off
set size square

plotfile="searchsizes.data"
satcol="sat"
famcol="family"
xcol=parconst
ycol=par

load "nodescatter.gnuplot"


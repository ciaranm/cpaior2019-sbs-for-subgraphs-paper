# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-random-goods.tex"

load "parula.pal"
load "common.gnuplot"

set title "Random, without nogoods"
set xlabel "Degree Search Time (ms)"
set ylabel "Random + Restarts Search Time (ms)" offset 2
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
ycol=randomrestartsgoods

load "scatter.gnuplot"


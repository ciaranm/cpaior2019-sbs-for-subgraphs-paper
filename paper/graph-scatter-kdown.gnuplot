# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.63cm,6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-kdown.tex"

load "inferno.pal"
load "common.gnuplot"

set xlabel "DFS k${\\downarrow}$ Runtime (ms)"
set ylabel "SBS k${\\downarrow}$ Runtime (ms)" offset 0.5
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key off
set size square

plotfile="kdownruntimes.data"
famcol="family"
xcol=kdown
ycol=kdownbiasedrestarts

load "optscatter.gnuplot"


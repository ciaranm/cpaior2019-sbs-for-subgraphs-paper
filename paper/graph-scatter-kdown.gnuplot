# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,5.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-scatter-kdown.tex"

load "common.gnuplot"

set xlabel "k${\\downarrow}$ Runtime (ms)"
set ylabel "Biased + Restarts k${\\downarrow}$ Runtime (ms)" offset 2
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


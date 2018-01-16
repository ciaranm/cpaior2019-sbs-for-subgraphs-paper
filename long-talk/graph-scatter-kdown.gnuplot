# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-scatter-kdown.tex"

load "common.gnuplot"

set xlabel "k${\\downarrow}$ Runtime (ms)"
set ylabel "Biased + Restarts k${\\downarrow}$ Runtime (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="../paper/kdownruntimes.data"
famcol="family"
xcol=kdown
ycol=kdownbiasedrestarts

load "optscatter.gnuplot"


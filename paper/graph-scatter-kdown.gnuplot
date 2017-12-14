# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-kdown.tex"

load "common.gnuplot"

set xlabel "k${\\downarrow}$ Runtime (ms)"
set ylabel "Random + Restarts k${\\downarrow}$ Runtime (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="< echo family kdown kdownbiasedrestarts ; echo 1 NaN NaN"
famcol="family"
xcol=kdown
ycol=kdownbiasedrestarts

load "optscatter.gnuplot"


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-scatter-mcsplit.tex"

load "common.gnuplot"

set xlabel "McSplit${\\downarrow}$ Runtime (ms)"
set ylabel "Biased + Restarts McSplit${\\downarrow}$ Runtime (ms)"
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plotfile="../paper/mcsruntimes.data"
famcol="family"
xcol=mcsplitdown
ycol=mcsplitdownbiasedrestarts

load "optscatter.gnuplot"


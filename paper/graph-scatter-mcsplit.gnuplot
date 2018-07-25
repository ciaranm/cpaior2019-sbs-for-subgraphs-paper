# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,5.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-mcsplit.tex"

load "common.gnuplot"

set xlabel "McSplit${\\downarrow}$ Runtime (ms)"
set ylabel "Biased + Restarts McSplit${\\downarrow}$ Runtime (ms)" offset 2
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 samplen 1
set size square

plotfile="<grep -v XXX mcsruntimes.data"
famcol="family"
xcol=mcsplitdown
ycol=mcsplitdownbiasedrestarts

load "optscatter.gnuplot"


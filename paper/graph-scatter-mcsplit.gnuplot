# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 7.5cm,5.1cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-mcsplit.tex"

load "inferno.pal"
load "common.gnuplot"

set xlabel "DFS McSplit${\\downarrow}$ Runtime (ms)"
set ylabel "SBS McSplit${\\downarrow}$ Runtime (ms)" offset 0.5
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 samplen 1 width -3
set size square

plotfile="<grep -v XXX mcsruntimes.data"
famcol="family"
xcol=mcsplitdown
ycol=mcsplitdownbiasedrestarts

load "optscatter.gnuplot"


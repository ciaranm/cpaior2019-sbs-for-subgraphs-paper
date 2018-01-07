# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-mcs.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:]
set key bottom right at 1e6, 100 width -9.5 Left invert

plot \
    "kdownruntimes.data" u (cumx(kdown)):(cumy(kdown)) smooth cumulative w l ti '~~~~Degree' ls 8 dt (18,2), \
    "kdownruntimes.data" u (cumx(kdownbiasedrestarts)):(cumy(kdownbiasedrestarts)) smooth cumulative w l ti '~~~~Biased + Restarts' ls 6 dt (6,2), \
    "kdownruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'k${\downarrow}$:', \
    "mcsruntimes.data" u (cumx(mcsplitdown)):(cumy(mcsplitdown)) smooth cumulative w l ti '~~~~Degree' ls 3 dt (2,2), \
    "mcsruntimes.data" u (cumx(mcsplitdownbiasedrestarts)):(cumy(mcsplitdownbiasedrestarts)) smooth cumulative w l ti '~~~~Biased + Restarts' ls 1, \
    "mcsruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'McSplit${\downarrow}$:'


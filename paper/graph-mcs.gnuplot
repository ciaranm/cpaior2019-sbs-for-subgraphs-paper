# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-mcs.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:]
set key bottom right at 1e6, 20 width -9 Left invert

plot \
    "mcsruntimes.data" u (NaN):(1) smooth cumulative w l ti '~~~~Degree' ls 7 dt (18,2), \
    "mcsruntimes.data" u (NaN):(1) smooth cumulative w l ti '~~~~Biased + Restarts' ls 6 dt (6,2), \
    "mcsruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'k${\downarrow}$:', \
    "<grep -v XXX mcsruntimes.data" u (cumx(mcsplit)):(cumy(mcsplit)) smooth cumulative w l ti '~~~~Degree' ls 2 dt (2,2), \
    "<grep -v XXX mcsruntimes.data" u (cumx(mcsplitbiasedrestarts)):(cumy(mcsplitbiasedrestarts)) smooth cumulative w l ti '~~~~Biased + Restarts' ls 1, \
    "mcsruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'McSplit:'


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 14cm,5.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-zoom.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Sat Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set format y '$~%.0f$'
set yrange [1200:2100]
set key outside right width -8 invert Left

plot \
    "runtimes.data" u (NaN):(NaN) w l ti '~~~~~~~~~~~~~~~~~~~~~' lc rgb "white", \
    "runtimes.data" u (cumx(final)):(cumsaty(final)) smooth cumulative w l notitle ls 1, \
    "runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l notitle ls 2 dt (2,2), \
    "runtimes.data" u (cumx(glasgow2)):(cumsaty(glasgow2)) smooth cumulative w l notitle ls 4 dt (6,2), \
    "runtimes.data" u (cumx(glasgow3)):(cumsaty(glasgow3)) smooth cumulative w l notitle ls 6 dt (18,2), \
    "runtimes.data" u (cumx(pathlad)):(cumsaty(pathlad)) smooth cumulative w l notitle ls 7 dt (6,2,2,2), \
    "runtimes.data" u (cumx(vf2)):(cumsaty(vf2)) smooth cumulative w l notitle ls 8 dt (18,2,2,2)


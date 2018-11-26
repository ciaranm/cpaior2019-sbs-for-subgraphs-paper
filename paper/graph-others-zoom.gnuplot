# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 6.4cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-zoom.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Sat Instances Solved" offset 1
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set format y '$~%.0f$'
set yrange [1200:2100]
set key off

plot \
    "runtimes.data" u (cumx(final)):(cumsaty(final)) smooth cumulative w l notitle ls 1, \
    "runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l notitle ls 2 dt (2,2), \
    "runtimes.data" u (cumx(pathlad)):(cumsaty(pathlad)) smooth cumulative w l notitle ls 3 dt (6,2), \
    "runtimes.data" u (cumx(vf2)):(cumsaty(vf2)) smooth cumulative w l notitle ls 4 dt (18,2), \
    "runtimes.data" u (cumx(ri)):(cumsaty(ri)) smooth cumulative w l notitle ls 5 dt (6,2,2,2)


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.5cm,5.2cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-value-ordering-heuristics.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Sat Instances Solved" offset 0.5
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [1400:2000]
set key bottom right Left

plot \
    "runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l ti 'Degree' ls 1, \
    "runtimes.data" u (cumx(softmax)):(cumsaty(softmax)) smooth cumulative w l ti 'Biased' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(random)):(cumsaty(random)) smooth cumulative w l ti 'Random' ls 4 dt (6,2), \
    "runtimes.data" u (cumx(anti)):(cumsaty(anti)) smooth cumulative w l ti 'Anti' ls 6 dt (18,2), \


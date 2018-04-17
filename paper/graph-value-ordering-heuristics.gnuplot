# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 14cm,5.8cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-value-ordering-heuristics.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Sat Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [1400:2000]
set key outside right Left

plot \
    "runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l ti 'Degree' ls 1, \
    "runtimes.data" u (cumx(softmax)):(cumsaty(softmax)) smooth cumulative w l ti 'Biased' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(random)):(cumsaty(random)) smooth cumulative w l ti 'Random' ls 4 dt (6,2), \
    "runtimes.data" u (cumx(anti)):(cumsaty(anti)) smooth cumulative w l ti 'Anti' ls 6 dt (18,2), \


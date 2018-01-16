# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-parallel.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [13500:14400]
set key bottom right width -0.8 Left

plot \
    "../paper/runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti 'Sequential' ls 1, \
    "../paper/runtimes.data" u (cumx(lubypar)):(cumy(lubypar)) smooth cumulative w l ti 'Parallel' ls 3 dt (2,2)


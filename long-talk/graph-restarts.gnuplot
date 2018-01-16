# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-restarts.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Sat Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [1500:2100]
set key bottom right width -0.8 Left

plot \
    "../paper/runtimes.data" u (cumx(final)):(cumsaty(final)) smooth cumulative w l ti 'Biased + Restarts' ls 1, \
    "../paper/runtimes.data" u (cumx(randomrestarts)):(cumsaty(randomrestarts)) smooth cumulative w l ti 'Random + Restarts' ls 2 dt (2,2), \
    "../paper/runtimes.data" u (cumx(softmax)):(cumsaty(softmax)) smooth cumulative w l ti 'Biased' ls 4 dt (6,2), \
    "../paper/runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l ti 'Degree' ls 6 dt (18,2), \
    "../paper/runtimes.data" u (cumx(random)):(cumsaty(random)) smooth cumulative w l ti 'Random' ls 7 dt (6,2,2,2)


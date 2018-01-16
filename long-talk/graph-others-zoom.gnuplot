# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-others-zoom.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Sat Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set format y '$~%.0f$'
set yrange [1200:2100]
set key bottom right at 1e6, 1450 Left

plot \
    "../paper/runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l ti 'Ours' ls 1, \
    "../paper/runtimes.data" u (cumx(glasgow2)):(cumsaty(glasgow2)) smooth cumulative w l ti 'Glasgow2' ls 4 dt (6,2), \
    "../paper/runtimes.data" u (cumx(glasgow3)):(cumsaty(glasgow3)) smooth cumulative w l ti 'Glasgow3' ls 6 dt (18,2), \
    "../paper/runtimes.data" u (cumx(pathlad)):(cumsaty(pathlad)) smooth cumulative w l ti 'PathLAD' ls 7 dt (6,2,2,2), \
    "../paper/runtimes.data" u (cumx(vf2)):(cumsaty(vf2)) smooth cumulative w l ti 'VF2' ls 8 dt (18,2,2,2)


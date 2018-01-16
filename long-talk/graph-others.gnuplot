# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-others.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:14621]
set ytics add ('$14621$' 14621) add ('' 14000)
set key bottom right at 1e6, 250 Left invert

plot \
    "../paper/runtimes.data" u (cumx(vf2)):(cumy(vf2)) smooth cumulative w l ti 'VF2' ls 8 dt (18,2,2,2), \
    "../paper/runtimes.data" u (cumx(pathlad)):(cumy(pathlad)) smooth cumulative w l ti 'PathLAD' ls 7 dt (6,2,2,2), \
    "../paper/runtimes.data" u (cumx(glasgow3)):(cumy(glasgow3)) smooth cumulative w l ti 'Glasgow3' ls 6 dt (18,2), \
    "../paper/runtimes.data" u (cumx(glasgow2)):(cumy(glasgow2)) smooth cumulative w l ti 'Glasgow2' ls 4 dt (6,2), \
    "../paper/runtimes.data" u (cumx(norestarts)):(cumy(norestarts)) smooth cumulative w l ti 'Ours' ls 1


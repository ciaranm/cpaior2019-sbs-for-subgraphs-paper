# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,5.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:14621]
set ytics add ('$14621$' 14621) add ('' 14000)
set key bottom right at 1e6, 250 width -9 Left invert

plot \
    "runtimes.data" u (cumx(vf2)):(cumy(vf2)) smooth cumulative w l ti 'VF2' ls 8 dt (18,2,2,2), \
    "runtimes.data" u (cumx(pathlad)):(cumy(pathlad)) smooth cumulative w l ti 'PathLAD' ls 7 dt (6,2,2,2), \
    "runtimes.data" u (cumx(glasgow3)):(cumy(glasgow3)) smooth cumulative w l ti 'Glasgow3' ls 6 dt (18,2), \
    "runtimes.data" u (cumx(glasgow2)):(cumy(glasgow2)) smooth cumulative w l ti 'Glasgow2' ls 4 dt (6,2), \
    "runtimes.data" u (cumx(norestarts)):(cumy(norestarts)) smooth cumulative w l ti '~~~~Degree' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti '~~~~Biased + Restarts' ls 1, \
    "runtimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'Algorithm 1:'


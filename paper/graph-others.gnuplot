# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 6.4cm,6.4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Instances Solved" offset 1
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:14621]
set ytics add ('$14621$' 14621) add ('' 14000)
set key above right Left width -2 maxrows 4 height 1

plot \
    "runtimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'Glasgow:', \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti '~~~~SBS' ls 1, \
    "runtimes.data" u (cumx(norestarts)):(cumy(norestarts)) smooth cumulative w l ti '~~~~DFS' ls 2 dt (2,2), \
    "runtimes.data" u (NaN):(NaN) w p lc rgb 'white' ti '~', \
    "runtimes.data" u (cumx(pathlad)):(cumy(pathlad)) smooth cumulative w l ti 'PathLAD' ls 3 dt (6,2), \
    "runtimes.data" u (cumx(vf2)):(cumy(vf2)) smooth cumulative w l ti 'VF2' ls 4 dt (18,2), \
    "runtimes.data" u (cumx(ri)):(cumy(ri)) smooth cumulative w l ti 'RI' ls 5 dt (6,2,2,2), \


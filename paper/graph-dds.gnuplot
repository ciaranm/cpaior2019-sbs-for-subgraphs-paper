# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.5cm,5.2cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-dds.tex"

load "parula.pal"
load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Instances Solved" offset 0.5
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:14621]
set ytics add ('$14621$' 14621) add ('' 14000)
set key bottom right Left

plot \
    "runtimes.data" u (cumx(norestarts)):(cumy(norestarts)) smooth cumulative w l ti 'Degree' ls 1, \
    "runtimes.data" u (cumx(dds)):(cumy(dds)) smooth cumulative w l ti 'DDS' ls 2 dt (2,2)


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8.4cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-induced-zoom.tex"

load "inferno.pal"
load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Induced Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set format y '$~%.0f$'
set yrange [1000:1500]
set key bottom right invert Left width -10

plot \
    "inducedruntimes.data" u (cumx(riinduced)):(cumsaty(riinduced)) smooth cumulative w l notitle ls 7 dt (18,2,2,2), \
    "inducedruntimes.data" u (cumx(vf3induced)):(cumsaty(vf3induced)) smooth cumulative w l notitle ls 6 dt (6,2,2,2), \
    "inducedruntimes.data" u (cumx(vf2induced)):(cumsaty(vf2induced)) smooth cumulative w l notitle ls 4 dt (18,2), \
    "inducedruntimes.data" u (cumx(pathladinduced)):(cumsaty(pathladinduced)) smooth cumulative w l notitle ls 3 dt (6,2), \
    "inducedruntimes.data" u (cumx(norestartsinduced)):(cumsaty(norestartsinduced)) smooth cumulative w l notitle ls 2 dt (2,2), \
    "inducedruntimes.data" u (cumx(finalinduced)):(cumsaty(finalinduced)) smooth cumulative w l notitle ls 1


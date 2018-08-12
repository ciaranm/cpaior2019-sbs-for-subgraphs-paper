# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8.4cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-induced.tex"

load "inferno.pal"
load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Induced Sat Instances Solved"
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set format y '$~%.0f$'
set yrange [0:14621]
set ytics add ('$14621$' 14621) add ('' 14000)
set key bottom right invert Left width -10

plot \
    "inducedruntimes.data" u (cumx(riinduced)):(cumy(riinduced)) smooth cumulative w l ti 'RI' ls 7 dt (18,2,2,2), \
    "inducedruntimes.data" u (cumx(vf3induced)):(cumy(vf3induced)) smooth cumulative w l ti 'VF3' ls 6 dt (6,2,2,2), \
    "inducedruntimes.data" u (cumx(vf2induced)):(cumy(vf2induced)) smooth cumulative w l ti 'VF2' ls 4 dt (18,2), \
    "inducedruntimes.data" u (cumx(pathladinduced)):(cumy(pathladinduced)) smooth cumulative w l ti 'PathLAD' ls 3 dt (6,2), \
    "inducedruntimes.data" u (cumx(norestartsinduced)):(cumy(norestartsinduced)) smooth cumulative w l ti '~~~~Degree' ls 2 dt (2,2), \
    "inducedruntimes.data" u (cumx(finalinduced)):(cumy(finalinduced)) smooth cumulative w l ti '~~~~Biased + Restarts' ls 1, \
    "inducedruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'Algorithm 1:'


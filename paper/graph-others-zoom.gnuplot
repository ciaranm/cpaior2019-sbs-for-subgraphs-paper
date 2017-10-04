# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-zoom.tex"

load "inferno.pal"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set border 3
set grid x y
set xtics nomirror
set ytics nomirror
set xrange [1e3:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [13600:14400]
set key bottom right

plot \
    "runtimes.data" u ($5>=1e6?1e6:$5):($5>=1e6?1e-10:1) smooth cumulative w l ti 'Algorithm 1' ls 1, \
    "runtimes.data" u ($18>=1e6?1e6:$18):($18>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow2' ls 4, \
    "runtimes.data" u ($19>=1e6?1e6:$19):($19>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow3' ls 6, \
    "runtimes.data" u ($16>=1e6?1e6:$16):($16>=1e6?1e-10:1) smooth cumulative w l ti 'PathLAD' ls 7


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-zoom.tex"

load "common.gnuplot"

set xlabel "Runtime (ms)"
set ylabel "Number of Instances Solved"
set xrange [1e3:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [13600:14400]
set key bottom right

plot \
    "runtimes.data" u ($5>=1e6?1e6:$5):($5>=1e6?1e-10:1) smooth cumulative w l ti 'Algorithm 1' ls 1, \
    "runtimes.data" u ($23>=1e6?1e6:$23):($23>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow2' ls 4, \
    "runtimes.data" u ($24>=1e6?1e6:$24):($24>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow3' ls 6, \
    "runtimes.data" u ($21>=1e6?1e6:$21):($21>=1e6?1e-10:1) smooth cumulative w l ti 'PathLAD' ls 7


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-final.tex"

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
    "runtimes.data" u ($10>=1e6?1e6:$10):($10>=1e6?1e-10:1) smooth cumulative w l ti 'Algorithm 1 + Restarts' ls 1 dt ".", \
    "runtimes.data" u ($20>=1e6?1e6:$20):($20>=1e6?1e-10:1) smooth cumulative w l ti 'Algorithm 1 + Restarts, Parallel' ls 1 dt "-", \
    "runtimes.data" u ($24>=1e6?1e6:$24):($24>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow2' ls 4, \
    "runtimes.data" u ($25>=1e6?1e6:$25):($25>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow3' ls 6, \
    "runtimes.data" u ($22>=1e6?1e6:$22):($22>=1e6?1e-10:1) smooth cumulative w l ti 'PathLAD' ls 7


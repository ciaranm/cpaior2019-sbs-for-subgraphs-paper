# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-zoom.tex"

load "paired.pal"

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
    "runtimes.data" u ($5>=1e6?1e6:$5):($5>=1e6?1e-10:1) smooth cumulative w l ti 'Algorithm 1' ls 3, \
    "runtimes.data" u ($16>=1e6?1e6:$16):($16>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow2' ls 3 dt ".", \
    "runtimes.data" u ($17>=1e6?1e6:$17):($17>=1e6?1e-10:1) smooth cumulative w l ti 'Glasgow3' ls 3 dt "-", \
    "runtimes.data" u ($14>=1e6?1e6:$14):($14>=1e6?1e-10:1) smooth cumulative w l ti 'PathLAD' ls 6, \


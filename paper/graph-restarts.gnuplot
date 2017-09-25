# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,4.82cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-restarts.tex"

load "magma.pal"

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
    "runtimes.data" u ($10>=1e6?1e6:$10):($10>=1e6?1e-10:1) smooth cumulative w l ti 'Weighted + Restarts' ls 1 dt ".", \
    "runtimes.data" u ($11>=1e6?1e6:$11):($11>=1e6?1e-10:1) smooth cumulative w l ti 'Random + Restarts' ls 7 dt ".", \
    "runtimes.data" u ($7>=1e6?1e6:$7):($7>=1e6?1e-10:1) smooth cumulative w l ti 'Weighted' ls 1, \
    "runtimes.data" u ($5>=1e6?1e6:$5):($5>=1e6?1e-10:1) smooth cumulative w l ti 'Tailored' ls 5, \
    "runtimes.data" u ($8>=1e6?1e6:$8):($8>=1e6?1e-10:1) smooth cumulative w l ti 'Random' ls 7, \


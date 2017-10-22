# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-restart-policies.tex"

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
    "runtimes.data" u ($10>=1e6?1e6:$10):($10>=1e6?1e-10:1) smooth cumulative w l ls 1 ti 'Luby 666', \
    "runtimes.data" u ($15>=1e6?1e6:$10):($15>=1e6?1e-10:1) smooth cumulative w l ls 4 ti 'Geometric 1.1', \
    "runtimes.data" u ($16>=1e6?1e6:$16):($16>=1e6?1e-10:1) smooth cumulative w l ls 7 ti 'Luby 10', \
    "runtimes.data" u ($17>=1e6?1e6:$17):($17>=1e6?1e-10:1) smooth cumulative w l ls 8 ti 'Luby 100', \
    "runtimes.data" u ($18>=1e6?1e6:$18):($18>=1e6?1e-10:1) smooth cumulative w l ls 9 ti 'luby 1000'


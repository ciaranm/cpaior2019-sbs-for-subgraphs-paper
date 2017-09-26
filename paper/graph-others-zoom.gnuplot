# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 8cm,5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-others-zoom.tex"

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
    "runtimes.data" u ($5>=1e6?1e6:$5):($5>=1e6?1e-10:1) smooth cumulative w l ti 'Tailored' ls 3, \
    "runtimes.data" u ($14>=1e6?1e6:$14):($14>=1e6?1e-10:1) smooth cumulative w l ti 'PathLAD' ls 5, \
    "runtimes.data" u ($13>=1e6?1e6:$13):($13>=1e6?1e-10:1) smooth cumulative w l ti 'LAD' ls 6


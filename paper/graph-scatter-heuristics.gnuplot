# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 7cm,7cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-heuristics.tex"

load "magma.pal"

set xlabel "Degree Runtime (ms)"
set ylabel "Degree-Biased Runtime (ms)"
set border 3
set grid x y
set xtics nomirror
set ytics nomirror
set xrange [1:1e6]
set yrange [1:1e6]
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key right horiz above width -8
set size square

plot \
    "runtimes.data" u ($3==1?NaN:$5>=1e6?1e6:$5):($7>=1e6?1e6:$7) w p ls 3 pt 2 ps 0.7 ti 'Unsatisfiable', \
    "runtimes.data" u ($3==0?NaN:$5>=1e6?1e6:$5):($7>=1e6?1e6:$7) w p ls 6 pt 6 ps 0.4 ti 'Satisfiable', \
    x w l ls 0 notitle


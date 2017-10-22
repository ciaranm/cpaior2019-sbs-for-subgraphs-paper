# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,7cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-geometric.tex"

load "inferno.pal"

set xlabel "Luby 666 Runtime (ms)"
set ylabel "Geometric 1.1 Runtime (ms)"
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
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plot \
    "runtimes.data" u ($3==1?NaN:$10>=1e6?1e6:$10):($15>=1e6?1e6:$15) w p ls 6 pt 2 ps 0.7 ti 'Unsat', \
    "runtimes.data" u ($3==0?NaN:$10>=1e6?1e6:$10):($15>=1e6?1e6:$15) w p ls 2 pt 6 ps 0.4 ti 'Sat', \
    x w l ls 0 notitle


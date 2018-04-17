# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,4.8cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-as-mcs.tex"

load "common.gnuplot"

set format x '%.0f'
set format y '%.0f'

fc(c)=stringcolumn(c)eq"NaN"?timeout:column(c)

set table 'gen-as-mcs-mcsplitdown.data'
plot "<grep -v XXX mcsruntimes.data" u (fc(mcsplitdown)):(fc(mcsplitdown)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-mcs-mcsplitdownbiasedrestarts.data'
plot "<grep -v XXX mcsruntimes.data" u (fc(mcsplitdownbiasedrestarts)):(fc(mcsplitdownbiasedrestarts)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-mcs-kdown.data'
plot "kdownruntimes.data" u (fc(kdown)):(fc(kdown)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-mcs-kdownbiasedrestarts.data'
plot "kdownruntimes.data" u (fc(kdownbiasedrestarts)):(fc(kdownbiasedrestarts)>=1e6?1e-10:1) smooth cumulative

unset table

set xlabel "Runtime (ms)"
set ylabel "Aggregate Speedup"
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:]
unset format y
set grid xtics ytics mytics

set key top left Left

plot \
    '<./asify.sh gen-as-mcs-mcsplitdownbiasedrestarts.data gen-as-mcs-mcsplitdown.data' u 3:($3/$2) w l ls 1 ti "McSplit${\\downarrow}$", \
    '<./asify.sh gen-as-mcs-kdownbiasedrestarts.data gen-as-mcs-kdown.data' u 3:($3/$2) w l ls 7 ti "kdown"


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-as-mcs.tex"

load "common.gnuplot"

set format x '%.0f'
set format y '%.0f'

fc(c)=stringcolumn(c)eq"NaN"?timeout:column(c)

set table 'gen-as-mcs-mcsplit.data'
plot "<grep -v XXX mcsruntimes.data" u (fc(mcsplit)):(fc(mcsplit)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-mcs-mcsplitbiasedrestarts.data'
plot "<grep -v XXX mcsruntimes.data" u (fc(mcsplitbiasedrestarts)):(fc(mcsplitbiasedrestarts)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-mcs-kdown.data'
plot "kdownruntimes.data" u (fc(kdown)):(fc(kdown)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-mcs-kdownbiasedrestarts.data'
plot "kdownruntimes.data" u (fc(kdownbiasedrestarts)):(fc(kdownbiasedrestarts)>=1e6?1e-10:1) smooth cumulative

unset table

set xlabel "Runtime (ms)"
set ylabel "Aggregate Speedup"
set xrange [1e0:1e5]
set logscale x
set format x '$10^{%T}$'
set yrange [0:]
unset format y
set grid xtics ytics mytics

set key top left Left

plot \
    '<./asify.sh gen-as-mcs-mcsplitbiasedrestarts.data gen-as-mcs-mcsplit.data' u 3:($3/$2) w l ls 1 ti "McSplit", \
    '<./asify.sh gen-as-mcs-kdownbiasedrestarts.data gen-as-mcs-kdown.data' u 3:($3/$2) w l ls 4 ti "kdown"


# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 11cm,6.5cm font '\scriptsize' preamble '\usepackage{microtype}'
set output "gen-graph-as-parallel.tex"

load "common.gnuplot"

set format x '%.0f'
set format y '%.0f'

fc(c)=stringcolumn(c)eq"NaN"?timeout:column(c)

set table 'gen-as-final.data'
plot "<sed -n -e '/^[^ ]\\+ [^ ]\\+ [01] /p ' -e '/family/p' ../paper/searchtimes.data" u (fc(final)):(fc(final)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-lubypar.data'
plot "<sed -n -e '/^[^ ]\\+ [^ ]\\+ [01] /p ' -e '/family/p' ../paper/searchtimes.data" u (fc(lubypar)):(fc(lubypar)>=1e6?1e-10:1) smooth cumulative

unset table

set xlabel "Runtime (ms)"
set ylabel "Aggregate Search Speedup"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:]
set ytics 1
unset format y
set grid

set key off

plot \
    '<../paper/asify.sh gen-as-lubypar.data gen-as-final.data' u 3:($3/$2) w l ls 1 ti "Parallel"


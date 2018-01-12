# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,4.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-as.tex"

load "common.gnuplot"

set format x '%.0f'
set format y '%.0f'

fc(c)=stringcolumn(c)eq"NaN"?timeout:column(c)

set table 'gen-as-sequential.data'
plot "<sed -n -e '/^[^ ]\\+ [^ ]\\+ [01] /p ' -e '/family/p' runtimes.data" u (fc(norestarts)):(fc(norestarts)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-random.data'
plot "<sed -n -e '/^[^ ]\\+ [^ ]\\+ [01] /p ' -e '/family/p' runtimes.data" u (fc(random)):(fc(random)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-anti.data'
plot "<sed -n -e '/^[^ ]\\+ [^ ]\\+ [01] /p ' -e '/family/p' runtimes.data" u (fc(anti)):(fc(anti)>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-sequentialinputorderbiasedrestarts.data'
plot "<sed -n -e '/^[^ ]\\+ [^ ]\\+ [01] /p ' -e '/family/p' runtimes.data" u (fc(final)):(fc(final)>=1e6?1e-10:1) smooth cumulative

unset table

set xlabel "Runtime (ms)"
set ylabel "Aggregate Speedup"
set xrange [1e0:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [1:]
unset format y
set logscale y
set grid xtics ytics mytics

set key top left Left width -8

plot \
    '<./asify.sh gen-as-sequentialinputorderbiasedrestarts.data gen-as-sequential.data' u 3:($3/$2) w l ls 1 ti "Biased + Restarts vs Degree", \
    '<./asify.sh gen-as-sequential.data gen-as-random.data' u 3:($3/$2) w l ls 3 ti "Degree vs Random", \
    '<./asify.sh gen-as-random.data gen-as-anti.data' u 3:($3/$2) w l ls 5 ti "Random vs Anti", \
    '<./asify.sh gen-as-sequential.data gen-as-anti.data' u 3:($3/$2) w l ls 6 ti "Degree vs Anti", \
    '<./asify.sh gen-as-sequentialinputorderbiasedrestarts.data gen-as-random.data' u 3:($3/$2) w l ls 7 ti "Biased + Restarts vs Random", \


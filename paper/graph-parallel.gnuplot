# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9.4cm,6.4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-parallel.tex"

load "parula.pal"
load "common.gnuplot"

solvedfinal=-1
solvedpar=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(isfail(final)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedpar=solvedpar+(isfail(par)?0:1)) smooth cumulative
unset table

set table "gen-as-runtimes-par.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(par)):(isfail(par)?0:1) smooth cumulative
unset table

parthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-par.data" u 1:(parthreshold=($2>=solvedfinal&&valid(1)&&$1<parthreshold)?(sprintf("%d",$1)):(parthreshold))
unset table

set xlabel "Runtime (ms)"
set ylabel "Instances Solved" offset 1
set xrange [1e3:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [13700:14621]
set ytics add ('$14621$' 14621) add ('' 14600)
set key bottom right Left width -4

set arrow 1 from 1e6, solvedfinal to parthreshold, solvedfinal front
set label 1 left at 1e6, solvedfinal "".sprintf("$%.1f{\\times}$", (1.0e6 / parthreshold))

plot \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti 'Sequential' ls 1, \
    "runtimes.data" u (cumx(par)):(cumy(par)) smooth cumulative w l ti 'Parallel' ls 2 dt (2,2)


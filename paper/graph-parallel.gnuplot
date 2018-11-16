# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9.4cm,6.4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-parallel.tex"

load "parula.pal"
load "common.gnuplot"

solvedfinal=-1
solvedpar=-1
solvedparconst=-1
solvedparconsttick=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(isfail(final)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedpar=solvedpar+(isfail(par)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedparconst=solvedparconst+(isfail(parconst)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedparconsttick=solvedparconsttick+(isfail(parconsttick)?0:1)) smooth cumulative
unset table

set table "gen-as-runtimes-par.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(par)):(isfail(par)?0:1) smooth cumulative
unset table

set table "gen-as-runtimes-parconst.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(parconst)):(isfail(parconst)?0:1) smooth cumulative
unset table

set table "gen-as-runtimes-parconsttick.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(parconsttick)):(isfail(parconsttick)?0:1) smooth cumulative
unset table

parthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-par.data" u 1:(parthreshold=($2>=solvedfinal&&valid(1)&&$1<parthreshold)?(sprintf("%d",$1)):(parthreshold))
unset table

parconstthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-parconst.data" u 1:(parconstthreshold=($2>=solvedfinal&&valid(1)&&$1<parconstthreshold)?(sprintf("%d",$1)):(parconstthreshold))
unset table

parconsttickthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-parconsttick.data" u 1:(parconsttickthreshold=($2>=solvedfinal&&valid(1)&&$1<parconsttickthreshold)?(sprintf("%d",$1)):(parconsttickthreshold))
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
set arrow 2 from parthreshold, solvedfinal to parconstthreshold, solvedfinal front
set arrow 3 from parconstthreshold, solvedfinal to parconsttickthreshold, solvedfinal front
set label 1 left at 1e6, solvedfinal "".sprintf("$%.1f{\\times}$, $%.1f{\\times}$, $%.1f{\\times}$", \
    (1.0e6 / (0.0+parthreshold)), (1.0e6 / (0.0+parconstthreshold)), (1.0e6 / (0.0+parconsttickthreshold)))

plot \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti 'Sequential' ls 1, \
    "runtimes.data" u (cumx(par)):(cumy(par)) smooth cumulative w l ti 'Parallel' ls 3 dt (2,2), \
    "runtimes.data" u (cumx(parconst)):(cumy(parconst)) smooth cumulative w l ti 'Parallel C' ls 5 dt (6,2), \
    "runtimes.data" u (cumx(parconsttick)):(cumy(parconsttick)) smooth cumulative w l ti 'Parallel TC' ls 7 dt (18,2)


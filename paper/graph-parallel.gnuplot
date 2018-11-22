# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9.4cm,6.4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-parallel.tex"

load "parula.pal"
load "common.gnuplot"

solvedfinal=-1
solvedpar=-1
solvedparconst=-1
solvedparconsttick=-1
solveddist=-1
lastfinal=-1
lastparconsttick=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(isfail(final)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedpar=solvedpar+(isfail(par)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(parconst)):(solvedparconst=solvedparconst+(isfail(parconst)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(parconsttick)):(solvedparconsttick=solvedparconsttick+(isfail(parconsttick)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(dist5)):(solveddist=solveddist+(isfail(dist5)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(final)):(lastfinal=(isfail(final)||cumx(final)<lastfinal?lastfinal:cumx(final))) smooth cumulative
plot "runtimes.data" u (cumx(parconsttick)):(lastparconsttick=(isfail(parconsttick)||cumx(parconsttick)<lastparconsttick?lastparconsttick:cumx(parconsttick))) smooth cumulative
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

set table "gen-as-runtimes-dist.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(dist5)):(isfail(dist5)?0:1) smooth cumulative
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

distthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist.data" u 1:(distthreshold=($2>=solvedfinal&&valid(1)&&$1<distthreshold)?(sprintf("%d",$1)):(distthreshold))
unset table

parconsttickthreshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-parconsttick.data" u 1:(parconsttickthreshold2=($2>=solvedparconsttick&&valid(1)&&$1<parconsttickthreshold2)?(sprintf("%d",$1)):(parconsttickthreshold2))
unset table

distthreshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist.data" u 1:(distthreshold2=($2>=solvedparconsttick&&valid(1)&&$1<distthreshold2)?(sprintf("%d",$1)):(distthreshold2))
unset table

set xlabel "Runtime (ms)"
set ylabel "Instances Solved" offset 1
set xrange [1e3:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [13700:14621]
set ytics add ('$14621$' 14621) add ('' 14600)
set key bottom right Left width -4

set arrow 1 from lastfinal, solvedfinal to parthreshold, solvedfinal front
set arrow 2 from parthreshold, solvedfinal to parconstthreshold, solvedfinal front
set arrow 3 from parconstthreshold, solvedfinal to parconsttickthreshold, solvedfinal front
set arrow 4 from parconsttickthreshold, solvedfinal to distthreshold, solvedfinal front
set label 5 left at 1e6, solvedfinal "".sprintf("$%.1f{\\times}$, $%.1f{\\times}$, $%.1f{\\times}$, $%.1f{\\times}$", \
    (lastfinal / (0.0+parthreshold)), (lastfinal / (0.0+parconstthreshold)), (lastfinal / (0.0+parconsttickthreshold)), (lastfinal / (0.0+distthreshold)))

set arrow 6 from lastparconsttick, solvedparconsttick to distthreshold2, solvedparconsttick front
set label 6 left at 1e6, solvedparconsttick "".sprintf("$%.1f{\\times}$", (lastparconsttick / (0.0+distthreshold2)))

plot \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti 'Sequential' ls 1, \
    "runtimes.data" u (cumx(par)):(cumy(par)) smooth cumulative w l ti 'Parallel' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(parconst)):(cumy(parconst)) smooth cumulative w l ti 'Parallel C' ls 4 dt (6,2), \
    "runtimes.data" u (cumx(parconsttick)):(cumy(parconsttick)) smooth cumulative w l ti 'Parallel TC' ls 5 dt (18,2), \
    "runtimes.data" u (cumx(dist5)):(cumy(dist5)) smooth cumulative w l ti 'Distributed 5' ls 7 dt (6,2,2,2), \
    "runtimes.data" u (cumx("glasgowbiasedrestartsconstant5400triggerednr32mpih5x2t18")):(cumy("glasgowbiasedrestartsconstant5400triggerednr32mpih5x2t18")) smooth cumulative w l ti 'Distributed 5R32' ls 8 dt (18,2,2,2)


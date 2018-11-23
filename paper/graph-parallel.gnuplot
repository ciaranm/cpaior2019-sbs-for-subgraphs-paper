# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9.4cm,6.4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-parallel.tex"

load "parula.pal"
load "common.gnuplot"

solvedfinal=-1
solvedtimer=-1
solvedpar=-1
solvedpartimer=-1
solveddist=-1
lastfinal=-1
lasttimer=-1
lastpartimer=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(isfail(final)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedpar=solvedpar+(isfail(par)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(timer)):(solvedtimer=solvedtimer+(isfail(timer)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(partimer)):(solvedpartimer=solvedpartimer+(isfail(partimer)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(dist5)):(solveddist=solveddist+(isfail(dist5)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(final)):(lastfinal=(isfail(final)||cumx(final)<lastfinal?lastfinal:cumx(final))) smooth cumulative
plot "runtimes.data" u (cumx(timer)):(lasttimer=(isfail(timer)||cumx(timer)<lasttimer?lasttimer:cumx(timer))) smooth cumulative
plot "runtimes.data" u (cumx(partimer)):(lastpartimer=(isfail(partimer)||cumx(partimer)<lastpartimer?lastpartimer:cumx(partimer))) smooth cumulative
unset table

set table "gen-as-runtimes-par.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(par)):(isfail(par)?0:1) smooth cumulative
unset table

set table "gen-as-runtimes-partimer.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(partimer)):(isfail(partimer)?0:1) smooth cumulative
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

partimerthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-partimer.data" u 1:(partimerthreshold=($2>=solvedtimer&&valid(1)&&$1<partimerthreshold)?(sprintf("%d",$1)):(partimerthreshold))
unset table

distthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist.data" u 1:(distthreshold=($2>=solvedtimer&&valid(1)&&$1<distthreshold)?(sprintf("%d",$1)):(distthreshold))
unset table

distthreshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist.data" u 1:(distthreshold2=($2>=solvedpartimer&&valid(1)&&$1<distthreshold2)?(sprintf("%d",$1)):(distthreshold2))
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
set label 1 left at 1e6, solvedfinal "".sprintf("\\raisebox{-0.1cm}{$%.1f{\\times}$}", (lastfinal / (0.0+parthreshold)))

set arrow 2 from lasttimer, solvedtimer to partimerthreshold, solvedtimer front
set arrow 3 from partimerthreshold, solvedtimer to distthreshold, solvedtimer front
set label 2 left at 1e6, solvedtimer "".sprintf("\\raisebox{0.1cm}{$%.1f{\\times}, %.1f{\\times}$}", \
    (lasttimer / (0.0+partimerthreshold)), (lasttimer / (0.0+distthreshold)))

set arrow 4 from lastpartimer, solvedpartimer to distthreshold2, solvedpartimer front
set label 4 left at 1e6, solvedpartimer "".sprintf("\\raisebox{0.0cm}{$%.1f{\\times}$}", \
    (lastpartimer / (0.0+distthreshold2)))

plot \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti 'Sequential' ls 1, \
    "runtimes.data" u (cumx(par)):(cumy(par)) smooth cumulative w l ti 'Parallel' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(timer)):(cumy(timer)) smooth cumulative w l ti 'Timer' ls 4 dt (6,2), \
    "runtimes.data" u (cumx(partimer)):(cumy(partimer)) smooth cumulative w l ti 'Parallel Timer' ls 5 dt (18,2), \
    "runtimes.data" u (cumx(dist5)):(cumy(dist5)) smooth cumulative w l ti 'Distributed 5' ls 7 dt (6,2,2,2), \


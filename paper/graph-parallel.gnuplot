# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 10.0cm,6.4cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-parallel.tex"

load "parula.pal"
load "common.gnuplot"

solvedfinal=0
solvedtimer=0
solvedpar=0
solvedpartimer=0
solveddist5=0
solveddist10=0
lastfinal=-1
lasttimer=-1
lastpartimer=-1
lastdist5=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(isfail(final)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedpar=solvedpar+(isfail(par)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(timer)):(solvedtimer=solvedtimer+(isfail(timer)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(partimer)):(solvedpartimer=solvedpartimer+(isfail(partimer)?0:1)) smooth cumulative
plot "runtimes.data" u (cumminx(dist5)):(solveddist5=solveddist5+(isfail(dist5)?0:1)) smooth cumulative
plot "runtimes.data" u (cumminx(dist10)):(solveddist10=solveddist10+(isfail(dist10)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(final)):(lastfinal=(isfail(final)||cumx(final)<lastfinal?lastfinal:cumx(final))) smooth cumulative
plot "runtimes.data" u (cumx(timer)):(lasttimer=(isfail(timer)||cumx(timer)<lasttimer?lasttimer:cumx(timer))) smooth cumulative
plot "runtimes.data" u (cumx(partimer)):(lastpartimer=(isfail(partimer)||cumx(partimer)<lastpartimer?lastpartimer:cumx(partimer))) smooth cumulative
plot "runtimes.data" u (cumminx(dist5)):(lastdist5=(isfail(dist5)||cumminx(dist5)<lastdist5?lastdist5:cumminx(dist5))) smooth cumulative
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

set table "gen-as-runtimes-dist5.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumminx(dist5)):(isfail(dist5)?0:1) smooth cumulative
unset table

set table "gen-as-runtimes-dist10.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumminx(dist10)):(isfail(dist10)?0:1) smooth cumulative
unset table

parthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-par.data" u 1:(parthreshold=($2>=solvedfinal&&valid(1)&&$1<parthreshold)?(sprintf("%d",$1)):(parthreshold))
unset table

partimerthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-partimer.data" u 1:(partimerthreshold=($2>=solvedtimer&&valid(1)&&$1<partimerthreshold)?(sprintf("%d",$1)):(partimerthreshold))
unset table

dist5threshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist5.data" u 1:(dist5threshold=($2>=solvedtimer&&valid(1)&&$1<dist5threshold)?(sprintf("%d",$1)):(dist5threshold))
unset table

dist10threshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist10.data" u 1:(dist10threshold=($2>=solvedtimer&&valid(1)&&$1<dist10threshold)?(sprintf("%d",$1)):(dist10threshold))
unset table

dist5threshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist5.data" u 1:(dist5threshold2=($2>=solvedpartimer&&valid(1)&&$1<dist5threshold2)?(sprintf("%d",$1)):(dist5threshold2))
unset table

dist10threshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist10.data" u 1:(dist10threshold2=($2>=solvedpartimer&&valid(1)&&$1<dist10threshold2)?(sprintf("%d",$1)):(dist10threshold2))
unset table

dist10threshold3=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist10.data" u 1:(dist10threshold3=($2>=solveddist5&&valid(1)&&$1<dist10threshold3)?(sprintf("%d",$1)):(dist10threshold3))
unset table

set xlabel "Runtime (ms)"
set ylabel "Instances Solved" offset 1
set xrange [5e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [13600:14500]
set key bottom right Left width -4

set arrow 1 from lastfinal, solvedfinal to parthreshold, solvedfinal front
set label 1 left at 1e6, solvedfinal "".sprintf("\\raisebox{-0.075cm}{$%.1f{\\times}$}", (lastfinal / (0.0+parthreshold)))

set arrow 2 from lasttimer, solvedtimer to partimerthreshold, solvedtimer front
set arrow 3 from partimerthreshold, solvedtimer to dist5threshold, solvedtimer front
set arrow 4 from dist5threshold, solvedtimer to dist10threshold, solvedtimer front
set label 2 left at 1e6, solvedtimer "".sprintf("\\raisebox{0.1cm}{$%.1f{\\times}, %.1f{\\times}, %.1f{\\times}$}", \
    (lasttimer / (0.0+partimerthreshold)), (lasttimer / (0.0+dist5threshold)), (lasttimer / (0.0+dist10threshold)))

set arrow 5 from lastpartimer, solvedpartimer to dist5threshold2, solvedpartimer front
set arrow 6 from dist5threshold2, solvedpartimer to dist10threshold2, solvedpartimer front
set label 5 left at 1e6, solvedpartimer "".sprintf("\\raisebox{0.0cm}{$%.1f{\\times}$, $%.1f{\\times}$}", \
    (lastpartimer / (0.0+dist5threshold2)), (lastpartimer / (0.0+dist10threshold2)))

set arrow 7 from lastdist5, solveddist5 to dist10threshold3, solveddist5 front
set label 7 left at 1e6, solveddist5 "".sprintf("\\raisebox{0.1cm}{$%.1f{\\times}$}", \
    (lastdist5 / (0.0+dist10threshold3)))

plot \
    "runtimes.data" u (cumx(final)):(cumy(final)) smooth cumulative w l ti 'SBS (Luby)' ls 1, \
    "runtimes.data" u (cumx(timer)):(cumy(timer)) smooth cumulative w l ti 'SBS (Timer)' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(par)):(cumy(par)) smooth cumulative w l ti '32 Threads (Luby)' ls 3 dt (6,2), \
    "runtimes.data" u (cumx(partimer)):(cumy(partimer)) smooth cumulative w l ti '32 Threads (Timer)' ls 4 dt (18,2), \
    "runtimes.data" u (cumminx(dist5)):(cumy(dist5)) smooth cumulative w l ti '5 Hosts (Timer)' ls 5 dt (6,2,2,2), \
    "runtimes.data" u (cumminx(dist10)):(cumy(dist10)) smooth cumulative w l ti '10 Hosts (Timer)' ls 6 dt (18,2,2,2), \

print "grepme"
print solvedfinal
print lastfinal
print parthreshold


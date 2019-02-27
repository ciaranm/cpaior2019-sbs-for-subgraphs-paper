# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 12.5cm,5.8cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-parallel.tex"

load "parula.pal"
load "common.gnuplot"

timerlong="glasgowbiasedtimer100long"
finallong="glasgowbiasedlong"

isfaillong(x)=(stringcolumn(x) eq "NaN" || column(x) >= 1e7)
cumxlong(x)=(isfaillong(x) ? 1e7 : column(x))
cumylong(x)=(isfaillong(x) ? 1e-10 : 1)

solvedfinal=0
solvedfinallong=0
solvedtimer=0
solvedtimerlong=0
solvedpar=0
solvedpartimer=0
solveddist5=0
solveddist10=0
solveddist20=0
lastfinal=-1
lastfinallong=-1
solvedfinallong=0
lasttimer=-1
lasttimerlong=-1
lastpartimer=-1
lastdist5=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(isfail(final)?0:1)) smooth cumulative
plot "runtimes.data" u (cumxlong(finallong)):(solvedfinallong=solvedfinallong+(isfaillong(finallong)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(par)):(solvedpar=solvedpar+(isfail(par)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(timer)):(solvedtimer=solvedtimer+(isfail(timer)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(timerlong)):(solvedtimerlong=solvedtimerlong+(isfaillong(timerlong)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(partimer)):(solvedpartimer=solvedpartimer+(isfail(partimer)?0:1)) smooth cumulative
plot "runtimes.data" u (cumminx(dist5)):(solveddist5=solveddist5+(isfail(dist5)?0:1)) smooth cumulative
plot "runtimes.data" u (cumminx(dist10)):(solveddist10=solveddist10+(isfail(dist10)?0:1)) smooth cumulative
plot "runtimes.data" u (cumminx(dist20)):(solveddist20=solveddist20+(isfail(dist20)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(final)):(lastfinal=(isfail(final)||cumx(final)<lastfinal?lastfinal:cumx(final))) smooth cumulative
plot "runtimes.data" u (cumxlong(finallong)):(lastfinallong=(isfaillong(finallong)||cumxlong(finallong)<lastfinallong?lastfinallong:cumxlong(finallong))) smooth cumulative
plot "runtimes.data" u (cumx(timer)):(lasttimer=(isfail(timer)||cumx(timer)<lasttimer?lasttimer:cumx(timer))) smooth cumulative
plot "runtimes.data" u (cumxlong(timerlong)):(lasttimerlong=(isfaillong(timerlong)||cumxlong(timerlong)<lasttimerlong?lasttimerlong:cumxlong(timerlong))) smooth cumulative
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

set table "gen-as-runtimes-dist20.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumminx(dist20)):(isfail(dist20)?0:1) smooth cumulative
unset table

parthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-par.data" u 1:(parthreshold=($2>=solvedfinal&&valid(1)&&$1<parthreshold)?(sprintf("%d",$1)):(parthreshold))
unset table

parthresholdlong=1e6
set table "/dev/null"
plot "gen-as-runtimes-par.data" u 1:(parthresholdlong=($2>=solvedfinallong&&valid(1)&&$1<parthresholdlong)?(sprintf("%d",$1)):(parthresholdlong))
unset table

partimerthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-partimer.data" u 1:(partimerthreshold=($2>=solvedtimer&&valid(1)&&$1<partimerthreshold)?(sprintf("%d",$1)):(partimerthreshold))
unset table

partimerthresholdlong=1e6
set table "/dev/null"
plot "gen-as-runtimes-partimer.data" u 1:(partimerthresholdlong=($2>=solvedtimerlong&&valid(1)&&$1<partimerthresholdlong)?(sprintf("%d",$1)):(partimerthresholdlong))
unset table

dist5threshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist5.data" u 1:(dist5threshold=($2>=solvedtimer&&valid(1)&&$1<dist5threshold)?(sprintf("%d",$1)):(dist5threshold))
unset table

dist10threshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist10.data" u 1:(dist10threshold=($2>=solvedtimer&&valid(1)&&$1<dist10threshold)?(sprintf("%d",$1)):(dist10threshold))
unset table

dist20threshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist20.data" u 1:(dist20threshold=($2>=solvedtimer&&valid(1)&&$1<dist20threshold)?(sprintf("%d",$1)):(dist20threshold))
unset table

dist5thresholdlong=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist5.data" u 1:(dist5thresholdlong=($2>=solvedtimerlong&&valid(1)&&$1<dist5thresholdlong)?(sprintf("%d",$1)):(dist5thresholdlong))
unset table

dist10thresholdlong=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist10.data" u 1:(dist10thresholdlong=($2>=solvedtimerlong&&valid(1)&&$1<dist10thresholdlong)?(sprintf("%d",$1)):(dist10thresholdlong))
unset table

dist20thresholdlong=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist20.data" u 1:(dist20thresholdlong=($2>=solvedtimerlong&&valid(1)&&$1<dist20thresholdlong)?(sprintf("%d",$1)):(dist20thresholdlong))
unset table

dist5threshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist5.data" u 1:(dist5threshold2=($2>=solvedpartimer&&valid(1)&&$1<dist5threshold2)?(sprintf("%d",$1)):(dist5threshold2))
unset table

dist10threshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist10.data" u 1:(dist10threshold2=($2>=solvedpartimer&&valid(1)&&$1<dist10threshold2)?(sprintf("%d",$1)):(dist10threshold2))
unset table

dist20threshold2=1e6
set table "/dev/null"
plot "gen-as-runtimes-dist20.data" u 1:(dist20threshold2=($2>=solvedpartimer&&valid(1)&&$1<dist20threshold2)?(sprintf("%d",$1)):(dist20threshold2))
unset table

set xlabel "Runtime (ms)"
set ylabel "Instances Solved" offset 1
set xrange [1e3:1e7]
set logscale x
set format x '$10^{%T}$'
set yrange [14200:14450]
set key outside right bottom width -7

set arrow 1 from lastfinal, solvedfinal to parthreshold, solvedfinal front
set label 1 left at 1e7, solvedfinal "".sprintf("$%.1f{\\times}$", (lastfinal / (0.0+parthreshold)))

set arrow 2 from lasttimer, solvedtimer to partimerthreshold, solvedtimer front
set arrow 3 from partimerthreshold, solvedtimer to dist5threshold, solvedtimer front
set arrow 4 from dist5threshold, solvedtimer to dist10threshold, solvedtimer front
set arrow 5 from dist5threshold, solvedtimer to dist20threshold, solvedtimer front
set label 2 left at 1e7, solvedtimer "".sprintf("$%.1f{\\times}, %.1f{\\times}, %.1f{\\times}, %.1f{\\times}$", \
    (lasttimer / (0.0+partimerthreshold)), (lasttimer / (0.0+dist5threshold)), (lasttimer / (0.0+dist10threshold)), \
    (lasttimer / (0.0+dist20threshold))) offset character 0, character 0

set arrow 6 from lastpartimer, solvedpartimer to dist5threshold2, solvedpartimer front
set arrow 7 from dist5threshold2, solvedpartimer to dist10threshold2, solvedpartimer front
set arrow 8 from dist10threshold2, solvedpartimer to dist20threshold2, solvedpartimer front
set label 6 left at 1e7, solvedpartimer "".sprintf("$%.1f{\\times}, %.1f{\\times}, %.1f{\\times}$", \
    (lastpartimer / (0.0+dist5threshold2)), (lastpartimer / (0.0+dist10threshold2)), (lastpartimer / (0.0+dist20threshold2))) \
    offset character 0, character 0.5

set arrow 9 from lasttimerlong, solvedtimerlong to partimerthresholdlong, solvedtimerlong front
set arrow 10 from partimerthresholdlong, solvedtimerlong to dist5thresholdlong, solvedtimerlong front
set arrow 11 from dist5thresholdlong, solvedtimerlong to dist10thresholdlong, solvedtimerlong front
set arrow 12 from dist10thresholdlong, solvedtimerlong to dist20thresholdlong, solvedtimerlong front
set label 9 left at 1e7, solvedtimerlong "".sprintf("$%.1f{\\times}, %.1f{\\times}, %.1f{\\times}, %.1f{\\times}$", \
    (lasttimerlong / (0.0+partimerthresholdlong)), \
    (lasttimerlong / (0.0+dist5thresholdlong)), \
    (lasttimerlong / (0.0+dist10thresholdlong)), \
    (lasttimerlong / (0.0+dist20thresholdlong)))

set arrow 13 from lastfinallong, solvedfinallong to parthresholdlong, solvedfinallong front
set label 13 left at 1e7, solvedfinallong "".sprintf("$%.1f{\\times}$", (lastfinallong / (0.0+parthresholdlong)))

plot \
    "runtimes.data" u (cumxlong(finallong)):(cumylong(finallong)) smooth cumulative w l ti 'SBS (Luby)' ls 1, \
    "runtimes.data" u (cumxlong(timerlong)):(cumylong(timerlong)) smooth cumulative w l ti 'SBS (Timer)' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(par)):(cumy(par)) smooth cumulative w l ti '32 Threads (Luby)' ls 3 dt (6,2), \
    "runtimes.data" u (cumx(partimer)):(cumy(partimer)) smooth cumulative w l ti '32 Threads (Timer)' ls 4 dt (18,2), \
    "runtimes.data" u (cumminx(dist5)):(cumy(dist5)) smooth cumulative w l ti '5 Hosts (Timer)' ls 5 dt (6,2,2,2), \
    "runtimes.data" u (cumminx(dist10)):(cumy(dist10)) smooth cumulative w l ti '10 Hosts (Timer)' ls 6 dt (18,2,2,2), \
    "runtimes.data" u (cumminx(dist20)):(cumy(dist20)) smooth cumulative w l ti '20 Hosts (Timer)' ls 7 dt (18,2,6,2), \

print "grepme"
print solvedtimerlong
print lasttimerlong


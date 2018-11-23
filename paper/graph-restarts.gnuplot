# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 5.5cm,5.2cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-restarts.tex"

load "parula.pal"
load "common.gnuplot"

solvedfinal=-1
solvednorestarts=-1
set table "/dev/null"
plot "runtimes.data" u (cumx(norestarts)):(solvednorestarts=solvednorestarts+(stringcolumn("sat")ne"1"||isfail(norestarts)?0:1)) smooth cumulative
plot "runtimes.data" u (cumx(final)):(solvedfinal=solvedfinal+(stringcolumn("sat")ne"1"||isfail(final)?0:1)) smooth cumulative
unset table

set table "gen-as-runtimes-norestarts-sat.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(norestarts)):(stringcolumn("sat")ne"1"||isfail(norestarts)?0:1) smooth cumulative
unset table

set table "gen-as-runtimes-final-sat.data"
set format x '%.0f'
set format x '%.0f'
plot "runtimes.data" u (cumx(final)):(stringcolumn("sat")ne"1"||isfail(final)?0:1) smooth cumulative
unset table

lastnorestarts=0
lastnorestartsvalue=0
set table "/dev/null"
plot "gen-as-runtimes-norestarts-sat.data" u 1:(valid(2)?lastnorestartsvalue=$2:NaN)
plot "gen-as-runtimes-norestarts-sat.data" u 1:(lastnorestarts=(valid(1)&&lastnorestarts==0&&$2==lastnorestartsvalue)?(sprintf("%d",$1)):(lastnorestarts))
unset table

finalthreshold=1e6
set table "/dev/null"
plot "gen-as-runtimes-final-sat.data" u 1:(finalthreshold=($2>=solvednorestarts&&valid(1)&&$1<finalthreshold)?(sprintf("%d",$1)):(finalthreshold))
unset table

set xlabel "Runtime (ms)"
set ylabel "Sat Instances Solved" offset 0.5
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [1400:2100]
set key bottom right Left width -2

set arrow 1 from lastnorestarts, solvednorestarts to finalthreshold, solvednorestarts front
set label 1 right at lastnorestarts, solvednorestarts "".sprintf("$%.1f{\\times}$", ((0.0+lastnorestarts) / finalthreshold)) offset character 1.0, character 0.5

plot \
    "runtimes.data" u (cumx(final)):(cumsaty(final)) smooth cumulative w l ti 'SBS' ls 1, \
    "runtimes.data" u (cumx(randomrestarts)):(cumsaty(randomrestarts)) smooth cumulative w l ti 'RSR' ls 2 dt (2,2), \
    "runtimes.data" u (cumx(softmax)):(cumsaty(softmax)) smooth cumulative w l ti 'DFS Biased' ls 4 dt (6,2), \
    "runtimes.data" u (cumx(norestarts)):(cumsaty(norestarts)) smooth cumulative w l ti 'DFS Degree' ls 6 dt (18,2), \
    "runtimes.data" u (cumx(random)):(cumsaty(random)) smooth cumulative w l ti 'DFS Random' ls 7 dt (6,2,2,2)


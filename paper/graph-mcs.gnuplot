# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 10.0cm,5.6cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-mcs.tex"

load "inferno.pal"
load "common.gnuplot"

solveddfskdown=0
solvedsbskdown=0
set table "/dev/null"
plot "kdownruntimes.data" u (cumx(kdown)):(solveddfskdown=solveddfskdown+(isfail(kdown)?0:1)) smooth cumulative
plot "kdownruntimes.data" u (cumx(kdownbiasedrestarts)):(solvedsbskdown=solvedsbskdown+(isfail(kdownbiasedrestarts)?0:1)) smooth cumulative
unset table

set table "gen-as-kdownruntimes-dfskdown.data"
set format x '%.0f'
set format x '%.0f'
plot "kdownruntimes.data" u (cumx(kdown)):(isfail(kdown)?0:1) smooth cumulative
unset table

set table "gen-as-kdownruntimes-sbskdown.data"
set format x '%.0f'
set format x '%.0f'
plot "kdownruntimes.data" u (cumx(kdownbiasedrestarts)):(isfail(kdownbiasedrestarts)?0:1) smooth cumulative
unset table

sbskdownthreshold=1e6
set table "/dev/null"
plot "gen-as-kdownruntimes-sbskdown.data" u 1:(sbskdownthreshold=($2>=solveddfskdown&&valid(1)&&$1<sbskdownthreshold)?(sprintf("%d",$1)):(sbskdownthreshold))
unset table

lastdfskdown=0
lastdfskdownvalue=0
set table "/dev/null"
plot "gen-as-kdownruntimes-dfskdown.data" u 1:(valid(2)?lastdfskdownvalue=$2:NaN)
plot "gen-as-kdownruntimes-dfskdown.data" u 1:(lastdfskdown=(valid(1)&&lastdfskdown==0&&$2==lastdfskdownvalue)?(sprintf("%d",$1)):(lastdfskdown))
unset table

solveddfsmcsplit=0
solvedsbsmcsplit=0
set table "/dev/null"
plot "mcsruntimes.data" u (cumx(mcsplitdown)):(solveddfsmcsplit=solveddfsmcsplit+(isfail(mcsplitdown)?0:1)) smooth cumulative
plot "mcsruntimes.data" u (cumx(mcsplitdownbiasedrestarts)):(solvedsbsmcsplit=solvedsbsmcsplit+(isfail(mcsplitdownbiasedrestarts)?0:1)) smooth cumulative
unset table

set table "gen-as-mcsruntimes-dfsmcsplit.data"
set format x '%.0f'
set format x '%.0f'
plot "mcsruntimes.data" u (cumx(mcsplitdown)):(isfail(mcsplitdown)?0:1) smooth cumulative
unset table

set table "gen-as-mcsruntimes-sbsmcsplit.data"
set format x '%.0f'
set format x '%.0f'
plot "mcsruntimes.data" u (cumx(mcsplitdownbiasedrestarts)):(isfail(mcsplitdownbiasedrestarts)?0:1) smooth cumulative
unset table

sbsmcsplitthreshold=1e6
set table "/dev/null"
plot "gen-as-mcsruntimes-sbsmcsplit.data" u 1:(sbsmcsplitthreshold=($2>=solveddfsmcsplit&&valid(1)&&$1<sbsmcsplitthreshold)?(sprintf("%d",$1)):(sbsmcsplitthreshold))
unset table

lastdfsmcsplit=0
lastdfsmcsplitvalue=0
set table "/dev/null"
plot "gen-as-mcsruntimes-dfsmcsplit.data" u 1:(valid(2)?lastdfsmcsplitvalue=$2:NaN)
plot "gen-as-mcsruntimes-dfsmcsplit.data" u 1:(lastdfsmcsplit=(valid(1)&&lastdfsmcsplit==0&&$2==lastdfsmcsplitvalue)?(sprintf("%d",$1)):(lastdfsmcsplit))
unset table

set xlabel "Runtime (ms)"
set ylabel "Instances Solved"
set xrange [1e2:1e6]
set logscale x
set format x '$10^{%T}$'
set yrange [0:]
set key bottom right invert Left width -4

set arrow 1 from lastdfskdown, solveddfskdown to sbskdownthreshold, solveddfskdown front
set label 1 left at lastdfskdown, solveddfskdown "".sprintf("$%.1f{\\times}$", ((0.0+lastdfskdown) / sbskdownthreshold)) offset character 0, character -0.4

set arrow 2 from lastdfsmcsplit, solveddfsmcsplit to sbsmcsplitthreshold, solveddfsmcsplit front
set label 2 left at lastdfsmcsplit, solveddfsmcsplit "".sprintf("$%.1f{\\times}$", ((0.0+lastdfsmcsplit) / sbsmcsplitthreshold)) offset character 0, character 0.2

plot \
    "kdownruntimes.data" u (cumx(kdown)):(cumy(kdown)) smooth cumulative w l ti '~~~~DFS' ls 7 dt (18,2), \
    "kdownruntimes.data" u (cumx(kdownbiasedrestarts)):(cumy(kdownbiasedrestarts)) smooth cumulative w l ti '~~~~SBS' ls 6 dt (6,2), \
    "kdownruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'k${\downarrow}$:', \
    "mcsruntimes.data" u (cumx(mcsplitdown)):(cumy(mcsplitdown)) smooth cumulative w l ti '~~~~DFS' ls 4 dt (2,2), \
    "mcsruntimes.data" u (cumx(mcsplitdownbiasedrestarts)):(cumy(mcsplitdownbiasedrestarts)) smooth cumulative w l ti '~~~~SBS' ls 2, \
    "mcsruntimes.data" u (NaN):(NaN) w p lc rgb 'white' ti 'McSplit${\downarrow}$:'


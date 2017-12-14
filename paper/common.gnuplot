# vim: set et ft=gnuplot sw=4 :

load "parula.pal"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

set border 3
set grid x y
set xtics nomirror
set ytics nomirror

timeout=1e6
isfail(x)=(stringcolumn(x) eq "NaN" || column(x) >= timeout)
cumx(x)=(isfail(x) ? 1e6 : column(x))
cumy(x)=(isfail(x) ? 1e-10 : 1)
cumsaty(x)=(stringcolumn("sat") eq "1" ? cumy(x) : 1e-10)

norestarts="sequential9"
softmax="sequentialinputordersoftmax13"
random="sequentialshuffle9"
randomrestarts="sequentialrestartsshuffle9"
anti="sequentialantiheuristic9"
final="sequentialinputordersoftmaxrestarts13"
dds="sequentialdds9"

mcsplit="mcsplit"
mcsplitbiasedrestarts="mcsplitbiasedrestarts"
kdown="kdown"
kdownbiasedrestarts="kdownbiasedrestarts"

glasgow2="glasgow2"
glasgow3="glasgow3"
pathlad="pathlad"
vf2="vf2"


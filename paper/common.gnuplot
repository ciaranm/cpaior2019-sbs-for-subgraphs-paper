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

norestarts="sequential13"
softmax="sequentialinputordersoftmax13"
random="sequentialshuffle13"
randomrestarts="sequentialrestartsshuffle13"
anti="sequentialantiheuristic13"
final="sequentialinputordersoftmaxrestarts13"
constant="sequentialinputordersoftmaxrestartsconstant13"
dds="sequentialdds13"
lubypar="parallelinputordersoftmaxrestarts13"
constantpar="parallelinputordersoftmaxrestartsconstant13"

mcsplitdown="mcsplitdown5"
mcsplitdownbiasedrestarts="mcsplitdownbiasedrestarts5"
kdown="sequentialixinduced3"
kdownbiasedrestarts="sequentialixinducedrestarts7"

glasgow2="glasgow2"
glasgow3="glasgow3"
pathlad="pathlad"
vf2="vf2"


# vim: set et ft=gnuplot sw=4 :

set style line 102 lc rgb '#333333' lt 1 lw 1
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
cumx(x)=(isfail(x) ? 1e6 : ((x eq ri || x eq riinduced) ? column(x) * 1000 : column(x)))
cumy(x)=(isfail(x) ? 1e-10 : 1)
cumsaty(x)=(stringcolumn("sat") eq "1" ? cumy(x) : 1e-10)
cumunsaty(x)=(stringcolumn("sat") eq "0" ? cumy(x) : 1e-10)

norestarts="sequential13"
softmax="sequentialinputordersoftmax13"
random="sequentialshuffle13"
randomrestarts="sequentialrestartsshuffle13"
randomrestartsgoods="sequentialrestartsshufflegoods13"
anti="sequentialantiheuristic13"
final="sequentialinputordersoftmaxrestarts13"
dds="sequentialdds13"

mcsplitdown="mcsplitdown14"
mcsplitdownbiasedrestarts="mcsplitdownbiasedrestarts14"
kdown="sequentialixinduced14"
kdownbiasedrestarts="sequentialixinducedrestarts14"

glasgow2="glasgow2"
glasgow3="glasgow3"
pathlad="pathlad"
vf2="vf2"
ri="ri"

norestartsinduced="sequentialinduced13"
finalinduced="sequentialinputordersoftmaxrestartsinduced13"

pathladinduced="pathladinduced"
vf2induced="vf2induced"
vf3induced="vf3induced"
riinduced="riinduced"


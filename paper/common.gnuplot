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
issat(x)=stringcolumn("sat") eq "1" ? 1 : 0
isunsat(x)=stringcolumn("sat") eq "0" ? 1 : 0
isfail(x)=(stringcolumn(x) eq "NaN" || column(x) >= timeout)
cumx(x)=(isfail(x) ? 1e6 : ((x eq ri || x eq riinduced) ? column(x) * 1000 : column(x)))
cumsatx(x)=(issat(x) ? cumx(x) : 0)
cumunsatx(x)=(isunsat(x) ? cumx(x) : 0)
cumy(x)=(isfail(x) ? 1e-10 : 1)
cumsaty(x)=(issat(x) ? cumy(x) : 1e-10)
cumunsaty(x)=(issat(x) ? cumy(x) : 1e-10)

norestarts="glasgowdegreenorestartsnonogoods"
softmax="glasgowbiasednorestartsnonogoods"
random="glasgowrandomnorestartsnonogoods"
randomrestarts="glasgowrandom"
biasedrestartsgoods="glasgowbiasednonogoods"
anti="glasgowantinorestartsnonogoods"
final="glasgowbiased"
dds="glasgowdegreenorestartsnonogoodsdds"

par="glasgowbiasedthreads36v2"
par2="glasgowbiasedthreads36v2"
parconst="glasgowbiasedconstant10000threads36v2"
parconsttick="glasgowbiasedconstant10000triggeredthreads36v2"
parconsttick2="glasgowbiasedconstant10000triggeredthreads36v2"

mcsplitdown="mcsplitdown14"
mcsplitdownbiasedrestarts="mcsplitdownbiasedrestarts14"
kdown="sequentialixinduced3"
kdownbiasedrestarts="sequentialixinducedrestarts7"

glasgow2="glasgow2"
glasgow3="glasgow3"
pathlad="pathlad"
vf2="vf2"
ri="ri"

norestartsinduced="glasgowdegreenorestartsnonogoodsinduced"
finalinduced="glasgowbiasedinduced"

pathladinduced="pathladinduced"
vf2induced="vf2induced"
vf3induced="vf3induced"
riinduced="riinduced"


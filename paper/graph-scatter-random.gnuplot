# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-random.tex"

load "common.gnuplot"

set xlabel "Degree Search Time (ms)"
set ylabel "Random + Restarts Search Time (ms)"
set xrange [1:1e6]
set yrange [1:1e6]
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plot \
    "searchtimes.data" u ($3==0||$2!=13?NaN:$4>=1e6?1e6:$4):($10>=1e6?1e6:$10) ls 4 pt 1 ps 0.7 ti 'Mesh sat', \
    "searchtimes.data" u ($3==0||($2!=2&&$2!=11)?NaN:$4>=1e6?1e6:$4):($10>=1e6?1e6:$10) w p ls 5 pt 2 ps 0.7 ti 'LV sat', \
    "searchtimes.data" u ($3==0||$2!=12?NaN:$4>=1e6?1e6:$4):($10>=1e6?1e6:$10) ls 6 pt 3 ps 0.7 ti 'Phase sat', \
    "searchtimes.data" u ($3==0||$2!=7?NaN:$4>=1e6?1e6:$4):($10>=1e6?1e6:$10) ls 7 pt 4 ps 0.7 ti 'Rand sat', \
    "searchtimes.data" u ($3==0||$2==13||$2==2||$2==11||$2==12||$2==7||$2==15?NaN:$4>=1e6?1e6:$4):($10>=1e6?1e6:$10) ls 8 pt 8 ps 0.7 ti 'Other sat', \
    "searchtimes.data" u ($3==1?NaN:$4>=1e6?1e6:$4):($10>=1e6?1e6:$10) ls 1 pt 6 ps 0.2 ti 'Any unsat', \
    x w l ls 0 notitle


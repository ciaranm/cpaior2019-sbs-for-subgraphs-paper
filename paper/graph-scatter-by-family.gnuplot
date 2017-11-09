# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 9cm,6.5cm font '\scriptsize' preamble '\usepackage{times,microtype}'
set output "gen-graph-scatter-by-family.tex"

load "common.gnuplot"

set xlabel "Degree Search Time (ms)"
set ylabel "Degree-Biased + Restarts Search Time (ms)"
set xrange [1:1e6]
set yrange [1:1e6]
set logscale x
set logscale y
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set key horiz rmargin maxcols 1 width -2 samplen 1
set size square

plot \
    "searchtimes.data" u ($3==0||$2==13||$2==2||$2==11||$2==12||$2==7||$2==15?NaN:$4>=1e6?1e6:$4):($9>=1e6?1e6:$9) ls 1 pt 7 ps 0.3 notitle, \
    "searchtimes.data" u ($3==0||$2!=13?NaN:$4>=1e6?1e6:$4):($9>=1e6?1e6:$9) ls 3 pt 1 ps 0.5 ti 'Mesh', \
    "searchtimes.data" u ($3==0||($2!=2&&$2!=11)?NaN:$4>=1e6?1e6:$4):($9>=1e6?1e6:$9) w p ls 5 pt 2 ps 0.5 ti 'LV', \
    "searchtimes.data" u ($3==0||$2!=12?NaN:$4>=1e6?1e6:$4):($9>=1e6?1e6:$9) ls 6 pt 3 ps 0.5 ti 'Phase', \
    "searchtimes.data" u ($3==0||$2!=7?NaN:$4>=1e6?1e6:$4):($9>=1e6?1e6:$9) ls 8 pt 6 ps 0.5 ti 'Rand', \
    "searchtimes.data" u (NaN):(NaN) ls 1 pt 7 ps 0.3 ti "Other", \
    x w l ls 0 notitle


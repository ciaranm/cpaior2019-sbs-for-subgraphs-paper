# vim: set et ft=gnuplot sw=4 :

set xtics add ('~~~~t/o' 2e6)
set ytics left offset character -3
set ytics add ('\,t/o' 2e6)

set xrange [1:2e6]
set yrange [1:2e6]

plot \
    plotfile u (column(xcol)==0&&column(ycol)==0?NaN:column(satcol)==0||column(famcol)!=13?NaN:column(xcol)>=1e6?2e6:column(xcol)==0?1:column(xcol)):(column(ycol)>=1e6?2e6:column(ycol)==0?1:column(ycol)) ls 4 pt 1 ps 0.7 ti 'Mesh sat', \
    plotfile u (column(xcol)==0&&column(ycol)==0?NaN:column(satcol)==0||(column(famcol)!=2&&column(famcol)!=11)?NaN:column(xcol)>=1e6?2e6:column(xcol)==0?1:column(xcol)):(column(ycol)>=1e6?2e6:column(ycol)==0?1:column(ycol)) w p ls 5 pt 2 ps 0.7 ti 'LV sat', \
    plotfile u (column(xcol)==0&&column(ycol)==0?NaN:column(satcol)==0||column(famcol)!=12?NaN:column(xcol)>=1e6?2e6:column(xcol)==0?1:column(xcol)):(column(ycol)>=1e6?2e6:column(ycol)==0?1:column(ycol)) ls 6 pt 3 ps 0.7 ti 'Phase sat', \
    plotfile u (column(xcol)==0&&column(ycol)==0?NaN:column(satcol)==0||column(famcol)!=7?NaN:column(xcol)>=1e6?2e6:column(xcol)==0?1:column(xcol)):(column(ycol)>=1e6?2e6:column(ycol)==0?1:column(ycol)) ls 7 pt 4 ps 0.7 ti 'Rand sat', \
    plotfile u (column(xcol)==0&&column(ycol)==0?NaN:column(satcol)==0||column(famcol)==13||column(famcol)==2||column(famcol)==11||column(famcol)==12||column(famcol)==7||column(famcol)==15?NaN:column(xcol)>=1e6?2e6:column(xcol)==0?1:column(xcol)):(column(ycol)>=1e6?2e6:column(ycol)==0?1:column(ycol)) ls 8 pt 8 ps 0.7 ti 'Other sat', \
    plotfile u (column(xcol)==0&&column(ycol)==0?NaN:column(satcol)==1?NaN:column(xcol)>=1e6?2e6:column(xcol)==0?1:column(xcol)):(column(ycol)>=1e6?2e6:column(ycol)==0?1:column(ycol)) ls 1 pt 6 ps 0.2 ti 'Any unsat', \
    x w l ls 0 notitle


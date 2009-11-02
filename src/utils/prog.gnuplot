# GNUplot script template
# George Foster

# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2009, Sa Majeste la Reine du Chef du Canada /
# Copyright 2009, Her Majesty in Right of Canada

# uncomment "term X11" and "pause 20000" to preview...
set term X11
# set term postscript eps color

set title "Main title for graph"
# set key 0.6, 69.5
# set key bottom right
set data style linespoints
set xlabel "label on xaxis"
set ylabel "label on yaxis"
# set xrange [:10000]
# set yrange [80:100]
# set xtics 0,100
plot \
"data-file" using 1:2 title "col2 versus col1",\
"data-file" using 1:3 title "col3 versus col1",\
"other-data-file" title "other data, col2 vs col2"

pause 20000

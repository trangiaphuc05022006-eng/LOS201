set terminal pngcairo size 1200,600 enhanced font 'Sans,11'
set output '/home/phuc/embedded_lab3/logs/latency_histogram.png'
set title 'Cyclictest Latency Histogram – Standard Kernel (Under Stress Load)'
set xlabel 'Latency (microseconds)'
set ylabel 'Number of occurrences (Log scale)'
set xrange [0:200]
set logscale y
set yrange [0.8:20000]
set grid
set key top right
set style fill solid 0.5
plot '/home/phuc/embedded_lab3/logs/hist_standard.txt' \
     using 1:2 with boxes title 'Thread 0 (Max: 141 us)' lc rgb '#2E75B6', \
     '' using 1:3 with boxes title 'Thread 1 (Max: 256 us)' lc rgb '#C55A11', \
     '' using 1:4 with boxes title 'Thread 2 (Max: 253 us)' lc rgb '#70AD47', \
     '' using 1:5 with boxes title 'Thread 3 (Max: 132 us)' lc rgb '#FFC000'

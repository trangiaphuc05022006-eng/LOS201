set terminal png size 1200,600
set output '/home/tran-dung/embedded_lab3/logs/latency_histogram.png'
set title 'Cyclictest Latency Histogram - Standard Kernel'
set xlabel 'Latency (microseconds)'
set ylabel 'Number of occurrences (log scale)'
set xrange [0:100]
set logscale y
set grid
set key top right
plot '/home/tran-dung/embedded_lab3/logs/hist_standard.txt' using 1:2 with linespoints title 'Thread 0', '' using 1:3 with linespoints title 'Thread 1', '' using 1:4 with linespoints title 'Thread 2', '' using 1:5 with linespoints title 'Thread 3'

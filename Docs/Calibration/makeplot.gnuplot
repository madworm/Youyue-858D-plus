set xlabel "T measured [°C]" font "Bitstream Vera Sans, 12"
set ylabel "ADC value" font "Bitstream Vera Sans, 12"

set title "Youyue 858D+ V1.45 temperature calibration - 2016-03-22" font "Bitstream Vera Sans, 12"

set xrange [0:500]
set yrange [0:500]

y1(x) = 0.90*x - 35 + 0.00026*x*x

plot "calibration.dat" using 3:2 title "Data" w lp, y1(x) title "Crude fit y(x) = 0.90*x + 0.00026*x^2 - 35"

pause -1 


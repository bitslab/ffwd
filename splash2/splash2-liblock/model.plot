
reset;
set ytics auto;
set xtics auto;
#set terminal aqua dashed enhanced font "Helvetica,18";
set terminal pdf dashed enhanced font "Helvetica,8";
set key default;
set key above;
set key center;
set datafile separator ",";

#set size ratio 0.5
set lmargin 8
set bmargin 2.8
set rmargin 1.2
set tmargin 0.7

set xrange[1:47];
#set yrange[1:20];

set xlabel "Number of cores";
set ylabel "Speedup";
set y2label "";

#set size 1.0, 0.8

plot "results/acceleration-radiosity-posix.csv"      using 1:2:3 axes x1y1 title "posix"      with yerrorlines lc rgb "#009400" lt 4 pt 13,\
		 "results/acceleration-radiosity-spinlock.csv"   using 1:2:3 axes x1y1 title "spinlock"   with yerrorlines lc rgb "#000094" lt 3 pt 1,\
		 "results/acceleration-radiosity-mcs.csv"        using 1:2:3 axes x1y1 title "mcs"        with yerrorlines lc rgb "#944a00" lt 2 pt 9,\
		 "results/acceleration-radiosity-flat.csv"       using 1:2:3 axes x1y1 title "flat"       with yerrorlines lc rgb "#009494" lt 5 pt 5,\
		 "results/acceleration-radiosity-rcl.csv"        using 1:2:3 axes x1y1 title "rcl"        with yerrorlines lc rgb "#940000" lw 2 lt 1 pt 7

#		 "results/acceleration-radiosity-posix.csv"      using 1:2:3 axes x1y1 title "patched poxix" with yerrorlines lc rgb "#000094" lt 3 pt 6


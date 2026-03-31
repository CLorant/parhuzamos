set encoding utf8
set datafile separator ","

name = ARG1
input_file = ARG2
output_file = ARG3
col_seq = int(ARG4)
col_speedup = int(ARG5)

col_n = 1
col_p = 2
col_par = col_seq + 1
col_efficiency = col_speedup + 1

stats input_file using col_n every ::1 nooutput
total_records = STATS_records
unique_n = ""
unique_labels = ""
do for [i=0:total_records-1] {
    stats input_file using (column(col_n)) every ::(i+1)::(i+1) nooutput
    n_val = STATS_min
    if (strstrt(unique_n, sprintf("%.0f", n_val)) == 0) {
        unique_n = sprintf("%s %.0f", unique_n, n_val)
        unique_labels = sprintf("%s %.0f", unique_labels, n_val)
    }
}
n_values = unique_n
n_labels = unique_labels

set terminal pngcairo enhanced font "Arial,12" size 1280,720
set output output_file
set grid lt 1 lc rgb "#dddddd"
set key top right noenhanced font ",12"
set tmargin 5
set bmargin 5
set style data linespoints

set multiplot layout 1,3 title name." - Számítási egységek számának hatása" font ",18" offset 0,-1.5

set origin 0.0, 0.0
set size 0.33, 0.95
set title "Futási Idő vs Szálak Száma" font ",14" offset 0, -0.75
set xlabel "Szálak száma (p)"
set ylabel "Futási idő (mp)"
set pointsize 1.5
set logscale y 2

plot_cmd_time = ""
do for [n_idx=1:words(n_values)] {
    plot_cmd_time = plot_cmd_time . \
      sprintf("'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) with linespoints title 'Seq (n=%s)', ", \
              input_file, col_n, real(word(n_values, n_idx)), col_p, col_n, real(word(n_values, n_idx)), col_seq, word(n_labels, n_idx))
    plot_cmd_time = plot_cmd_time . \
      sprintf("'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) with linespoints title 'Par (n=%s)', ", \
              input_file, col_n, real(word(n_values, n_idx)), col_p, col_n, real(word(n_values, n_idx)), col_par, word(n_labels, n_idx))
}
plot @plot_cmd_time

set origin 0.33, 0.0
set size 0.33, 0.95
set title "Gyorsítás vs Szálak Száma" font ",14" offset 0, -0.75
set xlabel "Szálak száma (p)"
set ylabel "Gyorsítás"
set pointsize 1.5
set logscale y 2

plot_cmd_speedup = ""
do for [n_idx=1:words(n_values)] {
    plot_cmd_speedup = plot_cmd_speedup . \
      sprintf("'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) with linespoints title 'n=%s', ", \
              input_file, col_n, real(word(n_values, n_idx)), col_p, col_n, real(word(n_values, n_idx)), col_speedup, word(n_labels, n_idx))
}
plot @plot_cmd_speedup

set origin 0.66, 0.0
set size 0.33, 0.95
set title "Hatékonyság vs Szálak Száma" font ",14" offset 0, -0.75
set xlabel "Szálak száma (p)"
set ylabel "Hatékonyság"
set pointsize 1.5
set logscale y 2

plot_cmd_eff = ""
do for [n_idx=1:words(n_values)] {
    plot_cmd_eff = plot_cmd_eff . \
      sprintf("'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) with linespoints title 'n=%s', ", \
              input_file, col_n, real(word(n_values, n_idx)), col_p, col_n, real(word(n_values, n_idx)), col_efficiency, word(n_labels, n_idx))
}
plot @plot_cmd_eff

unset multiplot

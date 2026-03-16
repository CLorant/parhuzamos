set encoding utf8
set datafile separator ","

name         = ARG1
input_file   = ARG2
output_file  = ARG3
col_seq      = int(ARG4)
col_speedup  = int(ARG5)

col_par = col_seq + 1
col_n = 1
col_p = 2

stats input_file using col_p every ::1 nooutput
total_records = STATS_records
unique_p = ""
do for [i=0:total_records-1] {
    stats input_file using (column(col_p)) every ::(i+1)::(i+1) nooutput
    p_val = STATS_min
    if (strstrt(unique_p, sprintf("%.0f", p_val)) == 0) {
         unique_p = unique_p . " " . sprintf("%.0f", p_val)
    }
}
p_values = unique_p

set terminal pngcairo size 960,720
set output output_file
set grid
set key bottom right noenhanced
set tmargin 5

set multiplot layout 1,2 title name." - Problémaméret hatása" font ",18" offset 0,-1.5

set origin 0.0, 0.0
set size 0.5, 0.95
set title "Futási Idő vs Problémaméret" font ",14"
set xlabel "Problémaméret (n)"
set ylabel "Futási idő (mp)"
set logscale y 2

set style line 1 lt 1 lw 2 pt 7  ps 1.5 lc rgb "#1f77b4"
set style line 2 lt 1 lw 2 pt 5  ps 1.5 lc rgb "#ff7f0e"
set style line 3 lt 1 lw 2 pt 9  ps 1.5 lc rgb "#2ca02c"
set style line 4 lt 1 lw 2 pt 11 ps 1.5 lc rgb "#d62728"
set style line 5 lt 1 lw 2 pt 13 ps 1.5 lc rgb "#B200FF"

i = 1
plot input_file every ::1 using (column(col_n)):(column(col_p)==1 ? column(col_seq) : 1/0) \
     with linespoints ls i title "Seq (p=1)", \
     for [p in p_values] input_file every ::1 using (column(col_n)):(column(col_p)==p ? column(col_par) : 1/0) \
     with linespoints ls (i=i+1) title sprintf("Par (p=%s)", p)

set origin 0.5, 0.0
set size 0.5, 0.95
set title "Gyorsítás vs Problémaméret" font ",14"
set xlabel "Problémaméret (n)"
set ylabel "Gyorsítás"
set logscale y 2

i = 0
plot for [p in p_values] input_file every ::1 using (column(col_n)):(column(col_p)==p ? column(col_speedup) : 1/0) \
     with linespoints ls (i=i+1) title sprintf("p=%s", p)
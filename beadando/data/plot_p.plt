set encoding utf8
set datafile separator ","

# ---- Arguments ----
# ARG1  display name   e.g. "Kódolás"
# ARG2  input CSV
# ARG3  output PNG
# ARG4  column index of the OMP time    (e.g. 3 for encode, 5 for decode)
# ARG5  column index of the OMP speedup (e.g. 7 for encode, 10 for decode)

name        = ARG1
input_file  = ARG2
output_file = ARG3
col_omp     = int(ARG4)
col_S_omp   = int(ARG5)

col_ocl     = col_omp   + 1
col_E_omp   = col_S_omp + 1   # OMP efficiency is one column after OMP speedup
col_S_ocl   = col_S_omp + 2

col_n = 1
col_p = 2

# ---- Discover unique n values ----
stats input_file using col_n every ::1 nooutput
total_records = STATS_records
unique_n      = ""
unique_labels = ""
do for [i=0:total_records-1] {
    stats input_file using (column(col_n)) every ::(i+1)::(i+1) nooutput
    n_val = STATS_min
    if (strstrt(unique_n, sprintf("%.0f", n_val)) == 0) {
        unique_n      = sprintf("%s %.0f", unique_n,      n_val)
        unique_labels = sprintf("%s %.0f", unique_labels, n_val)
    }
}
n_values = unique_n
n_labels  = unique_labels

# ---- Terminal ----
set terminal pngcairo enhanced font "Arial,12" size 1280,720
set output output_file
set grid lt 1 lc rgb "#dddddd"
set key top right noenhanced font ",11"
set tmargin 5
set bmargin 5
set style data linespoints

set style line 10 lt 2 lw 2.5 pt 6 ps 1.5 lc rgb "#8c564b"   # OCL reference

set multiplot layout 1,3 \
    title name." – Számítási egységek számának hatása" font ",18" offset 0,-1.5

# ==============================================================
# Panel 1 – Runtime vs p
# OCL appears as a horizontal dashed line per n (it does not vary with p).
# ==============================================================
set origin 0.0, 0.0
set size 0.33, 0.95
set title "Futási Idő vs Szálak Száma" font ",14" offset 0,-0.75
set xlabel "Szálak száma (p)"
set ylabel "Futási idő (mp)"
set pointsize 1.5
set logscale y 2

plot_cmd = ""
do for [n_idx=1:words(n_values)] {
    n_val  = real(word(n_values, n_idx))
    n_lbl  = word(n_labels,  n_idx)

    # OMP line for this n
    plot_cmd = plot_cmd . sprintf( \
        "'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) \
         with linespoints title 'OpenMP (n=%s)', ", \
        input_file, col_n, n_val, col_p, col_n, n_val, col_omp, n_lbl)

    # OCL horizontal reference for this n  (read OCL time at p=1 row)
    plot_cmd = plot_cmd . sprintf( \
        "'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) \
         with linespoints ls 10 title 'OpenCL (n=%s)', ", \
        input_file, col_n, n_val, col_p, col_n, n_val, col_ocl, n_lbl)
}
plot @plot_cmd

# ==============================================================
# Panel 2 – Speedup vs p
# ==============================================================
set origin 0.33, 0.0
set size 0.33, 0.95
set title "Gyorsítás vs Szálak Száma" font ",14" offset 0,-0.75
set xlabel "Szálak száma (p)"
set ylabel "Gyorsítás (alap: OMP p=1)"
set pointsize 1.5
set logscale y 2

plot_cmd = ""
do for [n_idx=1:words(n_values)] {
    n_val = real(word(n_values, n_idx))
    n_lbl = word(n_labels,  n_idx)

    plot_cmd = plot_cmd . sprintf( \
        "'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) \
         with linespoints title 'OpenMP (n=%s)', ", \
        input_file, col_n, n_val, col_p, col_n, n_val, col_S_omp, n_lbl)

    # OCL speedup is constant across p rows; plot it flat
    plot_cmd = plot_cmd . sprintf( \
        "'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) \
         with linespoints ls 10 title 'OpenCL (n=%s)', ", \
        input_file, col_n, n_val, col_p, col_n, n_val, col_S_ocl, n_lbl)
}
plot @plot_cmd

# ==============================================================
# Panel 3 – Efficiency vs p  (OMP only; OCL efficiency is not per-thread)
# ==============================================================
set origin 0.66, 0.0
set size 0.33, 0.95
set title "Hatékonyság vs Szálak Száma" font ",14" offset 0,-0.75
set xlabel "Szálak száma (p)"
set ylabel "Hatékonyság"
set pointsize 1.5
unset logscale y   # efficiency is in [0,1]; log scale not useful here
set yrange [0:1.2]

plot_cmd = ""
do for [n_idx=1:words(n_values)] {
    n_val = real(word(n_values, n_idx))
    n_lbl = word(n_labels,  n_idx)

    plot_cmd = plot_cmd . sprintf( \
        "'%s' every ::1 using (column(%d)==%g ? column(%d) : 1/0):(column(%d)==%g ? column(%d) : 1/0) \
         with linespoints title 'n=%s', ", \
        input_file, col_n, n_val, col_p, col_n, n_val, col_E_omp, n_lbl)
}
plot @plot_cmd

unset multiplot

set encoding utf8
set datafile separator ","

# ---- Arguments (same calling convention as the original) ----
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

# Adjacent columns in the CSV layout:
#   col_omp + 1 = OCL time
#   col_S_omp + 2 = OCL speedup  (OMP-speedup, OMP-efficiency, OCL-speedup)
col_ocl   = col_omp   + 1
col_S_ocl = col_S_omp + 2

col_n = 1
col_p = 2

# ---- Discover unique p values from the data ----
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

# ---- Terminal ----
set terminal pngcairo enhanced font "Arial,12" size 1280,720
set output output_file
set grid lt 1 lc rgb "#dddddd"
set key top right noenhanced font ",11"
set tmargin 5
set bmargin 5
set style data linespoints

set style line 1  lt 1 lw 2 pt 7  ps 1.5 lc rgb "#1f77b4"
set style line 2  lt 1 lw 2 pt 5  ps 1.5 lc rgb "#ff7f0e"
set style line 3  lt 1 lw 2 pt 9  ps 1.5 lc rgb "#2ca02c"
set style line 4  lt 1 lw 2 pt 11 ps 1.5 lc rgb "#d62728"
set style line 5  lt 1 lw 2 pt 13 ps 1.5 lc rgb "#9467bd"
# OCL gets a distinct dashed style so it is always visually separate
set style line 10 lt 2 lw 2.5 pt 6 ps 1.5 lc rgb "#8c564b"

set multiplot layout 1,2 \
    title name." – Problémaméret hatása" font ",18" offset 0,-1.5

# ==============================================================
# Panel 1 – Runtime vs N
# ==============================================================
set origin 0.0, 0.0
set size 0.5, 0.95
set title "Futási Idő vs Problémaméret" font ",14"
set xlabel "Problémaméret (pixelek száma)"
set ylabel "Futási idő (mp)"
set logscale y 2

i = 0
plot \
    for [p in p_values] \
        input_file every ::1 \
        using (column(col_n)):(column(col_p)==p ? column(col_omp) : 1/0) \
        with linespoints ls (i=i+1) title sprintf("OpenMP p=%s", p), \
    input_file every ::1 \
        using (column(col_n)):(column(col_p)==1 ? column(col_ocl) : 1/0) \
        with linespoints ls 10 title "OpenCL (GPU)"

# ==============================================================
# Panel 2 – Speedup vs N  (baseline = OMP p=1)
# ==============================================================
set origin 0.5, 0.0
set size 0.5, 0.95
set title "Gyorsítás vs Problémaméret" font ",14"
set xlabel "Problémaméret (pixelek száma)"
set ylabel "Gyorsítás (alap: OMP p=1)"
set logscale y 2

i = 0
plot \
    for [p in p_values] \
        input_file every ::1 \
        using (column(col_n)):(column(col_p)==p ? column(col_S_omp) : 1/0) \
        with linespoints ls (i=i+1) title sprintf("OpenMP p=%s", p), \
    input_file every ::1 \
        using (column(col_n)):(column(col_p)==1 ? column(col_S_ocl) : 1/0) \
        with linespoints ls 10 title "OpenCL (GPU)"

unset multiplot

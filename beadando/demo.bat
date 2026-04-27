@echo off
setlocal DisableDelayedExpansion
chcp 65001 >nul
title Parhuzamos LSB Szteganografia Demo
color 0B

rem -----------------------------------------------------------------------
rem  Check for Gnuplot
rem -----------------------------------------------------------------------
where gnuplot >nul 2>&1
if %errorlevel% equ 0 (
    set GNUPLOT_OK=1
) else (
    set GNUPLOT_OK=0
    echo [!] Gnuplot not found. Benchmark will skip plots.
    echo.
)

echo ==============================================
echo  Parhuzamos LSB szteganografia DEMO
echo ==============================================
echo.
pause

rem -----------------------------------------------------------------------
echo ==============================================
echo [1] SZINTETIKUS HORDOZO GENEREALASA (gen parancs)
echo     A program letrehoz egy 256x256-os PPM kepet.
echo ==============================================
echo.
stego.exe gen 256 256 data/demo/synthetic.ppm
if %errorlevel% equ 0 (
    echo     Sikeresen generalt kep: data/demo/synthetic.ppm
)
echo.
pause

rem -----------------------------------------------------------------------
echo ==============================================
echo [2] BEAGYAZAS OpenMP-VEL (--omp, 4 szal)
echo     Hordozo: data/samples/inputs/input.ppm
echo     Uzenet:  data/samples/inputs/input.txt
echo ==============================================
echo.

stego.exe encode data/samples/inputs/input.ppm data/demo/stego_omp.ppm data/samples/inputs/input.txt --omp --threads 4

if %errorlevel% equ 0 (
    echo     Kimenet: data/demo/stego_omp.ppm
)
echo.
pause

rem -----------------------------------------------------------------------
echo ==============================================
echo [3] DEKODOLAS OpenMP-VEL
echo     Visszafejtes a stego_omp.ppm-bol.
echo ==============================================

stego.exe decode data/demo/stego_omp.ppm data/demo/recovered_omp.txt --omp --threads 4

if %errorlevel% equ 0 (
    echo     Eredeti uzenet:
    more < data/samples/inputs/input.txt
    echo.
    echo     Visszanyert uzenet:
    more < data/demo/recovered_omp.txt
    echo.
)
echo.
pause

rem -----------------------------------------------------------------------
echo ==============================================
echo [4] BEAGYAZAS OpenCL-LEL (--ocl)
echo     Ugyanaz a hordozo es uzenet, GPU-val.
echo ==============================================

stego.exe encode data/samples/inputs/input.ppm data/demo/stego_ocl.ppm data/samples/inputs/input.txt --ocl

if %errorlevel% equ 0 (
    echo     Kimenet: data/demo/stego_ocl.ppm
)
echo.
pause

rem ---------- Step 5: Decode OCL (if file exists) ----------
if not exist data/demo/stego_ocl.ppm goto skip_ocl_decode

echo ==============================================
echo [5] DEKODOLAS OpenCL-LEL
echo ==============================================

stego.exe decode data/demo/stego_ocl.ppm data/demo/recovered_ocl.txt --ocl
if %errorlevel% equ 0 (
    echo     Visszanyert uzenet:
    more < data/demo/recovered_ocl.txt
    echo.
)
echo.
pause
goto after_ocl_decode

:skip_ocl_decode
echo [5] OpenCL DEKODOLAS kihagyva (nincs stego_ocl.ppm).
echo.
pause

:after_ocl_decode

rem -----------------------------------------------------------------------
echo ==============================================
echo [6] PNG HORDOZO KEZELESE
echo ==============================================

if not exist data/samples/inputs/input.png goto skip_png

echo     Hordozo: data/samples/inputs/input.png

stego.exe encode data/samples/inputs/input.png data/demo/stego_photo.png data/samples/inputs/input.txt --omp --threads 4
stego.exe decode data/demo/stego_photo.png data/demo/recovered_photo.txt --omp

echo ==============================================
echo     Visszanyert uzenet:
more < data/demo/recovered_photo.txt
echo ==============================================
echo.
echo.
pause
goto after_png

:skip_png
echo     A data/samples/inputs/input.png nem talalhato, a PNG teszt kihagyva.
echo.
pause

:after_png

rem -----------------------------------------------------------------------
echo ==============================================
echo [7] HIBAKEZELES: TUL NAGY UZENET
echo     Letrehozunk egy 2 MB-os uzenetet, ami nem fer bele a hordozoba.
echo ==============================================

fsutil file createnew data/demo/huge.txt 2000000 >nul

stego.exe encode data/samples/inputs/input.ppm data/demo/fail.ppm data/demo/huge.txt --omp

echo     A programnak hibat kellett jeleznie.
echo.
pause

rem -----------------------------------------------------------------------
echo ==============================================
echo [8] BENCHMARK FUTTATASA (n=256, p=1,2, trials=2)
echo ==============================================

stego.exe bench n=256 p=1 2 t=2

echo.
if "%GNUPLOT_OK%"=="1" (
    echo     Abrak: data/plots/
    dir /b data\plots\*.png 2>nul
) else (
    echo     (Gnuplot hianya miatt abrak nem keszultek.)
)
echo.

endlocal
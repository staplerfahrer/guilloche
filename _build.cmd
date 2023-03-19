@rem https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
gcc -g -O9 -o guilloche.exe main.c drawing.c fileio.c patterns.c threading.c
@set buildlevel=%errorlevel%
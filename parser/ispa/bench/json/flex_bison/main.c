#include <stdio.h>
#include <stdlib.h>
#include <time.h>
extern int yyparse(void);
extern void yy_scan_bytes(const char*, int);
int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); rewind(f);
    char* b = malloc(n); fread(b, 1, n, f); fclose(f);
    struct timespec s, e; clock_gettime(CLOCK_MONOTONIC, &s);
    yy_scan_bytes(b, (int)n); int rc = yyparse();
    clock_gettime(CLOCK_MONOTONIC, &e);
    if (rc) return 1;
    printf("{\"total_ms\":%f}\n", (e.tv_sec - s.tv_sec) * 1e3 + (e.tv_nsec - s.tv_nsec) / 1e6);
}

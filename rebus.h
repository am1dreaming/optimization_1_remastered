// Так как мне лень вставать с кровати Даня проверишь на винде работает не
// А то написано буквально в слепую
// Удачи!!!

#ifndef REBUS_H
#define REBUS_H

#if defined(__MINGW32__)
#define __USE_MINGW_ANSI_STDIO 1
#endif
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 199309L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#if defined(_MSC_VER)
#include <intrin.h>
#endif

#define MAX_ADDENDS 7
#define MAX_WORDS (MAX_ADDENDS + 1)  /* слагаемые + результат */
#define MAX_LEN 16 /* 16 цифр помещаются в long long */

static char words[MAX_WORDS][MAX_LEN + 1], letters[10];
static int nwords, nletters, leading[26], digit[26];
static unsigned long long nodes;

static int solve_core(void);

static inline int lowest_bit(unsigned x)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(x);
#elif defined(_MSC_VER)
    unsigned long i;
    _BitScanForward(&i, x);
    return (int)i;
#else
    int i = 0;
    while (!(x >> i & 1u)) i++;
    return i;
#endif
}


static int parse(const char *s)
{
    int len = 0, eq = 0, need_word = 1, seen[26] = {0};
    nwords = nletters = 0;
    memset(leading, 0, sizeof leading);
    for (;; s++) {
        if (*s >= 'A' && *s <= 'Z') {
            if (!len) {
                if (!need_word || nwords >= MAX_WORDS) return -1;
                need_word = 0;
            }
            if (len >= MAX_LEN) return -1;
            words[nwords][len++] = *s;
            continue;
        }
        if (len) {
            words[nwords++][len] = 0;
            len = 0;
        }
        if (!*s) break;
        if (*s == '+') {
            if (need_word || eq) return -1;
            need_word = 1;
        } else if (*s == '=') {
            if (need_word || eq++ || nwords < 2) return -1;
            need_word = 1;
        } else if (!strchr(" \t\r\n", *s)) {
            return -1;
        }
    }
    if (!eq || need_word || nwords < 3) return -1;
    for (int w = 0; w < nwords; w++) {
        for (char *c = words[w]; *c; c++)
            if (!seen[*c - 'A']++) {
                if (nletters == 10) return -1;
                letters[nletters++] = *c;
            }
        if (words[w][1]) leading[words[w][0] - 'A'] = 1;
    }
    return 0;
}

char *solve(const char *puzzle)
{
    nodes = 0;
    if (parse(puzzle) || !solve_core()) return NULL;
    size_t cap = MAX_WORDS * (MAX_LEN + 3) + 1;
    char *out = malloc(cap), *p = out;
    if (!out) return NULL;
    for (int w = 0; w < nwords; w++) {
        if (w) p += snprintf(p, cap - (size_t)(p - out), "%s", w == nwords - 1 ? " = " : " + ");
        for (char *c = words[w]; *c; c++) *p++ = (char)('0' + digit[*c - 'A']);
    }
    *p = 0;
    return out;
}

static int verify(const char *q, const char *a)
{
    int map[26], rev[10], start = 1, after_eq = 0;
    long long cur = 0, sum = 0;
    memset(map, -1, sizeof map);
    memset(rev, -1, sizeof rev);
    if (strlen(q) != strlen(a)) return 0;
    for (size_t i = 0;; i++) {
        if (q[i] >= 'A' && q[i] <= 'Z') {
            int L = q[i] - 'A', D = a[i] - '0';
            if (D < 0 || D > 9 || (map[L] != -1 && map[L] != D) || (rev[D] != -1 && rev[D] != L)) return 0;
            map[L] = D; rev[D] = L;
            if (start && D == 0 && q[i + 1] >= 'A' && q[i + 1] <= 'Z') return 0;
            start = 0; cur = cur * 10 + D;
            continue;
        }
        if (q[i] != a[i]) return 0;
        if (!start) {
            if (after_eq) {
                if (cur != sum) return 0;
            } else {
                sum += cur;
            }
            cur = 0;
        }
        start = 1;
        if (q[i] == '=') after_eq = 1;
        if (!q[i]) return after_eq;
    }
}

static const char *tests[] = {
    "SEND + MORE = MONEY",
    "BE + BE = MOO",
    "BIG + CAT = LION",
    "TWO + TWO = FOUR",
    "CROSS + ROADS = DANGER",
    "BASE + BALL = GAMES",
    "DONALD + GERALD = ROBERT",
    "ELEVEN + NINE + FIVE + FIVE = THIRTY",
    "A + B + C + D + E + F + G = HI",
    "THIS + IS + TOO = HARD",
    "SATURN + URANUS + NEPTUNE + PLUTO = PLANETS",

    "HJJ + FJAH + IGC + CCG + HDCJ + IBJ + CDE = HBEIA",
    "BCJF + IBGG + EDCD + AAEF + DAEB + ADHH + JDDEJ = GGDCD",
    "FJGFAF + GHEBDG + DCGAC + DADBAC + DCBIJA + CGDEFD + BACFG = DGJGIFC",
    "GBGJCA + DJIJCAB + BFFFEF + BHEFFAA + EHEDCJF = AIDBIGFD",
};

/* часы в миллисекундах */
static double now_ms(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    LARGE_INTEGER c;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1e3 / (double)freq.QuadPart;
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec / 1e6;
#endif
}

/* один ребус: вывод решения или замер (повторы не меньше min_ms мс) */
static int run_one(const char *q, int bench, double min_ms)
{
    if (!bench) {
        char *ans = solve(q);
        printf("Вход: %s\nРешение: %s\n\n", q, ans ? ans : "нет решения или неверный ввод");
        free(ans);
        return 1;
    }
    char *ans = NULL;
    int reps = 0;
    double t0 = now_ms(), el;
    do {
        free(ans);
        ans = solve(q);
        reps++;
    } while ((el = now_ms() - t0) < min_ms && reps < 1000000);
    int ok = ans && verify(q, ans);
    printf("%s\t%.6f\t%llu\t%s\n", q, el / reps, nodes, ok ? "OK" : "FAIL");
    fflush(stdout);
    free(ans);
    return ok;
}

/*
 * prog - решить встроенные тесты
 * prog "AB + CD = EF" и иные
 * prog -b [ребусы] - бенч -> ребус \t мс на решение \t узлы \t OK/FAIL
 *   флаги ->   -t МС - минимальное время замера ребуса (по умолчанию 200, 0 один прогон)
 *              -f ФАЙЛ - ребусы из файла, по одному в строке
 */
int main(int argc, char **argv)
{
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif
    int bench = 0, failed = 0, i = 1;
    double min_ms = 200;
    const char *file = NULL;
    for (; i < argc && argv[i][0] == '-'; i++) {
        if (!strcmp(argv[i], "-b")) bench = 1;
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) min_ms = atof(argv[++i]);
        else if (!strcmp(argv[i], "-f") && i + 1 < argc) file = argv[++i];
        else {
            fprintf(stderr, "неизвестный ключ: %s\n", argv[i]);
            return 2;
        }
    }

    if (file) {
        FILE *f = fopen(file, "r");
        char line[512];
        if (!f) {
            perror(file);
            return 2;
        }
        while (fgets(line, sizeof line, f)) {
            line[strcspn(line, "\r\n")] = 0;
            if (*line) failed += !run_one(line, bench, min_ms);
        }
        fclose(f);
    } else if (i < argc) {
        for (; i < argc; i++) failed += !run_one(argv[i], bench, min_ms);
    } else {
        for (size_t k = 0; k < sizeof tests / sizeof *tests; k++)
            failed += !run_one(tests[k], bench, min_ms);
    }
    return failed ? 1 : 0;
}

#endif /* REBUS_H */

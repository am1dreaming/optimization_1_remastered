#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORDS 8
#define MAX_LEN 16

static char words[MAX_WORDS][MAX_LEN + 1], letters[10];
static int  nwords, nletters, leading[26], digit[26];
static unsigned long long nodes;

static int solve_core(void);

static int parse(const char *s)
{
    int len = 0, eq = 0, seen[26] = {0};
    nwords = nletters = 0;
    memset(leading, 0, sizeof leading);
    for (;; s++) {
        if (*s >= 'A' && *s <= 'Z') {
            if (len >= MAX_LEN || nwords >= MAX_WORDS) return -1;
            words[nwords][len++] = *s; continue;
        }
        if (len) { 
            words[nwords++][len] = 0; len = 0; 
        }
        if (!*s) break;
        if (*s == '=') { 
            if (eq++ || nwords < 2) return -1; 
        }
        else if (!strchr(" \t\n", *s) && (*s != '+' || eq)) return -1;
    }
    if (!eq || nwords < 3) return -1;
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
    char *out = malloc(MAX_WORDS * (MAX_LEN + 3) + 1), *p = out;
    if (!out) return NULL;
    for (int w = 0; w < nwords; w++) {
        if (w) p += sprintf(p, w == nwords - 1 ? " = " : " + ");
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
            } else sum += cur; cur = 0; 
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

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

int main(int argc, char **argv)
{
    int bench = argc > 1 && !strcmp(argv[1], "-b"), first = 1 + bench;
    int n = argc > first ? argc - first : (int)(sizeof tests / sizeof *tests);
    for (int i = 0; i < n; i++) {
        const char *q = argc > first ? argv[first + i] : tests[i];
        if (!bench) {
            char *ans = solve(q);
            printf("Вход: %s\nРешение: %s\n\n", q, ans ? ans : "нет решения");
            free(ans);
            continue;
        }

        char *ans = NULL;
        int reps = 0;
        double t0 = now_ms(), el;
        do { 
            free(ans); 
            ans = solve(q); 
            reps++; 
        } while ((el = now_ms() - t0) < 200 && reps < 1000000);
        printf("%s\t%.4f\t%llu\t%s\n", q, el / reps, nodes, ans && verify(q, ans) ? "OK" : "FAIL");
        free(ans);
    }
    return 0;
}

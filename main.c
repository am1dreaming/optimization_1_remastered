#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS 8 
#define MAX_LEN 16

static char words[MAX_WORDS][MAX_LEN + 1], letters[10];
static int nwords, nletters, digit[26], leading[26], used[10];

static int parse(const char *s)
{
    int len = 0, eq = 0, seen[26] = {0};
    nwords = nletters = 0;
    memset(leading, 0, sizeof leading); 
    memset(used, 0, sizeof used);
    for (;; s++) {
        if (*s >= 'A' && *s <= 'Z') {
            if (len >= MAX_LEN || nwords >= MAX_WORDS) return -1;
            words[nwords][len++] = *s; continue;
        }
        if (len) { 
            words[nwords++][len] = 0; 
            len = 0; 
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

static long long value(const char *w)
{
    long long v = 0;
    while (*w) v = v * 10 + digit[*w++ - 'A'];
    return v;
}

static int search(int k)
{
    if (k == nletters) {
        long long sum = 0;
        for (int w = 0; w < nwords - 1; w++) sum += value(words[w]);
        return sum == value(words[nwords - 1]);
    }
    int L = letters[k] - 'A';
    for (int d = leading[L]; d <= 9; d++) {
        if (used[d]) continue;
        used[d] = 1; digit[L] = d;
        if (search(k + 1)) return 1; used[d] = 0;
    }
    return 0;
}
char *solve(const char *puzzle)
{
    if (parse(puzzle) || !search(0)) return NULL;
    char *out = malloc(MAX_WORDS * (MAX_LEN + 3) + 1), *p = out;
    for (int w = 0; w < nwords; w++) {
        if (w) p += sprintf(p, w == nwords - 1 ? " = " : " + ");
        for (char *c = words[w]; *c; c++) *p++ = (char)('0' + digit[*c - 'A']);
    }
    *p = 0;
    return out;
}

int main(int argc, char **argv)
{
    static const char *tests[] = {
        "SEND + MORE = MONEY", 
        "BE + BE = MOO", "BIG + CAT = LION",
        "TWO + TWO = FOUR",
        "CROSS + ROADS = DANGER",
        "BASE + BALL = GAMES",
        "ELEVEN + NINE + FIVE + FIVE = THIRTY",
        "A + B + C + D + E + F + G = HI",
        "THIS + IS + TOO = HARD",
        "SATURN + URANUS + NEPTUNE + PLUTO = PLANETS",
    };
    int n = argc > 1 ? argc - 1 : (int)(sizeof tests / sizeof *tests);
    for (int i = 0; i < n; i++) {
        const char *q = argc > 1 ? argv[i + 1] : tests[i];
        char *ans = solve(q);
        printf("Вход: %s\nРешение: %s\n\n", q, ans ? ans : "нет решения");
        free(ans);
    }
    return 0;
}

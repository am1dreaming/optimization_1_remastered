#include "rebus.h"

static int used[10];
static long long coef[10];

static int search(int k, long long sum)
{
    nodes++;
    if (k == nletters) return sum == 0;
    int L = letters[k] - 'A';
    for (int d = leading[L]; d <= 9; d++) {
        if (used[d]) continue;
        used[d] = 1; digit[L] = d;
        if (search(k + 1, sum + coef[k] * d)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_core(void)
{
    long long w[26] = {0};
    for (int i = 0; i < nwords; i++) {
        long long p = i == nwords - 1 ? -1 : 1;
        for (int j = (int)strlen(words[i]) - 1; j >= 0; j--, p *= 10)
            w[words[i][j] - 'A'] += p;
    }
    for (int k = 0; k < nletters; k++) coef[k] = w[letters[k] - 'A'];
    memset(used, 0, sizeof used);
    return search(0, 0);
}

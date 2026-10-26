#include "rebus.h"

static int used[10];

static long long value(const char *w)
{
    long long v = 0;
    while (*w) v = v * 10 + digit[*w++ - 'A'];
    return v;
}

static int search(int k)
{
    nodes++;
    if (k == nletters) {
        long long sum = 0;
        for (int w = 0; w < nwords - 1; w++) sum += value(words[w]);
        return sum == value(words[nwords - 1]);
    }
    int L = letters[k] - 'A';
    for (int d = leading[L]; d <= 9; d++) {
        if (used[d]) continue;
        used[d] = 1; digit[L] = d;
        if (search(k + 1)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_core(void)
{
    memset(used, 0, sizeof used);
    return search(0);
}

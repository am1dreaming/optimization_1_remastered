#include "rebus.h"

static int used[10];
static int R;
static int order[10], nord;
static int col_end[MAX_LEN];
static int col_n[MAX_LEN], col_l[MAX_LEN][MAX_WORDS], col_res[MAX_LEN];

static int search(int k, int col, int carry)
{
    nodes++;
    while (col < R && col_end[col] == k) {
        int s = carry;
        for (int i = 0; i < col_n[col]; i++) s += digit[col_l[col][i]];
        if (s % 10 != digit[col_res[col]]) return 0;
        carry = s / 10; col++;
    }
    if (k == nord) return carry == 0;
    int L = order[k];
    for (int d = leading[L]; d <= 9; d++) {
        if (used[d]) continue;
        used[d] = 1; digit[L] = d;
        if (search(k + 1, col, carry)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_core(void)
{
    const char *res = words[nwords - 1];
    int seen[26] = {0};
    R = (int)strlen(res);
    for (int w = 0; w < nwords - 1; w++)
        if ((int)strlen(words[w]) > R) return 0;
    nord = 0;
    for (int c = 0; c < R; c++) {
        col_n[c] = 0;
        for (int w = 0; w < nwords - 1; w++) {
            int len = (int)strlen(words[w]);
            if (c >= len) continue;
            int L = words[w][len - 1 - c] - 'A';
            col_l[c][col_n[c]++] = L;
            if (!seen[L]++) order[nord++] = L;
        }
        int L = col_res[c] = res[R - 1 - c] - 'A';
        if (!seen[L]++) order[nord++] = L;
        col_end[c] = nord;
    }
    memset(used, 0, sizeof used);
    return search(0, 0, 0);
}

#include "rebus.h"

static int R;
static int order[10], nord, forced[10];
static int col_end[MAX_LEN];
static int col_n[MAX_LEN], col_l[MAX_LEN][MAX_WORDS], col_res[MAX_LEN];

static int col_sum(int col, int carry)
{
    for (int i = 0; i < col_n[col]; i++) carry += digit[col_l[col][i]];
    return carry;
}

static int search(int k, int col, int carry, unsigned mask)
{
    nodes++;
    while (col < R && col_end[col] == k) {
        int s = col_sum(col, carry);
        if (s % 10 != digit[col_res[col]]) return 0;
        carry = s / 10; col++;
    }
    if (k == nord) return carry == 0;
    int L = order[k];
    if (forced[k]) {
        int s = col_sum(col, carry), d = s % 10;
        if ((mask >> d & 1) || (d == 0 && leading[L])) return 0;
        digit[L] = d;
        return search(k + 1, col + 1, s / 10, mask | 1u << d);
    }
    unsigned free = ~mask & 0x3FF & (leading[L] ? ~1u : ~0u);
    while (free) {
        int d = __builtin_ctz(free);
        free &= free - 1;
        digit[L] = d;
        if (search(k + 1, col, carry, mask | 1u << d)) return 1;
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
            if (!seen[L]++) { 
                forced[nord] = 0; order[nord++] = L; 
            }
        }
        int L = col_res[c] = res[R - 1 - c] - 'A';
        if (!seen[L]++) { 
            forced[nord] = 1; order[nord++] = L; 
        }
        col_end[c] = nord;
    }
    return search(0, 0, 0, 0);
}

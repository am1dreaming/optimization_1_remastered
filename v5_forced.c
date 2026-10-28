#include "rebus.h"

static int used[10];
static int R;
static int order[10], nord, forced[10];
static int col_end[MAX_LEN];
static int col_n[MAX_LEN], col_l[MAX_LEN][MAX_WORDS], col_res[MAX_LEN];
static long long coef[10], pos_rest[11], neg_rest[11];

static int col_sum(int col, int carry)
{
    for (int i = 0; i < col_n[col]; i++) carry += digit[col_l[col][i]];
    return carry;
}

static int search(int k, int col, int carry, long long sum)
{
    nodes++;
    while (col < R && col_end[col] == k) {
        int s = col_sum(col, carry);
        if (s % 10 != digit[col_res[col]]) return 0;
        carry = s / 10; col++;
    }
    if (k == nord) return carry == 0;
    if (sum + pos_rest[k] < 0 || sum + neg_rest[k] > 0) return 0;
    int L = order[k];
    if (forced[k]) {
        /* буква результата впервые встречается в этом столбце: все слагаемые
         * столбца уже известны, значит её цифра однозначно = сумма % 10 */
        int s = col_sum(col, carry), d = s % 10;
        if (used[d] || (d == 0 && leading[L])) return 0;
        used[d] = 1; digit[L] = d;
        if (search(k + 1, col + 1, s / 10, sum + coef[k] * d)) return 1;
        used[d] = 0;
        return 0;
    }
    for (int d = leading[L]; d <= 9; d++) {
        if (used[d]) continue;
        used[d] = 1; digit[L] = d;
        if (search(k + 1, col, carry, sum + coef[k] * d)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_core(void)
{
    const char *res = words[nwords - 1];
    int seen[26] = {0};
    long long w[26] = {0};
    R = (int)strlen(res);
    for (int w_ = 0; w_ < nwords - 1; w_++)
        if ((int)strlen(words[w_]) > R) return 0;

    for (int i = 0; i < nwords; i++) {
        long long p = i == nwords - 1 ? -1 : 1;
        for (int j = (int)strlen(words[i]) - 1; j >= 0; j--, p *= 10)
            w[words[i][j] - 'A'] += p;
    }

    nord = 0;
    for (int c = 0; c < R; c++) {
        col_n[c] = 0;
        for (int i = 0; i < nwords - 1; i++) {
            int len = (int)strlen(words[i]);
            if (c >= len) continue;
            int L = words[i][len - 1 - c] - 'A';
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

    pos_rest[nord] = neg_rest[nord] = 0;
    for (int k = nord - 1; k >= 0; k--) {
        coef[k] = w[order[k]];
        pos_rest[k] = pos_rest[k + 1] + (coef[k] > 0 ? coef[k] * 9 : 0);
        neg_rest[k] = neg_rest[k + 1] + (coef[k] < 0 ? coef[k] * 9 : 0);
    }
    memset(used, 0, sizeof used);
    return search(0, 0, 0, 0);
}

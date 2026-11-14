#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORDS   8      /* до 7 слагаемых + результат */
#define MAX_LEN     32     /* макс. длина одного слова */
#define MAX_LETTERS 10     /* различных букв не больше 10 */

static char words[MAX_WORDS][MAX_LEN]; /* слагаемые */
static int  n_addends;                 /* число слагаемых */
static char result_word[MAX_LEN];      /* права€ часть */

static char letters[MAX_LETTERS];      /* список уникальных букв */
static int  n_letters;
static int  digit_of[MAX_LETTERS];     /* индекс буквы -> цифра */
static int  used_digit[10];            /* зан€тые цифры */
static int  is_leading[MAX_LETTERS]; /* Ќќ¬ќ≈: флаг, что буква не может быть 0 */

/* ---------- разбор входа ---------- */

static int letter_index(char c) {
    for (int i = 0; i < n_letters; i++)
        if (letters[i] == c) return i;
    return -1;
}

static void add_letter(char c) {
    if (letter_index(c) >= 0) return;
    if (n_letters >= MAX_LETTERS) {
        fprintf(stderr, "—лишком много различных букв\n");
        exit(1);
    }
    letters[n_letters++] = c;
}

static void safe_copy(char* dst, const char* src) {
    if (strlen(src) >= MAX_LEN) {
        fprintf(stderr, "—лово слишком длинное: %s\n", src);
        exit(1);
    }
    strcpy(dst, src);
}

static void parse(const char* input) {
    char buf[512];
    int j = 0;
    /* убираем все пробелы */
    for (int i = 0; input[i]; i++)
        if (!isspace((unsigned char)input[i])) buf[j++] = input[i];
    buf[j] = '\0';

    char* eq = strchr(buf, '=');
    if (!eq) { fprintf(stderr, "Ќет '=' во входе\n"); exit(1); }
    *eq = '\0';
    safe_copy(result_word, eq + 1);

    n_addends = 0;
    char* p = buf;
    while (*p) {
        char* plus = strchr(p, '+');
        if (plus) *plus = '\0';
        if (n_addends >= MAX_WORDS - 1) {
            fprintf(stderr, "—лишком много слагаемых\n"); exit(1);
        }
        safe_copy(words[n_addends++], p);
        if (!plus) break;
        p = plus + 1;
    }
}

/* ---------- вычисление и проверка ---------- */

static long eval_word(const char* w) {
    long v = 0;
    for (int i = 0; w[i]; i++)
        v = v * 10 + digit_of[letter_index(w[i])];
    return v;
}

/* ведущие нули запрещены (дл€ слов длины > 1) */
static int no_leading_zero(void) {
    for (int i = 0; i < n_addends; i++)
        if (strlen(words[i]) > 1 &&
            digit_of[letter_index(words[i][0])] == 0) return 0;
    if (strlen(result_word) > 1 &&
        digit_of[letter_index(result_word[0])] == 0) return 0;
    return 1;
}

static int check_solution(void) {
    if (!no_leading_zero()) return 0;
    long sum = 0;
    for (int i = 0; i < n_addends; i++) sum += eval_word(words[i]);
    return sum == eval_word(result_word);
}

/* ---------- перебор ---------- */

static int solve_rec(int pos) {
    if (pos == n_letters) return check_solution();
    for (int d = 0; d <= 9; d++) {
        if (used_digit[d]) continue;

        /* ќѕ“»ћ»«ј÷»я 1: ранний отсев ведущих нулей */
        if (d == 0 && is_leading[pos]) continue;

        used_digit[d] = 1;
        digit_of[pos] = d;
        if (solve_rec(pos + 1)) return 1;   /* нашли Ч выходим */
        used_digit[d] = 0;
    }
    return 0;
}

/* ---------- вывод ---------- */

static void print_solution(void) {
    for (int i = 0; i < n_addends; i++) {
        if (i) printf(" + ");
        printf("%ld", eval_word(words[i]));
    }
    printf(" = %ld\n", eval_word(result_word));
}

/* ---------- main ---------- */

int main(void) {
    char line[1024];
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;

        n_letters = 0;
        parse(line);

        /* пор€док добавлени€ букв: сначала слагаемые, потом результат */
        for (int i = 0; i < n_addends; i++)
            for (int k = 0; words[i][k]; k++) add_letter(words[i][k]);
        for (int k = 0; result_word[k]; k++) add_letter(result_word[k]);

        /* ќѕ“»ћ»«ј÷»я 1 определ€ем буквы, которые не могут быть нул€ми */
        memset(is_leading, 0, sizeof is_leading);
        for (int i = 0; i < n_addends; i++) {
            if (strlen(words[i]) > 1)
                is_leading[letter_index(words[i][0])] = 1;
        }
        if (strlen(result_word) > 1)
            is_leading[letter_index(result_word[0])] = 1;

        /* ќѕ“»ћ»«ј÷»я 2 (—ќ–“»–ќ¬ ј) */
        int freq[MAX_LETTERS] = { 0 };
        // —читаем частоту каждой буквы
        for (int i = 0; i < n_addends; i++) {
            for (int k = 0; words[i][k]; k++) {
                freq[letter_index(words[i][k])]++;
            }
        }
        for (int k = 0; result_word[k]; k++) {
            freq[letter_index(result_word[k])]++;
        }

        // —ортируем пузырьком (букв максимум 10, это мгновенно)
        for (int i = 0; i < n_letters - 1; i++) {
            for (int j = i + 1; j < n_letters; j++) {
                // ѕриоритет: 100 баллов если буква ведуща€ + еЄ частота
                int score_i = (is_leading[i] ? 100 : 0) + freq[i];
                int score_j = (is_leading[j] ? 100 : 0) + freq[j];

                if (score_j > score_i) {
                    // ћен€ем местами сами буквы
                    char tmp_c = letters[i];
                    letters[i] = letters[j];
                    letters[j] = tmp_c;

                    // ќ„≈Ќ№ ¬ј∆Ќќ: мен€ем местами их флаги ведущих и частоту!
                    int tmp_i = is_leading[i];
                    is_leading[i] = is_leading[j];
                    is_leading[j] = tmp_i;

                    int tmp_f = freq[i];
                    freq[i] = freq[j];
                    freq[j] = tmp_f;
                }
            }
        }

        memset(used_digit, 0, sizeof used_digit);

        if (solve_rec(0)) {
            print_solution();
        }
        else {
            printf("–ешение не найдено\n");
        }
    }
    return 0;
}
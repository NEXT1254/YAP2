#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_WORDS   8
#define MAX_LEN     32
#define MAX_LETTERS 10

static char words[MAX_WORDS][MAX_LEN];
static int  n_addends;
static char result_word[MAX_LEN];

static char letters[MAX_LETTERS];
static int  n_letters;
static int  digit_of[MAX_LETTERS];
static int  used_digit[10];
static int  is_leading[MAX_LETTERS];

static int max_len = 0;                 /* Для поразрядного перебора */
static long long steps_done = 0;        /* Счетчик шагов */

static int solve_col(int col, int carry); /* Предварительное объявление */

/* Разбор входа */

static int letter_index(char c) {
    for (int i = 0; i < n_letters; i++)
        if (letters[i] == c) return i;
    return -1;
}

static void add_letter(char c) {
    if (letter_index(c) >= 0) return;
    if (n_letters >= MAX_LETTERS) {
        fprintf(stderr, "Too many distinct letters\n");
        exit(1);
    }
    letters[n_letters++] = c;
}

static void safe_copy(char* dst, const char* src) {
    if (strlen(src) >= MAX_LEN) {
        fprintf(stderr, "Word too long: %s\n", src);
        exit(1);
    }
    strcpy(dst, src);
}

static void parse(const char* input) {
    char buf[512];
    int j = 0;
    for (int i = 0; input[i]; i++)
        if (!isspace((unsigned char)input[i])) buf[j++] = input[i];
    buf[j] = '\0';

    char* eq = strchr(buf, '=');
    if (!eq) { fprintf(stderr, "No '=' in input\n"); exit(1); }
    *eq = '\0';
    safe_copy(result_word, eq + 1);

    n_addends = 0;
    char* p = buf;
    while (*p) {
        char* plus = strchr(p, '+');
        if (plus) *plus = '\0';
        if (n_addends >= MAX_WORDS - 1) {
            fprintf(stderr, "Too many addends\n"); exit(1);
        }
        safe_copy(words[n_addends++], p);
        if (!plus) break;
        p = plus + 1;
    }
}

/* Вычисление и вывод */

static long eval_word(const char* w) {
    long v = 0;
    for (int i = 0; w[i]; i++)
        v = v * 10 + digit_of[letter_index(w[i])];
    return v;
}

static void print_solution(void) {
    for (int i = 0; i < n_addends; i++) {
        if (i) printf(" + ");
        printf("%ld", eval_word(words[i]));
    }
    printf(" = %ld\n", eval_word(result_word));
}

/* ОПТИМИЗАЦИЯ 3: Поразрядная арифметика */

// Вспомогательная функция: перебирает цифры для нераспределенных букв в текущем столбце
static int assign_col(int col, int carry, int* unassigned, int unassigned_cnt, int depth) {
    steps_done++; // Считаем каждый шаг рекурсии

    if (depth == unassigned_cnt) {
        // Все буквы в текущем столбце назначены, проверяем арифметику
        int sum = carry;
        for (int i = 0; i < n_addends; i++) {
            int len = strlen(words[i]);
            if (col < len) {
                char c = words[i][len - 1 - col];
                sum += digit_of[letter_index(c)];
            }
        }
        int expected_digit = sum % 10;
        int carry_out = sum / 10;

        // Проверяем, совпадает ли с правой частью уравнения
        if (col < (int)strlen(result_word)) {
            char rc = result_word[strlen(result_word) - 1 - col];
            int ridx = letter_index(rc);
            if (digit_of[ridx] != expected_digit) return 0; // Тупик!
        }
        else {
            if (expected_digit != 0) return 0; // В результате нет разряда, а сумма есть
        }

        // Переходим к следующему столбцу (влево)
        return solve_col(col + 1, carry_out);
    }

    int idx = unassigned[depth];
    for (int d = 0; d <= 9; d++) {
        if (used_digit[d]) continue;
        // Оптимизация 1 (ведущие нули) всё еще работает
        if (d == 0 && is_leading[idx]) continue;

        used_digit[d] = 1;
        digit_of[idx] = d;

        if (assign_col(col, carry, unassigned, unassigned_cnt, depth + 1)) return 1;

        used_digit[d] = 0;
        digit_of[idx] = -1; // Откат
    }
    return 0;
}

// Главная функция поразрядного перебора
static int solve_col(int col, int carry) {
    if (col == max_len) {
        return carry == 0; // Если после всех столбцов остался перенос - ошибка
    }

    int unassigned[MAX_LETTERS];
    int unassigned_cnt = 0;

    // Собираем нераспределенные буквы из слагаемых текущего столбца
    for (int i = 0; i < n_addends; i++) {
        int len = strlen(words[i]);
        if (col < len) {
            char c = words[i][len - 1 - col];
            int idx = letter_index(c);
            if (digit_of[idx] == -1) {
                int found = 0;
                for (int k = 0; k < unassigned_cnt; k++)
                    if (unassigned[k] == idx) found = 1;
                if (!found) unassigned[unassigned_cnt++] = idx;
            }
        }
    }

    // Собираем букву из результата текущего столбца
    if (col < (int)strlen(result_word)) {
        char c = result_word[strlen(result_word) - 1 - col];
        int ridx = letter_index(c);
        if (digit_of[ridx] == -1) {
            int found = 0;
            for (int k = 0; k < unassigned_cnt; k++)
                if (unassigned[k] == ridx) found = 1;
            if (!found) unassigned[unassigned_cnt++] = ridx;
        }
    }

    return assign_col(col, carry, unassigned, unassigned_cnt, 0);
}

/* main */

int main(void) {
    char line[1024];
    while (fgets(line, sizeof line, stdin)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;

        n_letters = 0;
        parse(line);

        for (int i = 0; i < n_addends; i++)
            for (int k = 0; words[i][k]; k++) add_letter(words[i][k]);
        for (int k = 0; result_word[k]; k++) add_letter(result_word[k]);

        /* ОПТИМИЗАЦИЯ 1: Определение ведущих букв */
        memset(is_leading, 0, sizeof is_leading);
        for (int i = 0; i < n_addends; i++) {
            if (strlen(words[i]) > 1)
                is_leading[letter_index(words[i][0])] = 1;
        }
        if (strlen(result_word) > 1)
            is_leading[letter_index(result_word[0])] = 1;

        /* ОПТИМИЗАЦИЯ 2: Сортировка букв */
        int freq[MAX_LETTERS] = { 0 };
        for (int i = 0; i < n_addends; i++) {
            for (int k = 0; words[i][k]; k++) {
                freq[letter_index(words[i][k])]++;
            }
        }
        for (int k = 0; result_word[k]; k++) {
            freq[letter_index(result_word[k])]++;
        }

        for (int i = 0; i < n_letters - 1; i++) {
            for (int j = i + 1; j < n_letters; j++) {
                int score_i = (is_leading[i] ? 100 : 0) + freq[i];
                int score_j = (is_leading[j] ? 100 : 0) + freq[j];

                if (score_j > score_i) {
                    char tmp_c = letters[i]; letters[i] = letters[j]; letters[j] = tmp_c;
                    int tmp_i = is_leading[i]; is_leading[i] = is_leading[j]; is_leading[j] = tmp_i;
                    int tmp_f = freq[i]; freq[i] = freq[j]; freq[j] = tmp_f;
                }
            }
        }

        /* НАСТРОЙКА ДЛЯ ОПТИМИЗАЦИИ 3 */
        max_len = strlen(result_word);
        for (int i = 0; i < n_addends; i++) {
            int len = strlen(words[i]);
            if (len > max_len) max_len = len;
        }

        memset(used_digit, 0, sizeof used_digit);
        for (int i = 0; i < n_letters; i++) digit_of[i] = -1; // Инициализируем -1

        steps_done = 0; // Сбрасываем счетчик

        // Запускаем поразрядный перебор
        if (solve_col(0, 0)) {
            print_solution();
            printf("Steps done: %lld\n", steps_done); // Вывод шагов
        }
        else {
            printf("Solution not found\n");
        }
    }
    return 0;
}
#include <stdio.h>
#include "calculator.h"


int main(void) {
    char oper, cont = 'y';
    double num1, num2, ans = 0.0;
    int error = 0;

    do {
        printf("Введите первый операнд, операцию, второй операнд: ");
        if (scanf("%lf %c %lf", &num1, &oper, &num2) != 3) {
            puts("Ошибка ввода. Пример: 2.5 + 3");
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        int c; while ((c = getchar()) != '\n' && c != EOF) {}

        switch (oper) {
            case '+':
                error = add(&ans, 2, num1, num2);
                if (!error) printf("Ответ: %.10g\n", ans);
                break;

            case '-':
                error = subtract(&ans, 2, num1, num2);
                if (!error) printf("Ответ: %.10g\n", ans);
                break;

            case '*':
                error = multiply(&ans, 2, num1, num2);
                if (!error) printf("Ответ: %.10g\n", ans);
                break;

            case '/':
                error = divide(&ans, 2, num1, num2);
                if (error == CALC_ERR_DIV_ZERO) {
                    puts("Ошибка: деление на ноль.");
                } else if (!error) {
                    printf("Ответ: %.10g\n", ans);
                }
                break;

            default:
                puts("Неизвестная операция. Используйте + - * /");
                break;
        }

        printf("Продолжать (y/n)?: ");
        if (scanf(" %c", &cont) != 1) {
            fprintf(stderr, "Ошибка ввода\n");
            return 0;
        }

        while ((c = getchar()) != '\n' && c != EOF) {}
    } while (cont != 'n' && cont != 'N');

    return 0;
}

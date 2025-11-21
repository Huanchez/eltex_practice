#include <stdio.h>
#include "calculator.h"

static void print_ops(void){
    char syms[64];
    int n = calc_list(syms, (int)sizeof syms);
    if (n<=0){ puts("Нет операций."); return; }
    printf("Доступные операции: ");
    for(int i=0;i<n;++i){ printf("%c", syms[i]); if(i+1<n) printf(", "); }
    printf("\n");
}

int main(void){
    calc_register('+', add);
    calc_register('-', subtract);
    calc_register('*', multiply);
    calc_register('/', divide);

    char oper, cont='y';
    double num1, num2, ans=0.0;

    do{
        print_ops();
        printf("Введите первый операнд, операцию, второй операнд: ");
        if (scanf("%lf %c %lf", &num1, &oper, &num2) != 3){
            puts("Ошибка ввода. Пример: 2.5 + 3");
            int cc; while((cc=getchar())!='\n' && cc!=EOF){}
            continue;
        }
        int cc; while((cc=getchar())!='\n' && cc!=EOF){}

        double argv[2] = {num1, num2};
        int rc = calc_apply(oper, &ans, 2, argv);

        if (rc == CALC_OK){
            printf("Ответ: %.10g\n", ans);
        } else if (rc == CALC_ERR_NO_SUCH_OP){
            puts("Ошибка: нераспознанная операция.");
        } else if (rc == CALC_ERR_DIV_ZERO){
            puts("Ошибка: деление на ноль.");
        } else {
            printf("Ошибка (код %d)\n", rc);
        }

        printf("Продолжать (y/n)?: ");
        if (scanf(" %c", &cont) != 1) break;
        while((cc=getchar())!='\n' && cc!=EOF){}
        puts("");
    } while(cont!='n' && cont!='N');

    return 0;
}

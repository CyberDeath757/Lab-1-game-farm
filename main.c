
#include <stdio.h> 

#define INVENTORY_SIZE 10 

int main(void) 
{ 
    int current_day = 1; 
    int current_hour = 8; 
 
    int inventory[INVENTORY_SIZE] = { 
        4, 6, 0, 2, 0, 3, 8, 0, 1, 0 
    }; 
 
    const char *item_names[10] = { 
        "пусто", "дерево", "камень", "семена", "лопата", 
        "грабли", "тяпка", "телега", "лейка", "корзина" 
    }; 
 
    int choice; 
 
    while (1) 
    { 
        printf("\n===== ФЕРМА =====\n"); 
        printf("0 - Выход\n"); 
        printf("1 - Текущее время\n"); 
        printf("2 - Промотать время\n"); 
        printf("3 - Инвентарь\n"); 
        printf("4 - Положить предмет\n"); 
        printf("5 - Выбросить предмет\n"); 
        printf("6 - Любимый ресурс\n"); 
        printf("7 - Выбросить предмет по ID\n"); 
        printf("Выберите действие: "); 
 
        if (scanf("%d", &choice) != 1) 
        { 
            printf("Ошибка: нужно ввести число.\n"); 
 
            while (getchar() != '\n') 
                ; 
 
            continue; 
        } 
 
        switch (choice) 
        { 
            case 0: 
                printf("Программа завершена.\n"); 
                return 0; 
 
            case 1: 
                printf( 
                    "Сейчас день %d, %02d:00\n", 
                    current_day, 
                    current_hour 
                ); 
                break; 
 
            case 2: 
            { 
                int hours; 
 
                printf("Введите количество часов: "); 
 
                if (scanf("%d", &hours) != 1) 
                { 
                    printf("Ошибка: нужно ввести число.\n"); 
 
                    while (getchar() != '\n') 
                        ; 
 
                    break; 
                } 
 
                if (hours < 0) 
                { 
                    printf( 
                        "Ошибка: количество часов не может быть отрицательным.\n" 
                    ); 
                    break; 
                } 
 
                current_hour += hours; 
 
                while (current_hour >= 24) 
                { 
                    current_hour -= 24; 
                    current_day++; 
                } 
 
                printf( 
                    "Время перемотано на %d часов.\n", 
                    hours 
                ); 
 
                break; 
            } 
 
            default: 
                printf("Ошибка: такого пункта меню нет.\n"); 
        } 
    } 
 
    return 0; 
}

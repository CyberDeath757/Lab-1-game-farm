
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
        "пусто", 
        "дерево", 
        "камень", 
        "семена", 
        "лопата", 
        "грабли", 
        "тяпка", 
        "телега", 
        "лейка", 
        "корзина" 
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

            default: 
                printf("Ошибка: такого пункта меню нет.\n"); 
        } 
    } 
 
    return 0; 
}

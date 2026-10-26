#include <stdio.h>
#include <string.h>

#define INVENTORY_SIZE 10
#define ITEM_COUNT 10
#define MAX_NAME_LEN 32

int main(void)
{
    int current_day = 1;
    int current_hour = 8;

    int inventory[INVENTORY_SIZE] = {
        4, 6, 0, 2, 0, 3, 8, 0, 1, 0
    };

    char item_names[ITEM_COUNT][MAX_NAME_LEN] = {
        "Пусто", "Дерево", "Камень", "Семена", "Лопата",
        "Грабли", "Тяпка", "Телега", "Лейка", "Корзина"
    };

    char farmer_name[MAX_NAME_LEN];
    int choice;

    FILE *file = fopen("items.txt", "r");

    if (file == NULL)
    {
        printf("Предупреждение: items.txt не найден.\n");
        printf("Используются названия по умолчанию.\n");
    }
    else
    {
        int item_id;
        char item_name[MAX_NAME_LEN];

        while (fscanf(file, "%d %31s", &item_id, item_name) == 2)
        {
            if (item_id >= 0 && item_id < ITEM_COUNT)
            {
                strcpy(item_names[item_id], item_name);
            }
        }

        fclose(file);
        printf("Каталог предметов загружен.\n");
    }

    printf("Введите имя фермера: ");

    if (fgets(farmer_name, MAX_NAME_LEN, stdin) == NULL)
    {
        strcpy(farmer_name, "Фермер");
    }
    else
    {
        size_t length = strlen(farmer_name);

        if (length > 0 && farmer_name[length - 1] == '\n')
        {
            farmer_name[length - 1] = '\0';
        }
        else
        {
            int ch;

            while ((ch = getchar()) != '\n' && ch != EOF)
            {
            }
        }

        if (farmer_name[0] == '\0')
        {
            strcpy(farmer_name, "Фермер");
        }
    }

    printf("\nДобро пожаловать на ферму, %s!\n", farmer_name);

    while (1)
    {
        printf("\n===== ФЕРМА =====\n");
        printf("Фермер: %s\n", farmer_name);
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

        while (getchar() != '\n')
            ;

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

                while (getchar() != '\n')
                    ;

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

                printf("Время перемотано на %d часов.\n", hours);
                break;
            }

            case 3:
                printf("\n===== ИНВЕНТАРЬ =====\n");

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    printf(
                        "Слот %d: ID %d (%s)\n",
                        i,
                        inventory[i],
                        item_names[inventory[i]]
                    );
                }

                break;

            case 4:
            {
                int slot;
                int item_id;

                printf("Введите номер слота (0-9): ");

                if (scanf("%d", &slot) != 1)
                {
                    printf("Ошибка: нужно ввести число.\n");

                    while (getchar() != '\n')
                        ;

                    break;
                }

                while (getchar() != '\n')
                    ;

                if (slot < 0 || slot >= INVENTORY_SIZE)
                {
                    printf("Ошибка: такого слота нет.\n");
                    break;
                }

                printf("Введите ID предмета (0-9): ");

                if (scanf("%d", &item_id) != 1)
                {
                    printf("Ошибка: нужно ввести число.\n");

                    while (getchar() != '\n')
                        ;

                    break;
                }

                while (getchar() != '\n')
                    ;

                if (item_id < 0 || item_id >= ITEM_COUNT)
                {
                    printf("Ошибка: неверный ID предмета.\n");
                    break;
                }

                inventory[slot] = item_id;

                printf(
                    "В слот %d добавлен предмет: %s\n",
                    slot,
                    item_names[item_id]
                );

                break;
            }

            case 5:
            {
                int slot;

                printf("Введите номер слота для очистки (0-9): ");

                if (scanf("%d", &slot) != 1)
                {
                    printf("Ошибка: нужно ввести число.\n");

                    while (getchar() != '\n')
                        ;

                    break;
                }

                while (getchar() != '\n')
                    ;

                if (slot < 0 || slot >= INVENTORY_SIZE)
                {
                    printf("Ошибка: такого слота нет.\n");
                    break;
                }

                printf("Удалён предмет: %s\n", item_names[inventory[slot]]);
                inventory[slot] = 0;
                break;
            }

            case 6:
            {
                int favorite_id = 0;
                int max_count = 0;

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    if (inventory[i] == 0)
                        continue;

                    int current_count = 0;

                    for (int j = 0; j < INVENTORY_SIZE; j++)
                    {
                        if (inventory[j] == inventory[i])
                            current_count++;
                    }

                    if (current_count > max_count)
                    {
                        max_count = current_count;
                        favorite_id = inventory[i];
                    }
                }

                if (favorite_id == 0)
                {
                    printf("В инвентаре нет предметов.\n");
                }
                else if (max_count == 1)
                {
                    printf("Все предметы встречаются по одному разу.\n");
                }
                else
                {
                    printf(
                        "Любимый ресурс: %s (ID %d)\n",
                        item_names[favorite_id],
                        favorite_id
                    );

                    printf("Количество: %d\n", max_count);
                }

                break;
            }

            case 7:
            {
                int item_id;
                int cleared_slots = 0;

                printf("Введите ID предмета для удаления: ");

                if (scanf("%d", &item_id) != 1)
                {
                    printf("Ошибка: нужно ввести число.\n");

                    while (getchar() != '\n')
                        ;

                    break;
                }

                while (getchar() != '\n')
                    ;

                if (item_id < 1 || item_id >= ITEM_COUNT)
                {
                    printf("Ошибка: неверный ID предмета.\n");
                    break;
                }

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    if (inventory[i] == item_id)
                    {
                        inventory[i] = 0;
                        cleared_slots++;
                    }
                }

                printf("Очищено слотов: %d\n", cleared_slots);
                break;
            }

            default:
                printf("Ошибка: такого пункта меню нет.\n");
        }
    }

    return 0;
}

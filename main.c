#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <wchar.h>
#include <errno.h>

#define INVENTORY_SIZE 10
#define ITEM_COUNT 10
#define MAX_NAME_LEN 64
#define MAX_INPUT_LEN 256

int read_text(char *text, int size)
{
    wchar_t wide_text[MAX_INPUT_LEN];
    char utf8_text[MAX_INPUT_LEN * 4];
    DWORD read_count;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (!ReadConsoleW(input, wide_text, MAX_INPUT_LEN - 1,
                      &read_count, NULL))
    {
        text[0] = '\0';
        return 0;
    }

    while (read_count > 0 &&
           (wide_text[read_count - 1] == L'\n' ||
            wide_text[read_count - 1] == L'\r'))
    {
        read_count--;
    }

    wide_text[read_count] = L'\0';

    int bytes = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS,
        wide_text, -1,
        utf8_text, sizeof(utf8_text),
        NULL, NULL
    );

    if (bytes == 0 || bytes > size)
    {
        text[0] = '\0';
        return 0;
    }

    memcpy(text, utf8_text, bytes);
    return 1;
}

int read_number(const char *prompt, int *number)
{
    char input[MAX_INPUT_LEN];
    char *end;
    long value;

    printf("%s", prompt);

    if (!read_text(input, sizeof(input)))
        return 0;

    errno = 0;
    value = strtol(input, &end, 10);

    while (*end == ' ' || *end == '\t')
        end++;

    if (input == end || *end != '\0' ||
        errno == ERANGE || value < -2147483647L - 1 ||
        value > 2147483647L)
    {
        return 0;
    }

    *number = (int)value;
    return 1;
}

void load_items(char item_names[ITEM_COUNT][MAX_NAME_LEN])
{
    FILE *file = fopen("items.txt", "r");

    if (file == NULL)
    {
        printf("Предупреждение: items.txt не найден.\n");
        printf("Используются названия по умолчанию.\n");
        return;
    }

    char line[256];
    int loaded = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        int item_id;
        char item_name[MAX_NAME_LEN];
        char extra;

        if (sscanf(line, "%d %63s %c",
                   &item_id, item_name, &extra) >= 2)
        {
            if (item_id >= 0 && item_id < ITEM_COUNT)
            {
                strcpy(item_names[item_id], item_name);
                loaded++;
            }
        }
    }

    fclose(file);

    if (loaded > 0)
        printf("Каталог предметов загружен.\n");
    else
        printf("Предупреждение: каталог пуст или имеет неверный формат.\n");
}

int main(void)
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Ферма");

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

    load_items(item_names);

    printf("Введите имя фермера: ");

    if (!read_text(farmer_name, sizeof(farmer_name)) ||
        farmer_name[0] == '\0')
    {
        strcpy(farmer_name, "Фермер");
    }

    printf("\nДобро пожаловать на ферму, %s!\n", farmer_name);

    while (1)
    {
        int choice;

        printf("\n===== ФЕРМА =====\n");
        printf("Фермер: %s\n", farmer_name);
        printf("0 - Выход\n");
        printf("1 - Текущее время\n");
        printf("2 - Промотать время\n");
        printf("3 - Инвентарь\n");
        printf("4 - Положить предмет\n");
        printf("5 - Выбросить предмет\n");
        printf("6 - Любимый ресурс\n");
        printf("7 - Поиск предмета по названию\n");
        printf("10 - Выбросить предмет по ID\n");

        if (!read_number("Выберите действие: ", &choice))
        {
            printf("Ошибка: нужно ввести число.\n");
            continue;
        }

        switch (choice)
        {
            case 0:
                printf("Программа завершена.\n");
                return 0;

            case 1:
                printf("Сейчас день %d, %02d:00\n",
                       current_day, current_hour);
                break;

            case 2:
            {
                int hours;

                if (!read_number("Введите количество часов: ", &hours))
                {
                    printf("Ошибка: нужно ввести число.\n");
                    break;
                }

                if (hours < 0)
                {
                    printf("Ошибка: количество часов не может быть отрицательным.\n");
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
                    printf("Слот %d: ID %d (%s)\n",
                           i, inventory[i], item_names[inventory[i]]);
                }

                break;

            case 4:
            {
                int slot;
                int item_id;

                if (!read_number("Введите номер слота (0-9): ", &slot))
                {
                    printf("Ошибка: нужно ввести число.\n");
                    break;
                }

                if (slot < 0 || slot >= INVENTORY_SIZE)
                {
                    printf("Ошибка: такого слота нет.\n");
                    break;
                }

                if (!read_number("Введите ID предмета (0-9): ", &item_id))
                {
                    printf("Ошибка: нужно ввести число.\n");
                    break;
                }

                if (item_id < 0 || item_id >= ITEM_COUNT)
                {
                    printf("Ошибка: неверный ID предмета.\n");
                    break;
                }

                inventory[slot] = item_id;

                printf("В слот %d добавлен предмет: %s\n",
                       slot, item_names[item_id]);
                break;
            }

            case 5:
            {
                int slot;

                if (!read_number("Введите номер слота для очистки (0-9): ", &slot))
                {
                    printf("Ошибка: нужно ввести число.\n");
                    break;
                }

                if (slot < 0 || slot >= INVENTORY_SIZE)
                {
                    printf("Ошибка: такого слота нет.\n");
                    break;
                }

                printf("Удалён предмет: %s\n",
                       item_names[inventory[slot]]);
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
                    printf("Любимый ресурс: %s (ID %d)\n",
                           item_names[favorite_id], favorite_id);
                    printf("Количество: %d\n", max_count);
                }

                break;
            }

            case 7:
            {
                char search_name[MAX_NAME_LEN];
                int search_id = -1;
                int found_slot = 0;

                printf("Введите название предмета: ");

                if (!read_text(search_name, sizeof(search_name)))
                {
                    printf("Ошибка чтения названия.\n");
                    break;
                }

                for (int i = 0; i < ITEM_COUNT; i++)
                {
                    if (strcmp(item_names[i], search_name) == 0)
                    {
                        search_id = i;
                        break;
                    }
                }

                if (search_id == -1)
                {
                    printf("Такого предмета нет в каталоге.\n");
                    break;
                }

                printf("Предмет \"%s\": ", item_names[search_id]);

                for (int i = 0; i < INVENTORY_SIZE; i++)
                {
                    if (inventory[i] == search_id)
                    {
                        printf("слот %d ", i);
                        found_slot = 1;
                    }
                }

                if (!found_slot)
                    printf("в инвентаре отсутствует");

                printf("\n");
                break;
            }

            case 10:
            {
                int item_id;
                int cleared_slots = 0;

                if (!read_number("Введите ID предмета для удаления: ", &item_id))
                {
                    printf("Ошибка: нужно ввести число.\n");
                    break;
                }

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
}

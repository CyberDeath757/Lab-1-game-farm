#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <wchar.h>
#include <errno.h>
#include <time.h>

#define INVENTORY_SIZE 10
#define ITEM_COUNT 10
#define MAX_NAME_LEN 64
#define MAX_INPUT_LEN 256
#define MAX_ARTICLE_LEN 512
#define SEARCH_FILE "input.txt"
#define OUTPUT_FILE "output.txt"

/* Константы варианта 7: "Башня и руны" */
#define MAP_MIN_SIZE 5
#define MAP_MAX_SIZE 30
#define WALL '#'
#define FLOOR '.'
#define HERO '@'
#define STAIRS_DOWN 'D'
#define STAIRS_UP 'U'
#define TREE 'T'
#define ROCK 'R'
#define TREE_CHANCE 15
#define ROCK_CHANCE 10
#define RUNE_CHANCE 8

/* Ввод */

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

/* Загрузка списка предметов */

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

void write_diary(const char *farmer_name,
                 int current_day,
                 int current_hour,
                 const int inventory[INVENTORY_SIZE],
                 char item_names[ITEM_COUNT][MAX_NAME_LEN])
{
    FILE *file = fopen("diary.txt", "a");

    if (file == NULL)
    {
        printf("Ошибка: не удалось открыть diary.txt для записи.\n");
        return;
    }

    fprintf(file, "===== Дневник фермера =====\n");
    fprintf(file, "Фермер: %s\n", farmer_name);
    fprintf(file, "День %d, %02d:00\n", current_day, current_hour);
    fprintf(file, "Инвентарь:\n");

    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        fprintf(file, "  Слот %d: ID %d (%s)\n",
                i, inventory[i], item_names[inventory[i]]);
    }

    fprintf(file, "\n");
    fclose(file);

    printf("Состояние фермы записано в diary.txt\n");
}

/* Энциклопедия */

int contains_word_ci(const char *line, const char *word)
{
    size_t word_len = strlen(word);

    if (word_len == 0)
        return 0;

    for (const char *p = line; *p != '\0'; p++)
    {
        size_t i = 0;
        while (i < word_len &&
               p[i] != '\0' &&
               tolower((unsigned char)p[i]) ==
               tolower((unsigned char)word[i]))
        {
            i++;
        }

        if (i == word_len)
            return 1;
    }

    return 0;
}

void wrap_word_in_brackets(const char *src, const char *word,
                           char *dest, int dest_size)
{
    size_t word_len = strlen(word);
    int out = 0;

    if (word_len == 0 || dest_size <= 0)
    {
        dest[0] = '\0';
        return;
    }

    for (size_t i = 0; src[i] != '\0' && out < dest_size - 1; )
    {
        int match = 1;

        for (size_t j = 0; j < word_len; j++)
        {
            if (src[i + j] == '\0' ||
                tolower((unsigned char)src[i + j]) !=
                tolower((unsigned char)word[j]))
            {
                match = 0;
                break;
            }
        }

        if (match)
        {
            if (out < dest_size - 1) dest[out++] = '[';
            for (size_t j = 0; j < word_len && out < dest_size - 1; j++)
                dest[out++] = src[i + j];
            if (out < dest_size - 1) dest[out++] = ']';
            i += word_len;
        }
        else
        {
            dest[out++] = src[i++];
        }
    }

    dest[out] = '\0';
}

void process_encyclopedia(const char *search_word)
{
    FILE *in = fopen(SEARCH_FILE, "r");

    if (in == NULL)
    {
        printf("Ошибка: не удалось открыть %s.\n", SEARCH_FILE);
        return;
    }

    FILE *out = fopen(OUTPUT_FILE, "w");

    if (out == NULL)
    {
        printf("Ошибка: не удалось открыть %s для записи.\n", OUTPUT_FILE);
        fclose(in);
        return;
    }

    char line[MAX_ARTICLE_LEN];
    int line_number = 0;
    int found_count = 0;

    printf("\n===== РЕЗУЛЬТАТЫ ПОИСКА =====\n");

    while (fgets(line, sizeof(line), in) != NULL)
    {
        line_number++;

        size_t len = strlen(line);
        while (len > 0 &&
               (line[len - 1] == '\n' || line[len - 1] == '\r'))
        {
            line[--len] = '\0';
        }

        if (!contains_word_ci(line, search_word))
            continue;

        char wrapped[MAX_ARTICLE_LEN];
        wrap_word_in_brackets(line, search_word,
                              wrapped, sizeof(wrapped));

        printf("Статья %d: %s\n", line_number, wrapped);
        fprintf(out, "Статья %d: %s\n", line_number, wrapped);

        found_count++;
    }

    if (found_count == 0)
    {
        printf("Статей с таким словом не найдено.\n");
        fprintf(out, "Статей с таким словом не найдено.\n");
    }
    else
    {
        printf("Всего найдено статей: %d\n", found_count);
    }

    fclose(in);
    fclose(out);

    printf("Результат сохранён в %s\n", OUTPUT_FILE);
}


/* ВАРИАНТ 7: БАШНЯ — Структура и управление памятью */

typedef struct
{
    char **grid;
    int width;
    int height;
    int hero_x;
    int hero_y;
    int stairs_down_x;
    int stairs_down_y;
    int stairs_up_x;
    int stairs_up_y;
    int floor_number;
} TowerMap;

typedef struct
{
    int **rune;
} RuneLayer;

static void free_map(TowerMap *map)
{
    if (map == NULL || map->grid == NULL)
        return;

    for (int y = 0; y < map->height; y++)
        free(map->grid[y]);

    free(map->grid);
    map->grid = NULL;
}

static void free_runes(RuneLayer *runes, int height)
{
    if (runes == NULL || runes->rune == NULL)
        return;

    for (int y = 0; y < height; y++)
        free(runes->rune[y]);

    free(runes->rune);
    runes->rune = NULL;
}

static int alloc_map(TowerMap *map, int height, int width)
{
    map->grid = (char **)malloc(height * sizeof(char *));
    if (map->grid == NULL)
        return 0;

    for (int y = 0; y < height; y++)
    {
        map->grid[y] = (char *)malloc(width * sizeof(char));
        if (map->grid[y] == NULL)
        {
            for (int i = 0; i < y; i++)
                free(map->grid[i]);
            free(map->grid);
            map->grid = NULL;
            return 0;
        }
    }

    map->height = height;
    map->width = width;
    return 1;
}

static int alloc_runes(RuneLayer *runes, int height, int width)
{
    runes->rune = (int **)malloc(height * sizeof(int *));
    if (runes->rune == NULL)
        return 0;

    for (int y = 0; y < height; y++)
    {
        runes->rune[y] = (int *)calloc(width, sizeof(int));
        if (runes->rune[y] == NULL)
        {
            for (int i = 0; i < y; i++)
                free(runes->rune[i]);
            free(runes->rune);
            runes->rune = NULL;
            return 0;
        }
    }

    return 1;
}


/* Генерация этажа башни.
 * D - находится под героем на стартовой клетке.
 * U - всегда размещается на свободной клетке этажа.
 */
static int generate_floor(TowerMap *map, RuneLayer *runes, int floor_number)
{
    int height = MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);
    int width = MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);

    if (!alloc_map(map, height, width))
        return 0;

    if (!alloc_runes(runes, height, width))
    {
        free_map(map);
        return 0;
    }

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (y == 0 || y == height - 1 || x == 0 || x == width - 1)
                map->grid[y][x] = WALL;
            else
            {
                int roll = rand() % 100;
                if (roll < TREE_CHANCE)
                    map->grid[y][x] = TREE;
                else if (roll < TREE_CHANCE + ROCK_CHANCE)
                    map->grid[y][x] = ROCK;
                else
                    map->grid[y][x] = FLOOR;
            }
        }
    }

    /* Стартовая клетка героя — лестница вниз D под персонажем. */
    int placed = 0;
    for (int attempt = 0; attempt < 1000 && !placed; attempt++)
    {
        int x = 1 + rand() % (width - 2);
        int y = 1 + rand() % (height - 2);
        if (map->grid[y][x] == FLOOR)
        {
            map->hero_x = x;
            map->hero_y = y;
            map->stairs_down_x = x;
            map->stairs_down_y = y;
            map->grid[y][x] = HERO;
            placed = 1;
        }
    }

    if (!placed)
    {
        map->hero_x = width / 2;
        map->hero_y = height / 2;
        map->stairs_down_x = map->hero_x;
        map->stairs_down_y = map->hero_y;
        map->grid[map->hero_y][map->hero_x] = HERO;
    }

    /* Лестница вверх U — минимум одна на каждом этаже. */
    map->stairs_up_x = -1;
    map->stairs_up_y = -1;

    for (int attempt = 0; attempt < 2000 && map->stairs_up_x == -1; attempt++)
    {
        int x = 1 + rand() % (width - 2);
        int y = 1 + rand() % (height - 2);
        if (map->grid[y][x] == FLOOR)
        {
            map->stairs_up_x = x;
            map->stairs_up_y = y;
            map->grid[y][x] = STAIRS_UP;
        }
    }

    if (map->stairs_up_x == -1)
    {
        for (int y = 1; y < height - 1 && map->stairs_up_x == -1; y++)
            for (int x = 1; x < width - 1 && map->stairs_up_x == -1; x++)
                if (map->grid[y][x] == FLOOR)
                {
                    map->stairs_up_x = x;
                    map->stairs_up_y = y;
                    map->grid[y][x] = STAIRS_UP;
                }
    }

    /* Руны размещаются только на обычных свободных клетках. */
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            if (map->grid[y][x] == FLOOR && rand() % 100 < RUNE_CHANCE)
                runes->rune[y][x] = 1;
        }
    }

    map->floor_number = floor_number;
    return 1;
}


/* Отображение текущего этажа. Активные руны обозначаются символом ^. */
static void render_map(const TowerMap *map, const RuneLayer *runes)
{
    printf("\n===== БАШНЯ: ЭТАЖ %d =====\n", map->floor_number);
    printf("W/A/S/D — движение, Q — домой (только на D)\n\n");

    for (int y = 0; y < map->height; y++)
    {
        for (int x = 0; x < map->width; x++)
        {
            if (runes->rune[y][x])
                putchar('^');
            else
                putchar(map->grid[y][x]);
        }
        putchar('\n');
    }
}

static int is_walkable(const TowerMap *map, int x, int y)
{
    if (x < 0 || x >= map->width || y < 0 || y >= map->height)
        return 0;

    char cell = map->grid[y][x];
    return cell != WALL && cell != TREE && cell != ROCK;
}


/* Выбирает случайную свободную клетку для телепортации. */
static int teleport_to_empty_cell(TowerMap *map, RuneLayer *runes)
{
    int available = 0;

    for (int y = 1; y < map->height - 1; y++)
        for (int x = 1; x < map->width - 1; x++)
            if (map->grid[y][x] == FLOOR && !runes->rune[y][x])
                available++;

    if (available == 0)
        return 0;

    int selected = rand() % available;

    for (int y = 1; y < map->height - 1; y++)
    {
        for (int x = 1; x < map->width - 1; x++)
        {
            if (map->grid[y][x] == FLOOR && !runes->rune[y][x])
            {
                if (selected == 0)
                {
                    map->hero_x = x;
                    map->hero_y = y;
                    map->grid[y][x] = HERO;
                    printf("Руна телепортировала вас в другое место башни!\n");
                    return 1;
                }
                selected--;
            }
        }
    }

    return 0;
}

/* При наступании на ^ руна исчезает и переносит героя на случайный пол. */
static int try_move(TowerMap *map, RuneLayer *runes, int dx, int dy)
{
    int nx = map->hero_x + dx;
    int ny = map->hero_y + dy;

    if (!is_walkable(map, nx, ny))
    {
        printf("Туда нельзя — стена или препятствие.\n");
        return 0;
    }

    int stepped_on_rune = runes->rune[ny][nx];

    if (map->hero_x == map->stairs_down_x &&
        map->hero_y == map->stairs_down_y)
        map->grid[map->hero_y][map->hero_x] = STAIRS_DOWN;
    else
        map->grid[map->hero_y][map->hero_x] = FLOOR;

    map->hero_x = nx;
    map->hero_y = ny;

    if (map->grid[ny][nx] == STAIRS_UP)
    {
        map->grid[ny][nx] = HERO;
        return 2;
    }

    if (stepped_on_rune)
    {
        runes->rune[ny][nx] = 0;
        map->grid[ny][nx] = FLOOR;
        if (!teleport_to_empty_cell(map, runes))
        {
            map->grid[ny][nx] = HERO;
            printf("Руна погасла, но свободного места для телепортации нет.\n");
        }
        return 1;
    }

    map->grid[ny][nx] = HERO;
    return 1;
}

/* Спросить направление у пользователя.
 * Возвращает: 0 — выход/некорректно, 1 — dx/dy заданы.
 */
static int ask_direction(int *dx, int *dy)
{
    char input[MAX_INPUT_LEN];
    printf("Направление (W/A/S/D, Q — выход): ");
    if (!read_text(input, sizeof(input)))
        return 0;

    /* первый непробельный символ */
    char c = 0;
    for (int i = 0; input[i] != '\0'; i++)
    {
        if (input[i] != ' ' && input[i] != '\t')
        {
            c = (char)tolower((unsigned char)input[i]);
            break;
        }
    }

    *dx = 0;
    *dy = 0;
    switch (c)
    {
        case 'w': *dy = -1; return 1;
        case 's': *dy = 1;  return 1;
        case 'a': *dx = -1; return 1;
        case 'd': *dx = 1;  return 1;
        case 'q': return 0;
        default:
            printf("Неизвестное направление. Используйте W/A/S/D или Q.\n");
            return 0;
    }
}



/* Главный цикл башни. Возвращает 1 после успешного выхода домой. */
static int run_tower_adventure(int start_floor)
{
    TowerMap map = {0};
    RuneLayer runes = {0};
    int floor_number = start_floor;

    while (1)
    {
        free_map(&map);
        free_runes(&runes, map.height);

        if (!generate_floor(&map, &runes, floor_number))
        {
            printf("Не удалось выделить память для этажа башни.\n");
            free_map(&map);
            free_runes(&runes, map.height);
            return 1;
        }

        /* Новый этаж создаётся с D под героем. Q доступна на этой клетке. */
        while (1)
        {
            render_map(&map, &runes);

            int dx = 0;
            int dy = 0;

            if (!ask_direction(&dx, &dy))
            {
                if (map.hero_x == map.stairs_down_x &&
                    map.hero_y == map.stairs_down_y)
                {
                    printf("Вы вышли из башни и вернулись домой!\n");
                    free_map(&map);
                    free_runes(&runes, map.height);
                    return 1;
                }

                printf("Выход домой возможен только на лестнице вниз D.\n");
                continue;
            }

            int result = try_move(&map, &runes, dx, dy);

            if (result == 2)
            {
                printf("Вы поднялись по лестнице на следующий этаж...\n");
                floor_number++;
                break;
            }
        }
    }
}

/* MAIN */

int main(void)
{
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Ферма");

    srand((unsigned)time(NULL));

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
        printf("8 - Записать состояние в дневник\n");
        printf("9 - Расшифровать старые записи (Энциклопедия)\n");
        printf("10 - Выбросить предмет по ID\n");
        printf("11 - Отправиться в приключение (Башня)\n");

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

            case 8:
                write_diary(farmer_name, current_day, current_hour,
                            inventory, item_names);
                break;

            case 9:
            {
                char search_word[MAX_NAME_LEN];

                printf("Введите поисковое слово: ");

                if (!read_text(search_word, sizeof(search_word)) ||
                    search_word[0] == '\0')
                {
                    printf("Ошибка: пустое поисковое слово.\n");
                    break;
                }

                process_encyclopedia(search_word);
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

            case 11:
                printf("\nВы входите в башню...\n");
                run_tower_adventure(1);
                printf("Вы вернулись на ферму.\n");
                break;

            default:
                printf("Ошибка: такого пункта меню нет.\n");
        }
    }
}

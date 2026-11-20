#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

/* Константы */

/* Общие лимиты */
#define INVENTORY_SIZE 10
#define ITEM_COUNT 10
#define MAX_NAME_LEN 64
#define MAX_INPUT_LEN 256
#define MAX_ARTICLE_LEN 512

/* Имена файлов */
#define SEARCH_FILE "input.txt"
#define OUTPUT_FILE "output.txt"
#define ITEMS_FILE "items.txt"
#define DIARY_FILE "diary.txt"

/* Время */
#define START_HOUR 8
#define NIGHT_HOUR 2

/* Коды пунктов главного меню */
#define MENU_EXIT 0
#define MENU_CREATE 1
#define MENU_CARD 2
#define MENU_TIME 3
#define MENU_INVENTORY 4
#define MENU_PUT_ITEM 5
#define MENU_DROP_SLOT 6
#define MENU_FAVORITE 7
#define MENU_FIND_ITEM 8
#define MENU_DIARY 9
#define MENU_ENCYCLOPEDIA 10
#define MENU_DROP_ID 11
#define MENU_ADVENTURE 12

/* Игрок */
#define BASE_MAX_HP 100
#define BASE_MAX_STAMINA 50
#define START_GOLD 50

/* Карта */
#define MAP_MIN_SIZE 7
#define MAP_MAX_SIZE 15
#define GOLD_BASE_MULT 10
#define FLOOR_STAMINA_COST 5
#define STEP_STAMINA_COST 1
#define MAP_TREE_CHANCE 15
#define MAP_ROCK_CHANCE 10

/* Символы клеток карты */
#define WALL_CHAR '#'
#define FLOOR_CHAR '.'
#define HERO_CHAR '@'
#define STAIRS_CHAR 'U'
#define TREE_CHAR 'T'
#define ROCK_CHAR 'R'

/* Общие данные */

static char item_names[ITEM_COUNT][MAX_NAME_LEN] = {
    "Пусто", "Дерево", "Камень", "Семена", "Лопата",
    "Грабли", "Тяпка", "Телега", "Лейка", "Корзина"
};

/* Ввод, файлы, строки */

static int read_text(char *text, int size)
{
#ifdef _WIN32
    wchar_t wide_text[MAX_INPUT_LEN];
    char utf8_text[MAX_INPUT_LEN * 4];
    DWORD read_count;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);

    if (!ReadConsoleW(input, wide_text, MAX_INPUT_LEN - 1, &read_count, NULL))
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

    int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        wide_text, -1,
        utf8_text, sizeof(utf8_text), NULL, NULL);
    if (bytes == 0 || bytes > size)
    {
        text[0] = '\0';
        return 0;
    }
    memcpy(text, utf8_text, bytes);
    return 1;
#else
    if (fgets(text, size, stdin) == NULL)
    {
        text[0] = '\0';
        return 0;
    }
    size_t len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r'))
    {
        text[--len] = '\0';
    }
    return 1;
#endif
}

static int read_number(const char *prompt, int *number)
{
    char input[MAX_INPUT_LEN];
    char *end;
    long value;

    printf("%s", prompt);
    if (!read_text(input, sizeof(input))) return 0;

    errno = 0;
    value = strtol(input, &end, 10);
    while (*end == ' ' || *end == '\t') end++;

    if (input == end || *end != '\0' || errno == ERANGE ||
        value < -2147483647L - 1 || value > 2147483647L)
    {
        return 0;
    }
    *number = (int)value;
    return 1;
}

static int read_number_in_range(const char *prompt, int *number, int min, int max)
{
    int value;
    if (!read_number(prompt, &value)) return 0;
    if (value < min || value > max) return 0;
    *number = value;
    return 1;
}

static void load_items(void)
{
    FILE *file = fopen(ITEMS_FILE, "r");
    if (!file)
    {
        printf("Предупреждение: %s не найден. Имена по умолчанию.\n", ITEMS_FILE);
        return;
    }

    char line[256];
    int loaded = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        int item_id;
        char item_name[MAX_NAME_LEN];
        if (sscanf(line, "%d %63s", &item_id, item_name) == 2)
        {
            if (item_id >= 0 && item_id < ITEM_COUNT)
            {
                strncpy(item_names[item_id], item_name, MAX_NAME_LEN - 1);
                item_names[item_id][MAX_NAME_LEN - 1] = '\0';
                loaded++;
            }
        }
    }
    fclose(file);

    if (loaded > 0)
    printf("Каталог предметов загружен (%d шт.).\n", loaded);
    else
    printf("Предупреждение: каталог пуст или неверный формат.\n");
}

/* ВАРИАНТ 3: "Система состояний и травм" */

typedef enum {
    STATUS_HEALTHY = 0,
    STATUS_STRAIN = 1,
    STATUS_POISON = 2,
    STATUS_ENERGIZED = 3
} StatusEffect;

typedef struct {
    StatusEffect type;
    int remaining_steps;
} Buff;

static void buff_clear(Buff *buff)
{
    if (!buff) return;
    buff->type = STATUS_HEALTHY;
    buff->remaining_steps = 0;
}

static int buff_is_active(const Buff *buff)
{
    return buff && buff->type != STATUS_HEALTHY && buff->remaining_steps > 0;
}

static const char *buff_name(StatusEffect type)
{
    switch (type)
    {
    case STATUS_STRAIN:
        return "Растяжение";
    case STATUS_POISON:
        return "Отравление";
    case STATUS_ENERGIZED:
        return "Бодрость";
    case STATUS_HEALTHY:
    default:
        return "Здоров";
    }
}

static void buff_print(const Buff *buff)
{
    if (!buff_is_active(buff))
    {
        printf("Состояние: %s\n", buff_name(STATUS_HEALTHY));
        return;
    }
    printf("Состояние: %s (осталось шагов: %d)\n",
        buff_name(buff->type), buff->remaining_steps);
}

/* Структура игрока и операции над ней */

typedef struct Player {
    char name[MAX_NAME_LEN];
    int hp;
    int max_hp;
    int stamina;
    int max_stamina;
    int gold;
    int x, y;
    int inventory[INVENTORY_SIZE];
    Buff buff;
} Player;

static void player_destroy(Player *player)
{
    if (!player) return;
    printf("Память структуры по адресу %p освобождена\n", (void *)player);
    free(player);
}

static int player_exists(const Player *player)
{
    if (!player)
    {
        printf("Сначала создайте персонажа (пункт меню [1])!\n");
        return 0;
    }
    return 1;
}

static Player *player_create(void)
{
    Player *p = (Player *)malloc(sizeof(Player));
    if (!p)
    {
        printf("Ошибка: не удалось выделить память под игрока.\n");
        return NULL;
    }

    memset(p, 0, sizeof(*p));
    buff_clear(&p->buff);

    printf("Введите имя фермера: ");
    if (!read_text(p->name, sizeof(p->name)) || p->name[0] == '\0')
    {
        strncpy(p->name, "Фермер", MAX_NAME_LEN - 1);
    }

    p->max_hp = BASE_MAX_HP;
    p->hp = BASE_MAX_HP;
    p->max_stamina = BASE_MAX_STAMINA;
    p->stamina = BASE_MAX_STAMINA;
    p->gold = START_GOLD;

    int start[INVENTORY_SIZE] = { 4, 6, 0, 2, 0, 3, 8, 0, 1, 0 };
    memcpy(p->inventory, start, sizeof(start));

    printf("Создан персонаж \"%s\" (адрес: %p)\n", p->name, (void *)p);
    return p;
}

static void player_restore_full(Player *player)
{
    if (!player) return;
    player->hp = player->max_hp;
    player->stamina = player->max_stamina;
}

static void player_add_gold(Player *player, int amount)
{
    if (!player || amount <= 0) return;
    player->gold += amount;
}

static void player_take_gold_percent(Player *player, int percent)
{
    if (!player || percent <= 0) return;
    int taken = player->gold * percent / 100;
    player->gold -= taken;
    printf("Списано золота: %d (осталось %d)\n", taken, player->gold);
}

static void player_take_damage(Player *player, int dmg)
{
    if (!player || dmg <= 0) return;
    player->hp -= dmg;
    if (player->hp < 0) player->hp = 0;
}

static int player_spend_stamina(Player *player, int amount)
{
    if (!player || amount <= 0) return 1;
    if (player->stamina < amount) return 0;
    player->stamina -= amount;
    return 1;
}

/* Карта: структура и управление памятью */

typedef struct Map {
    char **grid;
    int width;
    int height;
    int hero_x;
    int hero_y;
    int stairs_x;
    int stairs_y;
    int floor_number;
} Map;

static int map_alloc(Map *map, int height, int width)
{
    map->grid = (char **)malloc(height * sizeof(char *));
    if (!map->grid) return 0;

    for (int y = 0; y < height; y++)
    {
        map->grid[y] = (char *)malloc(width * sizeof(char));
        if (!map->grid[y])
        {
            for (int i = 0; i < y; i++) free(map->grid[i]);
            free(map->grid);
            map->grid = NULL;
            return 0;
        }
    }
    map->height = height;
    map->width = width;
    return 1;
}

static char map_random_cell(void)
{
    int roll = rand() % 100;
    if (roll < MAP_TREE_CHANCE)
    {
        return TREE_CHAR;
    }
    if (roll < MAP_TREE_CHANCE + MAP_ROCK_CHANCE)
    {
        return ROCK_CHAR;
    }
    return FLOOR_CHAR;
}

static void map_place_stairs(Map *map)
{
    int placed = 0;
    for (int attempt = 0; attempt < 2000 && !placed; attempt++)
    {
        int x = 1 + rand() % (map->width - 2);
        int y = 1 + rand() % (map->height - 2);
        if (map->grid[y][x] == FLOOR_CHAR &&
            !(x == map->hero_x && y == map->hero_y))
        {
            map->stairs_x = x;
            map->stairs_y = y;
            map->grid[y][x] = STAIRS_CHAR;
            placed = 1;
        }
    }
    if (!placed)
    {
        for (int y = 1; y < map->height - 1 && !placed; y++)
        {
            for (int x = 1; x < map->width - 1 && !placed; x++)
            {
                if (map->grid[y][x] == FLOOR_CHAR &&
                    !(x == map->hero_x && y == map->hero_y))
                {
                    map->stairs_x = x;
                    map->stairs_y = y;
                    map->grid[y][x] = STAIRS_CHAR;
                    placed = 1;
                }
            }
        }
    }
}

static Map *map_create(int floor_number)
{
    Map *map = (Map *)malloc(sizeof(Map));
    if (!map)
    {
        printf("Ошибка: не удалось выделить память под карту.\n");
        return NULL;
    }
    memset(map, 0, sizeof(*map));

    int h = MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);
    int w = MAP_MIN_SIZE + rand() % (MAP_MAX_SIZE - MAP_MIN_SIZE + 1);

    if (!map_alloc(map, h, w))
    {
        free(map);
        printf("Ошибка: не удалось выделить память под сетку карты.\n");
        return NULL;
    }

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            if (y == 0 || y == h - 1 || x == 0 || x == w - 1)
            map->grid[y][x] = WALL_CHAR;
            else
            map->grid[y][x] = map_random_cell();
        }
    }

    map->hero_x = 1 + rand() % (w - 2);
    map->hero_y = 1 + rand() % (h - 2);
    map->grid[map->hero_y][map->hero_x] = HERO_CHAR;

    map_place_stairs(map);

    map->floor_number = floor_number;
    printf("Память карты выделена (адрес: %p, этаж %d)\n",
        (void *)map, floor_number);
    return map;
}

static void map_destroy(Map *map)
{
    if (!map) return;
    if (map->grid)
    {
        for (int y = 0; y < map->height; y++) free(map->grid[y]);
        free(map->grid);
    }
    printf("Память структуры по адресу %p освобождена\n", (void *)map);
    free(map);
}

static int map_is_walkable(const Map *map, int x, int y)
{
    if (!map || x < 0 || x >= map->width || y < 0 || y >= map->height)
    return 0;
    char c = map->grid[y][x];
    return c != WALL_CHAR && c != TREE_CHAR && c != ROCK_CHAR;
}

static int map_hero_on_exit(const Map *map)
{
    return map && map->hero_x == map->stairs_x && map->hero_y == map->stairs_y;
}

/* Возвращает: 0 — стена, 1 — шаг, 2 — переход на этаж */
static int map_try_move(Map *map, int dx, int dy)
{
    if (!map) return 0;

    int nx = map->hero_x + dx;
    int ny = map->hero_y + dy;

    if (!map_is_walkable(map, nx, ny))
    {
        printf("Туда нельзя — стена или препятствие.\n");
        return 0;
    }

    if (map->grid[map->hero_y][map->hero_x] == HERO_CHAR)
    map->grid[map->hero_y][map->hero_x] = FLOOR_CHAR;

    map->hero_x = nx;
    map->hero_y = ny;

    if (map->grid[ny][nx] == STAIRS_CHAR)
    {
        map->grid[ny][nx] = HERO_CHAR;
        return 2;
    }
    map->grid[ny][nx] = HERO_CHAR;
    return 1;
}

/* Время и режим дня */

static void advance_time(int *day, int *hour, int hours)
{
    *hour += hours;
    while (*hour >= 24)
    {
        *hour -= 24;
        (*day)++;
    }
}

static int hours_until_night(int hour)
{
    if (hour >= NIGHT_HOUR) return 24 - hour + NIGHT_HOUR;
    return NIGHT_HOUR - hour;
}

/* Главное меню и подменю */

static void player_print_card(const Player *player, int day, int hour)
{
    if (!player_exists(player)) return;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%d.%m.%Y %H:%M", tm_info);

    printf("\n===== КАРТОЧКА ПЕРСОНАЖА =====\n");
    printf("Имя: %s\n", player->name);
    printf("HP: %d / %d\n", player->hp, player->max_hp);
    printf("Выносливость: %d / %d\n", player->stamina, player->max_stamina);
    printf("Золото: %d\n", player->gold);
    printf("Координаты: (%d, %d)\n", player->x, player->y);
    printf("Игровое время: День %d, %02d:00\n", day, hour);
    printf("Реальное время: %s\n", timebuf);
    buff_print(&player->buff);
    printf("Инвентарь:\n");
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        printf("  Слот %d: ID %d (%s)\n",
            i, player->inventory[i], item_names[player->inventory[i]]);
    }
}

static void menu_create(Player **player)
{
    if (*player != NULL)
    {
        int confirm;
        printf("Персонаж \"%s\" уже существует. Удалить и создать нового?\n",
            (*player)->name);
        if (!read_number_in_range("1 - Да, 0 - Нет: ", &confirm, 0, 1))
        {
            printf("Отменено.\n");
            return;
        }
        if (!confirm)
        {
            printf("Отменено.\n");
            return;
        }
        player_destroy(*player);
        *player = NULL;
    }
    *player = player_create();
}

static void menu_time(int *day, int *hour, Player *player)
{
    int choice;
    printf("\n===== ВРЕМЯ =====\n");
    printf("1 - Посмотреть на часы\n");
    printf("2 - Пропустить время\n");
    printf("3 - Отдохнуть (лечь спать)\n");

    if (!read_number("Выберите действие: ", &choice))
    {
        printf("Ошибка: нужно ввести число.\n");
        return;
    }

    switch (choice)
    {
    case 1:
        printf("Сейчас день %d, %02d:00\n", *day, *hour);
        break;

    case 2:
        {
            int hours;
            if (!read_number_in_range("Сколько часов ждать (1..24)? ",
                &hours, 1, 24))
            {
                printf("Ошибка: введите число от 1 до 24.\n");
                break;
            }
            int limit = hours_until_night(*hour);
            if (hours > limit)
            {
                printf("Нельзя: после 02:00 ночи нужно спать. Максимум %d ч.\n",
                    limit);
                break;
            }
            advance_time(day, hour, hours);
            printf("Прошло %d ч. Сейчас день %d, %02d:00\n", hours, *day, *hour);
            break;
        }

    case 3:
        (*day)++;
        *hour = START_HOUR;
        if (player)
        {
            player_restore_full(player);
            printf("Вы отдохнули. HP и выносливость восстановлены.\n");
        }
        printf("Наступил день %d, %02d:00\n", *day, *hour);
        break;

    default:
        printf("Неизвестный пункт.\n");
    }
}

static void menu_inventory(const Player *player)
{
    if (!player_exists(player)) return;
    printf("\n===== ИНВЕНТАРЬ =====\n");
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        printf("Слот %d: ID %d (%s)\n",
            i, player->inventory[i], item_names[player->inventory[i]]);
    }
}

static void menu_put_item(Player *player)
{
    if (!player_exists(player)) return;

    int slot, item_id;
    if (!read_number_in_range("Номер слота (0..9): ", &slot, 0, INVENTORY_SIZE - 1))
    {
        printf("Ошибка: слот должен быть 0..9.\n");
        return;
    }
    if (!read_number_in_range("ID предмета (0..9): ", &item_id, 0, ITEM_COUNT - 1))
    {
        printf("Ошибка: ID должен быть 0..9.\n");
        return;
    }
    player->inventory[slot] = item_id;
    printf("В слот %d добавлен: %s\n", slot, item_names[item_id]);
}

static void menu_drop_slot(Player *player)
{
    if (!player_exists(player)) return;

    int slot;
    if (!read_number_in_range("Номер слота (0..9): ", &slot, 0, INVENTORY_SIZE - 1))
    {
        printf("Ошибка: слот должен быть 0..9.\n");
        return;
    }
    printf("Удалён предмет: %s\n", item_names[player->inventory[slot]]);
    player->inventory[slot] = 0;
}

static void menu_favorite(const Player *player)
{
    if (!player_exists(player)) return;

    int favorite_id = 0, max_count = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        if (player->inventory[i] == 0) continue;
        int cnt = 0;
        for (int j = 0; j < INVENTORY_SIZE; j++)
        {
            if (player->inventory[j] == player->inventory[i]) cnt++;
        }
        if (cnt > max_count)
        {
            max_count = cnt;
            favorite_id = player->inventory[i];
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
        printf("Любимый ресурс: %s (ID %d), шт: %d\n",
            item_names[favorite_id], favorite_id, max_count);
    }
}

static void menu_find_item(const Player *player)
{
    if (!player_exists(player)) return;

    char search_name[MAX_NAME_LEN];
    int search_id = -1, found_slot = 0;

    printf("Введите название предмета: ");
    if (!read_text(search_name, sizeof(search_name)))
    {
        printf("Ошибка чтения названия.\n");
        return;
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
        return;
    }

    printf("Предмет \"%s\": ", item_names[search_id]);
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        if (player->inventory[i] == search_id)
        {
            printf("слот %d ", i);
            found_slot = 1;
        }
    }
    if (!found_slot) printf("в инвентаре отсутствует");
    printf("\n");
}

static void menu_drop_id(Player *player)
{
    if (!player_exists(player)) return;

    int item_id, cleared = 0;
    if (!read_number_in_range("ID для удаления (1..9): ", &item_id, 1, ITEM_COUNT - 1))
    {
        printf("Ошибка: ID должен быть 1..9.\n");
        return;
    }
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        if (player->inventory[i] == item_id)
        {
            player->inventory[i] = 0;
            cleared++;
        }
    }
    printf("Очищено слотов: %d\n", cleared);
}

static void write_diary(const Player *player, int day, int hour)
{
    FILE *file = fopen(DIARY_FILE, "a");
    if (!file)
    {
        printf("Ошибка: не удалось открыть %s для записи.\n", DIARY_FILE);
        return;
    }

    fprintf(file, "===== Дневник фермера =====\n");
    fprintf(file, "Фермер: %s\n", player->name);
    fprintf(file, "День %d, %02d:00\n", day, hour);
    fprintf(file, "HP: %d/%d, Выносливость: %d/%d, Золото: %d\n",
        player->hp, player->max_hp,
        player->stamina, player->max_stamina,
        player->gold);
    fprintf(file, "Инвентарь:\n");
    for (int i = 0; i < INVENTORY_SIZE; i++)
    {
        fprintf(file, "  Слот %d: ID %d (%s)\n",
            i, player->inventory[i], item_names[player->inventory[i]]);
    }
    fprintf(file, "\n");
    fclose(file);
    printf("Состояние записано в %s\n", DIARY_FILE);
}

static void menu_diary(const Player *player, int day, int hour)
{
    if (!player_exists(player)) return;
    write_diary(player, day, hour);
}

static int contains_word_ci(const char *line, const char *word)
{
    size_t word_len = strlen(word);
    if (word_len == 0) return 0;

    for (const char *p = line; *p != '\0'; p++)
    {
        size_t i = 0;
        while (i < word_len && p[i] != '\0' &&
            tolower((unsigned char)p[i]) ==
            tolower((unsigned char)word[i]))
        {
            i++;
        }
        if (i == word_len) return 1;
    }
    return 0;
}

static void wrap_word_in_brackets(const char *src, const char *word,
    char *dest, int dest_size)
{
    size_t word_len = strlen(word);
    int out = 0;

    if (word_len == 0 || dest_size <= 0)
    {
        dest[0] = '\0';
        return;
    }

    for (size_t i = 0; src[i] != '\0' && out < dest_size - 1;)
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

static void process_encyclopedia(const char *search_word)
{
    FILE *in = fopen(SEARCH_FILE, "r");
    if (!in)
    {
        printf("Ошибка: не удалось открыть %s.\n", SEARCH_FILE);
        return;
    }

    FILE *out = fopen(OUTPUT_FILE, "w");
    if (!out)
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
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
        line[--len] = '\0';

        if (!contains_word_ci(line, search_word)) continue;

        char wrapped[MAX_ARTICLE_LEN];
        wrap_word_in_brackets(line, search_word, wrapped, sizeof(wrapped));
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

static void menu_encyclopedia(void)
{
    char search_word[MAX_NAME_LEN];
    printf("Введите поисковое слово: ");
    if (!read_text(search_word, sizeof(search_word)) || search_word[0] == '\0')
    {
        printf("Ошибка: пустое поисковое слово.\n");
        return;
    }
    process_encyclopedia(search_word);
}

/* Поход по карте */

static void map_render(const Map *map, const Player *player, int day, int hour)
{
    if (!map) return;

    printf("\n===== ЭТАЖ %d =====\n", map->floor_number);
    printf("HP: %d/%d | Выносливость: %d/%d | Золото: %d | Этаж: %d | Время: День %d, %02d:00\n",
        player->hp, player->max_hp,
        player->stamina, player->max_stamina,
        player->gold, map->floor_number, day, hour);
    buff_print(&player->buff);
    printf("W/A/S/D — шаг, Q — выход (только на U)\n\n");

    for (int y = 0; y < map->height; y++)
    {
        for (int x = 0; x < map->width; x++)
        putchar(map->grid[y][x]);
        putchar('\n');
    }
    putchar('\n');
}

static int ask_direction(int *dx, int *dy)
{
    char input[MAX_INPUT_LEN];
    printf("Направление (W/A/S/D, Q — выход): ");
    if (!read_text(input, sizeof(input))) return -1;

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
    case 's': *dy = 1; return 1;
    case 'a': *dx = -1; return 1;
    case 'd': *dx = 1; return 1;
    case 'q': return 0;
    default:
        printf("Неизвестное направление. Используйте W/A/S/D или Q.\n");
        return -1;
    }
}

/* Возвращает: 1 — штатный выход, 0 — эвакуация */
static int run_adventure(Player *player, int *day, int *hour)
{
    int floor_number = 1;
    Map *map = map_create(floor_number);
    if (!map) return 0;

    while (1)
    {
        map_render(map, player, *day, *hour);

        int dx, dy;
        int dir = ask_direction(&dx, &dy);

        if (dir == 0)
        {
            if (map_hero_on_exit(map))
            {
                printf("Вы вышли из приключения штатно, золото при вас.\n");
                map_destroy(map);
                return 1;
            }
            printf("Выход возможен только на лестнице U.\n");
            continue;
        }
        if (dir < 0) continue;

        int move_result = map_try_move(map, dx, dy);
        if (move_result == 0) continue;

        if (!player_spend_stamina(player, STEP_STAMINA_COST))
        {
            printf("Выносливость на нуле — вы падаете без сил!\n");
            player_take_gold_percent(player, 50);
            map_destroy(map);
            return 0;
        }

        if (player->hp <= 0)
        {
            printf("Вы потеряли сознание! Спасатели доставили вас домой.\n");
            player_take_gold_percent(player, 50);
            map_destroy(map);
            return 0;
        }

        if (move_result == 2)
        {
            if (!player_spend_stamina(player, FLOOR_STAMINA_COST))
            {
                printf("Не хватило сил на переход этажа — вы падаете!\n");
                player_take_gold_percent(player, 50);
                map_destroy(map);
                return 0;
            }

            advance_time(day, hour, 1);
            int reward = GOLD_BASE_MULT * floor_number;
            player_add_gold(player, reward);
            printf("Этаж пройден! Награда: %d золота, время +1 ч.\n", reward);

            if (*hour == NIGHT_HOUR)
            {
                printf("Наступило 02:00 — вы засыпаете прямо на карте!\n");
                player_take_gold_percent(player, 50);
                map_destroy(map);
                return 0;
            }

            floor_number++;
            map_destroy(map);
            map = map_create(floor_number);
            if (!map) return 0;
        }
    }
}

static void menu_adventure(Player *player, int *day, int *hour)
{
    if (!player_exists(player)) return;

    if (player->stamina <= 0)
    {
        printf("Выносливость на нуле — отправляться в поход нельзя. Отдохните.\n");
        return;
    }
    if (*hour == NIGHT_HOUR)
    {
        printf("Сейчас 02:00 ночи — фермер должен спать.\n");
        return;
    }

    printf("\nВы отправляетесь в приключение...\n");
    int result = run_adventure(player, day, hour);
    if (result == 1)
    {
        printf("Вы вернулись на ферму.\n");
    }
    else
    {
        printf("Вы очнулись дома. Приключение окончено.\n");
    }
}

/* main */

static void print_main_menu(void)
{
    printf("\n===== ФЕРМА =====\n");
    printf("0  - Выход\n");
    printf("1  - Создать / пересоздать персонажа\n");
    printf("2  - Карточка персонажа\n");
    printf("3  - Время (часы / ждать / спать)\n");
    printf("4  - Инвентарь\n");
    printf("5  - Положить предмет\n");
    printf("6  - Выбросить предмет из слота\n");
    printf("7  - Любимый ресурс\n");
    printf("8  - Поиск предмета по названию\n");
    printf("9  - Записать состояние в дневник\n");
    printf("10 - Расшифровать старые записи (Энциклопедия)\n");
    printf("11 - Выбросить предмет по ID\n");
    printf("12 - Отправиться в приключение\n");
}

int main(void)
{
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Ферма");
#endif

    srand((unsigned)time(NULL));

    int current_day = 1;
    int current_hour = START_HOUR;
    Player *player = NULL;

    load_items();

    while (1)
    {
        int choice;
        print_main_menu();

        if (!read_number("Выберите действие: ", &choice))
        {
            printf("Ошибка: нужно ввести число.\n");
            continue;
        }

        switch (choice)
        {
        case MENU_EXIT:
            player_destroy(player);
            printf("До свидания!\\n");
            return 0;

        case MENU_CREATE:
            menu_create(&player);
            break;
        case MENU_CARD:
            player_print_card(player, current_day, current_hour);
            break;
        case MENU_TIME:
            menu_time(&current_day, &current_hour, player);
            break;
        case MENU_INVENTORY:
            menu_inventory(player);
            break;
        case MENU_PUT_ITEM:
            menu_put_item(player);
            break;
        case MENU_DROP_SLOT:
            menu_drop_slot(player);
            break;
        case MENU_FAVORITE:
            menu_favorite(player);
            break;
        case MENU_FIND_ITEM:
            menu_find_item(player);
            break;
        case MENU_DIARY:
            menu_diary(player, current_day, current_hour);
            break;
        case MENU_ENCYCLOPEDIA:
            menu_encyclopedia();
            break;
        case MENU_DROP_ID:
            menu_drop_id(player);
            break;
        case MENU_ADVENTURE:
            menu_adventure(player, &current_day, &current_hour);
            break;

        default:
            printf("Ошибка: такого пункта меню нет.\n");
        }
    }
}

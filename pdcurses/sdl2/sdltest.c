//#define SDL_MAIN_HANDLED
//#include <SDL.h>
//#include <curses.h>
//#include <stdlib.h>
//#include <time.h>
//#include <string.h>
//
///* External declarations for PDCurses SDL integration */
//PDCEX SDL_Window* pdc_window;
//PDCEX SDL_Surface* pdc_screen;
//PDCEX int pdc_yoffset;
//
///* Game constants */
//#define MAP_WIDTH 80
//#define MAP_HEIGHT 24
//#define STATUS_HEIGHT 4
//#define MAX_ENEMIES 10
//#define MAX_ITEMS 15
//
///* Entity types */
//#define ENTITY_PLAYER 0
//#define ENTITY_ENEMY 1
//#define ENTITY_ITEM 2
//#define ENTITY_WALL 3
//#define ENTITY_FLOOR 4
//#define ENTITY_STAIRS 5
//
///* Item types */
//#define ITEM_HEALTH 0
//#define ITEM_WEAPON 1
//#define ITEM_ARMOR 2
//
///* Game map */
//typedef struct {
//    int type;
//    char symbol;
//    int color;
//    int walkable;
//} Tile;
//
///* Game entities */
//typedef struct {
//    int x, y;
//    int type;
//    char symbol;
//    int color;
//    int health;
//    int max_health;
//    int attack;
//    int defense;
//    int item_type; /* For items only */
//    int value;     /* For items only */
//    int active;    /* For tracking if entity is alive/valid */
//} Entity;
//
///* Game state */
//typedef struct {
//    Tile map[MAP_HEIGHT][MAP_WIDTH];
//    Entity player;
//    Entity enemies[MAX_ENEMIES];
//    Entity items[MAX_ITEMS];
//    int level;
//    int score;
//    int game_over;
//    char message[80];
//} GameState;
//
///* Function prototypes */
//void init_game(GameState* game);
//void generate_level(GameState* game);
//void render_game(GameState* game);
//void process_input(GameState* game, int input);
//void update_game(GameState* game);
//void enemy_ai(GameState* game, Entity* enemy);
//void attempt_move(GameState* game, Entity* entity, int dx, int dy);
//int check_collision(GameState* game, int x, int y);
//Entity* get_entity_at(GameState* game, int x, int y);
//void combat(GameState* game, Entity* attacker, Entity* defender);
//void pickup_item(GameState* game, Entity* item);
//void show_game_over(GameState* game);
//
//int main(int argc, char** argv)
//{
//    GameState game;
//    int ch;
//
//    /* Initialize random seed */
//    srand(time(NULL));
//
//    /* Initialize SDL */
//    if (SDL_Init(SDL_INIT_VIDEO) < 0)
//        exit(1);
//    atexit(SDL_Quit);
//
//    /* Create window */
//    pdc_window = SDL_CreateWindow("Roguelike - PDCurses for SDL",
//        SDL_WINDOWPOS_UNDEFINED,
//        SDL_WINDOWPOS_UNDEFINED,
//        640, 480, 0);
//    pdc_screen = SDL_GetWindowSurface(pdc_window);
//
//    /* Initialize PDCurses */
//    pdc_yoffset = 0;  /* Use entire window for curses */
//    initscr();
//    start_color();
//    cbreak();
//    noecho();
//    keypad(stdscr, TRUE);
//    curs_set(0);  /* Hide cursor */
//
//    /* Initialize color pairs */
//    init_pair(1, COLOR_WHITE, COLOR_BLACK);     /* Default */
//    init_pair(2, COLOR_YELLOW, COLOR_BLACK);    /* Player */
//    init_pair(3, COLOR_RED, COLOR_BLACK);       /* Enemy */
//    init_pair(4, COLOR_GREEN, COLOR_BLACK);     /* Items */
//    init_pair(5, COLOR_CYAN, COLOR_BLACK);      /* Walls */
//    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);   /* Stairs */
//    init_pair(7, COLOR_WHITE, COLOR_BLUE);      /* Status bar */
//
//    /* Initialize and run game */
//    init_game(&game);
//    generate_level(&game);
//
//    /* Main game loop */
//    while (!game.game_over) {
//        /* Render current game state */
//        render_game(&game);
//
//        /* Get player input */
//        ch = getch();
//
//        /* Process input */
//        process_input(&game, ch);
//
//        /* Update game state */
//        update_game(&game);
//    }
//
//    /* Show game over screen */
//    show_game_over(&game);
//
//    /* Clean up */
//    endwin();
//    return 0;
//}
//
//void init_game(GameState* game)
//{
//    /* Initialize game state */
//    game->level = 1;
//    game->score = 0;
//    game->game_over = 0;
//    strcpy(game->message, "Welcome to Roguelike! Use arrow keys to move.");
//
//    /* Initialize player */
//    game->player.x = MAP_WIDTH / 2;
//    game->player.y = MAP_HEIGHT / 2;
//    game->player.type = ENTITY_PLAYER;
//    game->player.symbol = '@';
//    game->player.color = COLOR_PAIR(2);
//    game->player.health = 100;
//    game->player.max_health = 100;
//    game->player.attack = 10;
//    game->player.defense = 5;
//    game->player.active = 1;
//
//    /* Clear enemies and items */
//    for (int i = 0; i < MAX_ENEMIES; i++) {
//        game->enemies[i].active = 0;
//    }
//
//    for (int i = 0; i < MAX_ITEMS; i++) {
//        game->items[i].active = 0;
//    }
//}
//
//void generate_level(GameState* game)
//{
//    int i, j;
//    int num_enemies = 5 + game->level;
//    int num_items = 3 + game->level / 2;
//
//    if (num_enemies > MAX_ENEMIES) num_enemies = MAX_ENEMIES;
//    if (num_items > MAX_ITEMS) num_items = MAX_ITEMS;
//
//    /* Clear map - fill with floors */
//    for (i = 0; i < MAP_HEIGHT; i++) {
//        for (j = 0; j < MAP_WIDTH; j++) {
//            game->map[i][j].type = ENTITY_FLOOR;
//            game->map[i][j].symbol = '.';
//            game->map[i][j].color = COLOR_PAIR(1);
//            game->map[i][j].walkable = 1;
//        }
//    }
//
//    /* Add walls around the edges */
//    for (i = 0; i < MAP_HEIGHT; i++) {
//        game->map[i][0].type = ENTITY_WALL;
//        game->map[i][0].symbol = '#';
//        game->map[i][0].color = COLOR_PAIR(5);
//        game->map[i][0].walkable = 0;
//
//        game->map[i][MAP_WIDTH - 1].type = ENTITY_WALL;
//        game->map[i][MAP_WIDTH - 1].symbol = '#';
//        game->map[i][MAP_WIDTH - 1].color = COLOR_PAIR(5);
//        game->map[i][MAP_WIDTH - 1].walkable = 0;
//    }
//
//    for (j = 0; j < MAP_WIDTH; j++) {
//        game->map[0][j].type = ENTITY_WALL;
//        game->map[0][j].symbol = '#';
//        game->map[0][j].color = COLOR_PAIR(5);
//        game->map[0][j].walkable = 0;
//
//        game->map[MAP_HEIGHT - 1][j].type = ENTITY_WALL;
//        game->map[MAP_HEIGHT - 1][j].symbol = '#';
//        game->map[MAP_HEIGHT - 1][j].color = COLOR_PAIR(5);
//        game->map[MAP_HEIGHT - 1][j].walkable = 0;
//    }
//
//    /* Add some random walls */
//    for (i = 0; i < MAP_WIDTH * MAP_HEIGHT / 20; i++) {
//        int x = rand() % (MAP_WIDTH - 2) + 1;
//        int y = rand() % (MAP_HEIGHT - 2) + 1;
//
//        /* Don't place a wall where the player is */
//        if (x == game->player.x && y == game->player.y)
//            continue;
//
//        game->map[y][x].type = ENTITY_WALL;
//        game->map[y][x].symbol = '#';
//        game->map[y][x].color = COLOR_PAIR(5);
//        game->map[y][x].walkable = 0;
//    }
//
//    /* Place stairs to next level */
//    while (1) {
//        int x = rand() % (MAP_WIDTH - 2) + 1;
//        int y = rand() % (MAP_HEIGHT - 2) + 1;
//
//        if (game->map[y][x].walkable &&
//            (abs(x - game->player.x) > 10 || abs(y - game->player.y) > 10)) {
//            game->map[y][x].type = ENTITY_STAIRS;
//            game->map[y][x].symbol = '>';
//            game->map[y][x].color = COLOR_PAIR(6);
//            game->map[y][x].walkable = 1;
//            break;
//        }
//    }
//
//    /* Place enemies */
//    for (i = 0; i < num_enemies; i++) {
//        int x, y;
//        int attempts = 0;
//        Entity* enemy = &game->enemies[i];
//
//        /* Find an empty spot */
//        do {
//            x = rand() % (MAP_WIDTH - 2) + 1;
//            y = rand() % (MAP_HEIGHT - 2) + 1;
//            attempts++;
//
//            /* Give up after too many attempts */
//            if (attempts > 100) {
//                enemy->active = 0;
//                break;
//            }
//        } while (!game->map[y][x].walkable ||
//            (abs(x - game->player.x) < 5 && abs(y - game->player.y) < 5) ||
//            check_collision(game, x, y));
//
//        if (attempts <= 100) {
//            enemy->x = x;
//            enemy->y = y;
//            enemy->type = ENTITY_ENEMY;
//            enemy->symbol = (rand() % 2 == 0) ? 'g' : 'o';  /* Goblin or Orc */
//            enemy->color = COLOR_PAIR(3);
//            enemy->health = 20 + game->level * 5;
//            enemy->max_health = enemy->health;
//            enemy->attack = 5 + game->level;
//            enemy->defense = 2 + game->level / 2;
//            enemy->active = 1;
//        }
//    }
//
//    /* Place items */
//    for (i = 0; i < num_items; i++) {
//        int x, y;
//        int attempts = 0;
//        Entity* item = &game->items[i];
//
//        /* Find an empty spot */
//        do {
//            x = rand() % (MAP_WIDTH - 2) + 1;
//            y = rand() % (MAP_HEIGHT - 2) + 1;
//            attempts++;
//
//            /* Give up after too many attempts */
//            if (attempts > 100) {
//                item->active = 0;
//                break;
//            }
//        } while (!game->map[y][x].walkable || check_collision(game, x, y));
//
//        if (attempts <= 100) {
//            item->x = x;
//            item->y = y;
//            item->type = ENTITY_ITEM;
//            item->item_type = rand() % 3;  /* Random item type */
//            item->active = 1;
//
//            switch (item->item_type) {
//            case ITEM_HEALTH:
//                item->symbol = '+';
//                item->value = 20 + rand() % 20;  /* Heal 20-40 */
//                break;
//            case ITEM_WEAPON:
//                item->symbol = '/';
//                item->value = 2 + rand() % 3;  /* +2-4 attack */
//                break;
//            case ITEM_ARMOR:
//                item->symbol = ']';
//                item->value = 1 + rand() % 2;  /* +1-2 defense */
//                break;
//            }
//            item->color = COLOR_PAIR(4);
//        }
//    }
//
//    /* Place player at a clear spot if needed */
//    if (check_collision(game, game->player.x, game->player.y) ||
//        !game->map[game->player.y][game->player.x].walkable) {
//        while (1) {
//            int x = rand() % (MAP_WIDTH - 2) + 1;
//            int y = rand() % (MAP_HEIGHT - 2) + 1;
//
//            if (game->map[y][x].walkable && !check_collision(game, x, y)) {
//                game->player.x = x;
//                game->player.y = y;
//                break;
//            }
//        }
//    }
//}
//
//void render_game(GameState* game)
//{
//    int i, j;
//    char status_bar[MAP_WIDTH];
//
//    /* Clear screen */
//    clear();
//
//    /* Draw map and entities */
//    for (i = 0; i < MAP_HEIGHT; i++) {
//        for (j = 0; j < MAP_WIDTH; j++) {
//            /* Get tile at this position */
//            Tile tile = game->map[i][j];
//
//            /* Check if an entity is here */
//            Entity* entity = get_entity_at(game, j, i);
//
//            if (entity != NULL) {
//                /* Draw entity */
//                attron(entity->color);
//                mvaddch(i, j, entity->symbol);
//                attroff(entity->color);
//            }
//            else {
//                /* Draw tile */
//                attron(tile.color);
//                mvaddch(i, j, tile.symbol);
//                attroff(tile.color);
//            }
//        }
//    }
//
//    /* Draw status bar */
//    attron(COLOR_PAIR(7));
//    for (i = 0; i < STATUS_HEIGHT; i++) {
//        mvprintw(MAP_HEIGHT + i, 0, "%*s", MAP_WIDTH, "");
//    }
//
//    /* Status info */
//    mvprintw(MAP_HEIGHT, 0, " Health: %d/%d | Attack: %d | Defense: %d | Level: %d | Score: %d ",
//        game->player.health, game->player.max_health,
//        game->player.attack, game->player.defense,
//        game->level, game->score);
//
//    /* Show latest message */
//    mvprintw(MAP_HEIGHT + 1, 0, " %s", game->message);
//
//    /* Controls help */
//    mvprintw(MAP_HEIGHT + 2, 0, " Controls: Arrows=Move, q=Quit");
//    attroff(COLOR_PAIR(7));
//
//    /* Update screen */
//    refresh();
//}
//
//void process_input(GameState* game, int input)
//{
//    /* Process player movement/actions */
//    switch (input) {
//    case KEY_UP:
//        attempt_move(game, &game->player, 0, -1);
//        break;
//    case KEY_DOWN:
//        attempt_move(game, &game->player, 0, 1);
//        break;
//    case KEY_LEFT:
//        attempt_move(game, &game->player, -1, 0);
//        break;
//    case KEY_RIGHT:
//        attempt_move(game, &game->player, 1, 0);
//        break;
//    case 'q':
//    case 'Q':
//        game->game_over = 1;
//        strcpy(game->message, "You quit the game.");
//        break;
//    }
//}
//
//void update_game(GameState* game)
//{
//    int i;
//
//    /* Check if player is dead */
//    if (game->player.health <= 0) {
//        game->game_over = 1;
//        strcpy(game->message, "You died! Game over.");
//        return;
//    }
//
//    /* Check if player is on stairs */
//    if (game->map[game->player.y][game->player.x].type == ENTITY_STAIRS) {
//        game->level++;
//        sprintf(game->message, "You descended to level %d!", game->level);
//        generate_level(game);
//        return;
//    }
//
//    /* Update enemies */
//    for (i = 0; i < MAX_ENEMIES; i++) {
//        if (game->enemies[i].active) {
//            enemy_ai(game, &game->enemies[i]);
//        }
//    }
//
//    /* Check for items */
//    Entity* item = NULL;
//    for (i = 0; i < MAX_ITEMS; i++) {
//        if (game->items[i].active &&
//            game->items[i].x == game->player.x &&
//            game->items[i].y == game->player.y) {
//            item = &game->items[i];
//            break;
//        }
//    }
//
//    /* Pick up item if standing on one */
//    if (item != NULL) {
//        pickup_item(game, item);
//    }
//}
//
//void enemy_ai(GameState* game, Entity* enemy)
//{
//    /* Simple AI: Move toward player if nearby */
//    int dx = 0, dy = 0;
//    int distance_x = game->player.x - enemy->x;
//    int distance_y = game->player.y - enemy->y;
//    int abs_x = abs(distance_x);
//    int abs_y = abs(distance_y);
//
//    /* Only move if player is within sight range (8 tiles) */
//    if (abs_x <= 8 && abs_y <= 8) {
//        /* Move in the direction of the player */
//        if (abs_x > abs_y) {
//            dx = (distance_x > 0) ? 1 : -1;
//        }
//        else {
//            dy = (distance_y > 0) ? 1 : -1;
//        }
//
//        /* Attempt to move */
//        attempt_move(game, enemy, dx, dy);
//    }
//    else {
//        /* Random movement if player not in sight */
//        if (rand() % 3 == 0) {  /* 1/3 chance to move */
//            dx = (rand() % 3) - 1;  /* -1, 0, or 1 */
//            dy = (rand() % 3) - 1;  /* -1, 0, or 1 */
//
//            /* Ensure we're not trying to move diagonally */
//            if (dx != 0 && dy != 0) {
//                if (rand() % 2 == 0)
//                    dx = 0;
//                else
//                    dy = 0;
//            }
//
//            attempt_move(game, enemy, dx, dy);
//        }
//    }
//}
//
//void attempt_move(GameState* game, Entity* entity, int dx, int dy)
//{
//    int new_x = entity->x + dx;
//    int new_y = entity->y + dy;
//
//    /* Check if the new position is within bounds and walkable */
//    if (new_x >= 0 && new_x < MAP_WIDTH &&
//        new_y >= 0 && new_y < MAP_HEIGHT &&
//        game->map[new_y][new_x].walkable) {
//
//        /* Check for collisions with entities */
//        Entity* other = get_entity_at(game, new_x, new_y);
//
//        if (other == NULL) {
//            /* No collision, move entity */
//            entity->x = new_x;
//            entity->y = new_y;
//        }
//        else if (entity->type == ENTITY_PLAYER && other->type == ENTITY_ENEMY) {
//            /* Player attacking enemy */
//            combat(game, entity, other);
//        }
//        else if (entity->type == ENTITY_ENEMY && other->type == ENTITY_PLAYER) {
//            /* Enemy attacking player */
//            combat(game, entity, other);
//        }
//    }
//}
//
//int check_collision(GameState* game, int x, int y)
//{
//    int i;
//
//    /* Check if player is at this position */
//    if (game->player.x == x && game->player.y == y)
//        return 1;
//
//    /* Check if any enemy is at this position */
//    for (i = 0; i < MAX_ENEMIES; i++) {
//        if (game->enemies[i].active &&
//            game->enemies[i].x == x &&
//            game->enemies[i].y == y)
//            return 1;
//    }
//
//    /* Check if any item is at this position */
//    for (i = 0; i < MAX_ITEMS; i++) {
//        if (game->items[i].active &&
//            game->items[i].x == x &&
//            game->items[i].y == y)
//            return 1;
//    }
//
//    return 0;
//}
//
//Entity* get_entity_at(GameState* game, int x, int y)
//{
//    int i;
//
//    /* Check if player is at this position */
//    if (game->player.x == x && game->player.y == y)
//        return &game->player;
//
//    /* Check if any enemy is at this position */
//    for (i = 0; i < MAX_ENEMIES; i++) {
//        if (game->enemies[i].active &&
//            game->enemies[i].x == x &&
//            game->enemies[i].y == y)
//            return &game->enemies[i];
//    }
//
//    /* Check if any item is at this position */
//    for (i = 0; i < MAX_ITEMS; i++) {
//        if (game->items[i].active &&
//            game->items[i].x == x &&
//            game->items[i].y == y)
//            return &game->items[i];
//    }
//
//    return NULL;
//}
//
//void combat(GameState* game, Entity* attacker, Entity* defender)
//{
//    int damage;
//
//    /* Calculate damage based on attack and defense */
//    damage = attacker->attack - defender->defense / 2;
//    if (damage < 1) damage = 1;  /* Minimum damage is 1 */
//
//    /* Apply damage */
//    defender->health -= damage;
//
//    /* Generate combat message */
//    if (attacker->type == ENTITY_PLAYER) {
//        sprintf(game->message, "You hit %c for %d damage!",
//            defender->symbol, damage);
//
//        /* Check if enemy died */
//        if (defender->health <= 0) {
//            sprintf(game->message, "You killed %c! +10 score",
//                defender->symbol);
//            defender->active = 0;
//            game->score += 10;
//        }
//    }
//    else {
//        sprintf(game->message, "%c hits you for %d damage!",
//            attacker->symbol, damage);
//    }
//}
//
//void pickup_item(GameState* game, Entity* item)
//{
//    /* Apply item effect */
//    switch (item->item_type) {
//    case ITEM_HEALTH:
//        game->player.health += item->value;
//        if (game->player.health > game->player.max_health)
//            game->player.health = game->player.max_health;
//        sprintf(game->message, "You picked up a healing potion (+%d HP)",
//            item->value);
//        break;
//    case ITEM_WEAPON:
//        game->player.attack += item->value;
//        sprintf(game->message, "You picked up a better weapon (+%d Attack)",
//            item->value);
//        break;
//    case ITEM_ARMOR:
//        game->player.defense += item->value;
//        sprintf(game->message, "You picked up better armor (+%d Defense)",
//            item->value);
//        break;
//    }
//
//    /* Remove item from game */
//    item->active = 0;
//
//    /* Add score */
//    game->score += 5;
//}
//
//void show_game_over(GameState* game)
//{
//    /* Show game over screen */
//    clear();
//    attron(COLOR_PAIR(7));
//
//    mvprintw(MAP_HEIGHT / 2 - 2, MAP_WIDTH / 2 - 5, "GAME OVER");
//    mvprintw(MAP_HEIGHT / 2, MAP_WIDTH / 2 - 12, "Final Score: %d", game->score);
//    mvprintw(MAP_HEIGHT / 2 + 1, MAP_WIDTH / 2 - 12, "Dungeon Level: %d", game->level);
//    mvprintw(MAP_HEIGHT / 2 + 3, MAP_WIDTH / 2 - 15, "Press any key to exit...");
//
//    attroff(COLOR_PAIR(7));
//    refresh();
//
//    /* Wait for any key */
//    getch();
//}
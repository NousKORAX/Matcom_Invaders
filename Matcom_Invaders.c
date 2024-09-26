#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <time.h>

#define USE_COLORS 1 
#define SCREEN_WIDTH 60    // Ancho de la pantalla
#define SCREEN_HEIGHT 30   // Altura de la pantalla
#define MAX_ENEMIES 10     // Número máximo de enemigos
#define SHOTS 5

// Tamaño de la nave del jugador
#define PLAYER_WIDTH 5
#define PLAYER_HEIGHT 2

// Tamaño de las naves enemigas
#define ENEMY_WIDTH 6
#define ENEMY_HEIGHT 2

// Tamaño de los escudos
#define SHIELD_WIDTH 6
#define SHIELD_HEIGHT 2

// Intervalos en microsegundos
#define ENEMY_MOVE_INTERVAL 200000 // 0.5 segundos para el movimiento de enemigos
#define PLAYER_MOVE_INTERVAL 50000  // 0.05 segundos para el movimiento de la nave
#define ENEMY_UPDATE_INTERVAL 2000000 // 2 segundos para la aparición de enemigos
#define MAX_QUEUE_SIZE 100
typedef struct {
    int x;
    int y;
    int alive;
    int health; // Vida del escudo
    char *form[SHIELD_HEIGHT];
} Shield;

typedef struct {
    int x;
    int y;
    int alive;
    char *form[PLAYER_HEIGHT];
} Player;

typedef struct {
    int x;
    int y;
    int alive;
    char *form[ENEMY_HEIGHT];
} Enemy;

typedef struct {
    int x;
    int y;
    int alive;
} Shot;
typedef struct {
    int x;
    int y;
} EnemyRequest;

EnemyRequest enemy_queue[MAX_QUEUE_SIZE];
int queue_front = 0;
int queue_rear = 0;
int queue_size = 0;
Player player;
Enemy enemies[MAX_ENEMIES];
Shot shots[SHOTS];
Shield shields[SCREEN_WIDTH / SHIELD_WIDTH]; // Crear un arreglo de escudos
int num_shields = 0; // Contador de escudos
int score = 0;
int lives = 3;
int game_over = 0;
int move_left = 0; // Flag para movimiento hacia la izquierda
int move_right = 0; // Flag para movimiento hacia la derecha
int fire = 0; // Flag para disparar


void check_terminal_size(int *term_width, int *term_height) {
    getmaxyx(stdscr, *term_height, *term_width);

    while (*term_width < SCREEN_WIDTH || *term_height < SCREEN_HEIGHT) {
        clear();
        mvprintw(*term_height / 2, (*term_width / 2) - 20, "Terminal window is too small!");
        mvprintw(*term_height / 2 + 1, (*term_width / 2) - 15, "Minimum size required: %d x %d", SCREEN_WIDTH, SCREEN_HEIGHT);
        refresh();
        usleep(500000); // Check every 0.5 seconds
        getmaxyx(stdscr, *term_height, *term_width);
    }
    return;
}

void display_start_screen() {
    clear();
    int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
    #ifdef USE_COLORS
        if (has_colors()) attron(COLOR_PAIR(4));
    #endif
    mvprintw(2, (COLS / 2)-50 , "  888b     d888      8888888                                 888                                  ");
    mvprintw(3, (COLS / 2)-50 , "  8888b   d8888        888                                   888            ");
    mvprintw(4, (COLS / 2)-50 , "  88888b.d88888        888                                   888                 ");
    mvprintw(5, (COLS / 2)-50 , "  888Y88888P888        888   88888b.  888  888  8888b.   .d88888  .d88b.  888d888 .d8888b  ");
    mvprintw(6, (COLS / 2)-50 , "  888 Y888P 888        888   888  88b 888  888      88b d88  888 d8P  Y8b 888P    88K   ");
    mvprintw(7, (COLS / 2)-50 , "  888  Y8P  888 888888 888   888  888 Y88  88P .d888888 888  888 88888888 888      Y8888b");
    mvprintw(8, (COLS / 2)-50 , "  888       888        888   888  888  Y8bd8P  888  888 Y88b 888 Y8b.     888          X88");
    mvprintw(9, (COLS / 2)-50 , "  888       888      8888888 888  888   Y88P    Y888888   Y88888   Y8888  888      88888P");
    mvprintw(12, (COLS / 2) - 8," /\\ ");
    mvprintw(13, (COLS / 2) - 8, "/__\\");
    mvprintw(12, (COLS / 2) - 4,"Player");
    mvprintw(15, (COLS / 2) - 8,"(__)" );
    mvprintw(16, (COLS / 2) - 8,"(__)");
    mvprintw(15, (COLS / 2), "Shelters");
    mvprintw(18, (COLS / 2), "Enemies");
    mvprintw(21, (term_width / 2) - 14, "Press SPACE to Start");
    #ifdef USE_COLORS
        if (has_colors()) attron(COLOR_PAIR(3));
    #endif
    mvprintw(18, (COLS / 2) - 8,"_/MMM\\_");
    mvprintw(19, (COLS / 2) - 8,"qWAVAWp" );
    mvprintw(term_height / 2+10, (term_width / 2) - 14, "Ronal Prieto Vazquez C-211");
    mvprintw(term_height / 2+9, (term_width / 2) - 14, "Dayan Cabrera Corvo C-211");

    refresh();
    while (getch() != ' ') {
        // Wait for the player to press SPACE
        usleep(100000); // Check every 0.1 seconds
    }
    clear(); // Clear the start screen
}

void display_end_screen() {
    clear();
    int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
    
    mvprintw(term_height / 2 - 1, (term_width / 2) - 7, "GAME OVER!");
    mvprintw(term_height / 2, (term_width / 2) - 9, "Score: %d", score);
    mvprintw(term_height / 2 + 1, (term_width / 2) - 14, "Press SPACE to Restart");

    refresh();
    
    while (getch() != ' ') {
        // Wait for the player to press SPACE
        usleep(100000); // Check every 0.1 seconds
    }
    clear(); // Clear the end screen
}

void init_game() {
    // Definir la nave del jugador
    player.form[0] = " /\\ ";
    player.form[1] = "/__\\";

    player.x = SCREEN_WIDTH / 2 - (PLAYER_WIDTH / 2); // Ajustar para el centro de la pantalla
    player.y = SCREEN_HEIGHT - PLAYER_HEIGHT - SHIELD_HEIGHT+1; // Posicionar en la parte inferior, debajo del escudo
    player.alive = 1;

    // Inicializar los escudos
    num_shields = SCREEN_WIDTH / SHIELD_WIDTH;
    for (int i = 0; i < num_shields; ++i) {
        shields[i].form[0] = "(__)";
        shields[i].form[1] = "(__)";
        shields[i].x = i * SHIELD_WIDTH;
        shields[i].y = SCREEN_HEIGHT - PLAYER_HEIGHT - SHIELD_HEIGHT - 2; // Encima de la nave
        shields[i].alive = 1;
        shields[i].health = 2; // Asignar una vida a cada escudo
    }

    // Definir las naves enemigas
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        enemies[i].form[0] = "_/MMM\\_";
        enemies[i].form[1] = "qWAVAWp";
        enemies[i].alive = 0;
    }

    for (int i = 0; i < SHOTS; ++i) {
        shots[i].alive = 0;
    }

    score = 0;
    lives = 3;
    game_over = 0;
}

int check_overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    return !(x1 >= x2 + w2 || x1 + w1 <= x2 || y1 >= y2 + h2 || y1 + h1 <= y2);
}
void enqueue_enemy(int x, int y) {
    if (queue_size < MAX_QUEUE_SIZE) {
        enemy_queue[queue_rear].x = x;
        enemy_queue[queue_rear].y = y;
        queue_rear = (queue_rear + 1) % MAX_QUEUE_SIZE;
        queue_size++;
    }
}

EnemyRequest dequeue_enemy() {
    EnemyRequest req = {0, 0};
    if (queue_size > 0) {
        req = enemy_queue[queue_front];
        queue_front = (queue_front + 1) % MAX_QUEUE_SIZE;
        queue_size--;
    }
    return req;
}


int has_dead_enemies(){
    int dead_enemy = 0;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!enemies[i].alive) {
            dead_enemy = 1;
            break;
        }
    }
    return dead_enemy ;
}
void *generate_enemies(void *arg) {
    while (!game_over) {
        int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
        if (term_width < SCREEN_WIDTH || term_height < SCREEN_HEIGHT) {
            check_terminal_size(&term_width, &term_height);
        }
        int x, y;
        int overlap;
        do {
            overlap = 0;
             // Generar enemigos en la parte superior de la pantalla
             x = rand() % (SCREEN_WIDTH - ENEMY_WIDTH);
             y = 0; // Parte superior de la pantalla
            for (int i = 0; i < MAX_ENEMIES; ++i) {
                if (enemies[i].alive && check_overlap(x, y, ENEMY_WIDTH, ENEMY_HEIGHT, enemies[i].x, enemies[i].y, ENEMY_WIDTH, ENEMY_HEIGHT)) {
                    overlap = 1;
                    break;
                }
            }
        } while (overlap);
        enqueue_enemy(x, y);
        int dead_enemy=has_dead_enemies();
        while (queue_size > 0 && dead_enemy==1 ) {
            EnemyRequest req = dequeue_enemy();
            for (int i = 0; i < MAX_ENEMIES; ++i) {
                if (!enemies[i].alive) {
                    enemies[i].x = req.x;
                    enemies[i].y = req.y;
                    enemies[i].alive = 1;
                    break;
                }
            }
            dead_enemy=has_dead_enemies();
        }
        usleep(ENEMY_UPDATE_INTERVAL); // Ajustar la velocidad de aparición de enemigos
    }
    return NULL;
}



void *move_enemies(void *arg) {
    while (!game_over) {
        int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
        if (term_width < SCREEN_WIDTH || term_height < SCREEN_HEIGHT) {
            check_terminal_size(&term_width, &term_height);
        }
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (enemies[i].alive) {
                if (enemies[i].y < SCREEN_HEIGHT - ENEMY_HEIGHT) {
                    enemies[i].y++;
                } else {
                    enemies[i].alive = 0; // Marcar al enemigo como muerto cuando llega al fondo
                    lives -=1;
                    if(lives==0){game_over=1;}
                }
            }
        }
        usleep(ENEMY_MOVE_INTERVAL); // Ajustar la velocidad de movimiento de los enemigos
    }
    return NULL;
}


void *handle_input(void *arg) {
    while (!game_over) {
        int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
        if (term_width < SCREEN_WIDTH || term_height < SCREEN_HEIGHT) {
            check_terminal_size(&term_width, &term_height);
        }
        int ch = getch();
        switch (ch) {
            case 'a': move_left = 1; break;
            case 'd': move_right = 1; break;
            case ' ': fire = 1; break;
            default: break;
        }
        usleep(30000); // Ajustar la velocidad de la lectura de entradas
    }
    return NULL;
}

void *move_player(void *arg) {
    while (!game_over) {
        int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);
        if (term_width < SCREEN_WIDTH || term_height < SCREEN_HEIGHT) {
            check_terminal_size(&term_width, &term_height);
        }
        if (move_left && player.x > 0) player.x -= 2; // Movimiento más rápido para la nave
        if (move_right && player.x < SCREEN_WIDTH - PLAYER_WIDTH) player.x += 2; // Movimiento más rápido para la nave
        move_left = 0; // Resetear el flag de movimiento hacia la izquierda
        move_right = 0; // Resetear el flag de movimiento hacia la derecha
        usleep(PLAYER_MOVE_INTERVAL); // 0.05 segundos para la velocidad de movimiento de la nave
    }
    return NULL;
}

void update_shots() {
    for (int i = 0; i < SHOTS; ++i) {
        if (shots[i].alive) {
            shots[i].y--;
            if (shots[i].y < 0) {
                shots[i].alive = 0;
            }
        }
    }
}

void check_collisions() {
    // Comprobar colisiones entre disparos y enemigos
    for (int i = 0; i < SHOTS; ++i) {
        if (shots[i].alive) {
            for (int j = 0; j < MAX_ENEMIES; ++j) {
                if (enemies[j].alive && check_overlap(shots[i].x, shots[i].y, 1, 1, enemies[j].x, enemies[j].y, ENEMY_WIDTH, ENEMY_HEIGHT)) {
                    shots[i].alive = 0;
                    enemies[j].alive = 0;
                    score += 10;
                }
            }
        }
    }

    // Comprobar colisiones entre el jugador y los enemigos
    if (player.alive) {
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (enemies[i].alive && check_overlap(player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT, enemies[i].x, enemies[i].y, ENEMY_WIDTH, ENEMY_HEIGHT)) {
                lives-=1;
                if(lives==0){game_over=1;}
            }
        }
    }

    // Comprobar colisiones entre disparos y escudos
    /*for (int i = 0; i < SHOTS; ++i) {
        if (shots[i].alive) {
            for (int j = 0; j < num_shields; ++j) {
                if (shields[j].alive && check_overlap(shots[i].x, shots[i].y, 1, 1, shields[j].x, shields[j].y, SHIELD_WIDTH, SHIELD_HEIGHT)) {
                    // Los disparos del jugador no afectan a los escudos
                    shots[i].alive = 0;
                }
            }
        }
    }*/

    // Comprobar colisiones entre enemigos y escudos
    for (int i = 0; i < num_shields; ++i) {
        if (shields[i].alive) {
            for (int j = 0; j < MAX_ENEMIES; ++j) {
                if (enemies[j].alive && check_overlap(enemies[j].x, enemies[j].y, ENEMY_WIDTH, ENEMY_HEIGHT, shields[i].x, shields[i].y, SHIELD_WIDTH, SHIELD_HEIGHT)) {
                    enemies[j].alive = 0;
                    shields[i].alive = 0; // El escudo se rompe
                }
            }
        }
    }
}

void draw() {
    clear();
    int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);

    if (game_over) {
        display_end_screen();
    } else {
        // Dibujar escudos
        for (int i = 0; i < num_shields; ++i) {
            if (shields[i].alive) {
                #ifdef USE_COLORS
                    if (has_colors()) attron(COLOR_PAIR(4));
                #endif
                for (int j = 0; j < SHIELD_HEIGHT; ++j) {
                    mvprintw(shields[i].y + j, shields[i].x, "%s",shields[i].form[j]);
                }
            }
        }

        // Dibujar la nave del jugador
        for (int i = 0; i < PLAYER_HEIGHT; ++i) {
            #ifdef USE_COLORS
                if (has_colors()) attron(COLOR_PAIR(4));
            #endif
            mvprintw(player.y + i, player.x, "%s",player.form[i]);
        }

        // Dibujar los disparos
        for (int i = 0; i < SHOTS; ++i) {
            if (shots[i].alive) {
                #ifdef USE_COLORS
                    if (has_colors()) attron(COLOR_PAIR(4));
                #endif
                mvprintw(shots[i].y, shots[i].x, "^");
            }
        }

        // Dibujar las naves enemigas
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (enemies[i].alive) {
                #ifdef USE_COLORS
                    if (has_colors()) attron(COLOR_PAIR(3));
                #endif
                for (int j = 0; j < ENEMY_HEIGHT; ++j) {
                    mvprintw(enemies[i].y + j, enemies[i].x, "%s",enemies[i].form[j]);
                }
            }
        }
        #ifdef USE_COLORS
            if (has_colors()) attron(COLOR_PAIR(2));
        #endif
        mvprintw(0, 0, "Score: %d  Lives: %d", score, lives);
        refresh();
    }
}

int main() {
    srand(time(NULL));
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);
    keypad(stdscr, TRUE);

    int term_width, term_height;
    getmaxyx(stdscr, term_height, term_width);

    check_terminal_size(&term_width, &term_height); // Verificar tamaño de terminal
    #ifdef USE_COLORS
    if (has_colors()) {
        start_color();
        init_pair(0, COLOR_BLACK, COLOR_BLACK);
        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_RED, COLOR_BLACK);
        init_pair(3, COLOR_CYAN, COLOR_BLACK);
        init_pair(4, COLOR_WHITE, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_BLUE, COLOR_BLACK);
        init_pair(7, COLOR_YELLOW, COLOR_BLACK);
    }
    #endif
    display_start_screen(); // Pantalla de inicio

    while (1) {
        init_game();

        pthread_t enemy_gen_thread;
        pthread_create(&enemy_gen_thread, NULL, generate_enemies, NULL);

        pthread_t enemy_move_thread;
        pthread_create(&enemy_move_thread, NULL, move_enemies, NULL);

        pthread_t input_thread;
        pthread_create(&input_thread, NULL, handle_input, NULL);

        pthread_t player_move_thread;
        pthread_create(&player_move_thread, NULL, move_player, NULL);

        while (!game_over) {

            check_terminal_size(&term_width, &term_height);
            if (fire) {
                fire = 0; // Resetear el flag de disparo
                for (int i = 0; i < SHOTS; ++i) {
                    if (!shots[i].alive) {
                        shots[i].x = player.x + PLAYER_WIDTH / 2;
                        shots[i].y = player.y - 1;
                        shots[i].alive = 1;
                        break;
                    }
                }
            }

            update_shots();
            check_collisions();
            draw();
            usleep(PLAYER_MOVE_INTERVAL); // 0.05 segundos para el movimiento de la nave
        }

        // Detener los hilos
        pthread_cancel(input_thread);
        pthread_cancel(player_move_thread);
        pthread_join(enemy_gen_thread, NULL);
        pthread_join(enemy_move_thread, NULL);

        // Esperar antes de reiniciar el juego
        display_end_screen(); // Pantalla de fin y espera por SPACE
    }

    endwin();
    return 0;
}


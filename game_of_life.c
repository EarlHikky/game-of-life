#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>

#define MOVING 1
#define TRAIN 2
#define FILE_DATA 3

void init_game(int **field, int rows, int cols);
int tick_game(int **field, int rows, int cols, int *speed, MEVENT *event);
void set_init_state(int **field, int rows, int cols);
void field_init(int **field, int rows, int cols, int choice);
void output(int **field, int rows, int cols);
int neighbor_count(int **field, int rows, int cols, int ceil_row, int ceil_column);
int refresh_field(int **field, int rows, int cols);
void show_menu();
int load_field_from_file(int **field, int rows, int cols);
void print_welcome_message();
void print_game_over();

int main() {
    struct winsize w;
    
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int rows = w.ws_row;
    int cols = w.ws_col;
    
    int **field = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        field[i] = (int *)malloc(cols * sizeof(int));
    }

    int gameover = 0;
    int speed = 200;
    MEVENT event;

    print_welcome_message();
    printf("\033[H\033[J");

    init_game(field, rows, cols);

    while (!gameover) {
        gameover = tick_game(field, rows, cols, &speed, &event);
    }

    endwin();
    print_game_over();

    for (int i = 0; i < rows; i++) {
        free(field[i]);
    }
    free(field);

    return 0;
}

void print_welcome_message() {
    printf("\n");
    printf("  _____                 ____  ___  __   _ ___  \n");
    printf(" / ___/__ ___ _  ___   / __ \\/ _/ / /  (_) _/__ \n");
    printf("/ (_ / _ `/  ' \\/ -_) / /_/ / _/ / /__/ / _/ -_)\n");
    printf("\\___/\\_,_/_/_/_/\\__/  \\____/_/  /____/_/_/ \\__/ \n");
    printf("\n");
    printf("Welcome to the Game of Life!\n");
    napms(3000);
    printf("\n");
}

void print_game_over() {
    printf("\n");
    printf("   _____                         ____                 \n");
    printf("  / ____|                       / __ \\                \n");
    printf(" | |  __  __ _ _ __ ___   ___  | |  | |_   _____ _ __ \n");
    printf(" | | |_ |/ _` | '_ ` _ \\ / _ \\ | |  | \\ \\ / / _ \\ '__|\n");
    printf(" | |__| | (_| | | | | | |  __/ | |__| |\\ V /  __/ |   \n");
    printf("  \\_____|\\__,_|_| |_| |_|\\___|  \\____/  \\_/ \\___|_|   \n");
    printf("\n");
    printf("\n");
}

void init_game(int **field, int rows, int cols) {
    initscr();
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_BLACK, COLOR_BLACK);
    curs_set(0);
    set_init_state(field, rows, cols);
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);
    
}

int tick_game(int **field, int rows, int cols, int *speed, MEVENT *event) {
    int ch;
    int flag = 0;
    napms(*speed);
    clear();
    flag = refresh_field(field, rows, cols);
    refresh();
    ch = getch();
    switch (ch) {
        case 'q':
            flag = 1;
            break;
        case 'f':
            if (*speed > 50) *speed -= 50;
            break;
        case 's':
            if (*speed < 1000) *speed += 50;
            break;
        case KEY_MOUSE:
            if (getmouse(event) == OK) {
                if (event->bstate & BUTTON1_CLICKED) {
                    int x = event->x % cols;
                    int y = event->y % rows;
                    field[y][x] = 1;
                    output(field, rows, cols);
                    refresh();
                    break;
                }
            }
        default:
            break;
    }
    return flag;
}

void set_init_state(int **field, int rows, int cols) {
    int choice = 4;
    if (!isatty(fileno(stdin))) {
        field_init(field, rows, cols, FILE_DATA);
        load_field_from_file(field, rows, cols);
    } else {
        show_menu();
        scanw("%d", &choice);
        field_init(field, rows, cols, choice);
    }
}

void show_menu() {
    printw("Choose init state:\n");
    printw("1. Moving\n");
    printw("2. Train\n");
}

void field_init(int **field, int rows, int cols, int choice) {
    srand(time(NULL));
    int cells;
    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < cols; column++) {
            field[row][column] = 0;
        }
    }

    switch (choice) {
        case MOVING:
            field[1][2] = 1;
            field[2][3] = 1;
            field[3][1] = 1;
            field[3][2] = 1;
            field[3][3] = 1;
            break;
        case TRAIN:
            field[1][2] = 1;
            field[1][5] = 1;
            field[2][6] = 1;
            field[3][6] = 1;
            field[4][6] = 1;
            field[4][5] = 1;
            field[4][4] = 1;
            field[4][3] = 1;
            break;
        case FILE_DATA:
            break;
        default:
            cells = rand() % 100 + 250;
            for (int i = 0; i < cells; i++) {
                int row = rand() % rows;
                int column = rand() % cols;
                field[row][column] = 1;
            }
            break;
    }
}

void output(int **field, int rows, int cols) {
    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < cols; column++) {
            move(row, column);

            if (field[row][column]) {
                attron(COLOR_PAIR(1));
                printw("0");
                attroff(COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(2));
                printw(".");
                attroff(COLOR_PAIR(2));
            }

            if (column < cols - 1) {
                printw(" ");
            }
        }
        printw("\n");
    }
}

int neighbor_count(int **field, int rows, int cols, int ceil_row, int ceil_column) {
    int count = 0;
    for (int row = -1; row <= 1; row++) {
        for (int column = -1; column <= 1; column++) {
            if (!(row == 0 && column == 0)) {
                int neighbor_row = (ceil_row + row + rows) % rows;
                int neighbor_column = (ceil_column + column + cols) % cols;
                if (field[neighbor_row][neighbor_column]) count++;
            }
        }
    }
    return count;
}

int refresh_field(int **field, int rows, int cols) {
    int **new_field = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++) {
        new_field[i] = (int *)malloc(cols * sizeof(int));
    }

    int counter = 0;
    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < cols; column++) {
            int neighbors = neighbor_count(field, rows, cols, row, column);
            if (neighbors > 0) counter++;

            if (field[row][column]) {
                if (neighbors == 2 || neighbors == 3) {
                    new_field[row][column] = 1;
                } else {
                    new_field[row][column] = 0;
                }
            } else {
                if (neighbors == 3) {
                    new_field[row][column] = 1;
                } else {
                    new_field[row][column] = 0;
                }
            }
        }
    }

    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < cols; column++) {
            field[row][column] = new_field[row][column];
        }
    }

    for (int i = 0; i < rows; i++) {
        free(new_field[i]);
    }
    free(new_field);

    output(field, rows, cols);
    return counter ? 0 : 1;
}

int load_field_from_file(int **field, int rows, int cols) {
    char line[cols + 2];
    for (int row = 0; row < rows && fgets(line, sizeof(line), stdin); row++) {
        for (int column = 0; column < cols && line[column] != '\n'; column++) {
            if (line[column] == '0') {
                field[row][column] = 1;
            } else {
                field[row][column] = 0;
            }
        }
    }
    fclose(stdin);
    freopen("/dev/tty", "r", stdin);
    return 1;
}

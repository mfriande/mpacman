#include <stdio.h>
#include <stdlib.h>
// install library: sudo apt-get install libncurses5-dev libncursesw5-dev
// compile with: gcc -g game_board_design.c -o game_board -lncurses
#include <ncurses.h> 
#include <unistd.h> // Include the header for usleep
#include <pthread.h>
                    
#define ROWS 36
#define COLS 28
#define NUM_GHOSTS 4
#define NUM_PELLETS 240
#define NUM_ENERGIZER_PELLETS 4
#define BONUS_TIME 10
// ###### 3rd step - Define Game Entities

// Enum to represent Pacman's direction
enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
};

enum Ghost_Type {
    BLINKY, // RED GHOST
    PINKY,  // PINK GHOST
    INKY,   // BLUE GHOST 
    CLYDE   // ORANGE GHOST  
};

enum Ghost_Mode {
    FRIGHTENED, 
    SCATTER,
    CHASE,
    EATEN
};

enum Speed {
    VERY_SLOW,
    SLOW,
    NORMAL,
    FAST,
    VERY_FAST
};

enum Tile_Type {
    EMPTY,              // V
    WALL,               // #
    PELLET,             // .
    ENERGYZER_PELLET,   // *
    TUNNEL,             // T
    GATE,               // G
};

enum Bonus_Type {
    FRUIT,
    KEY,
    BELL,
    GALAXIAN,
};

struct Tile_Coordinates {
    int x, y;
};

struct Tunnel {
    struct Tile_Coordinates left_entrance;
    struct Tile_Coordinates left_exit;
    struct Tile_Coordinates right_entrance;
    struct Tile_Coordinates right_exit;
};

struct Tunnel tunnel = {
    .left_entrance = {17, 5},
    .left_exit = {17, 0},
    .right_entrance = {17, 22},
    .right_exit = {17,27}
};

struct Tile {
    enum Tile_Type type;
};

// Example structure for Pacman
struct Pacman {
    enum Direction direction;           // Pacman current direction
    enum Speed speed;
    int lives;                          // Number of lives
    int score;                          // Player's score
    struct Tile_Coordinates current;    // current position
    struct Tile_Coordinates next;       // next position 
};


// Example structure for Ghost
struct Ghost {
    enum Direction direction;           // Ghost current direction
    enum Speed speed;
    struct Tile_Coordinates target;     // target position
    struct Tile_Coordinates current;    // current position
    struct Tile_Coordinates next;       // next position
    enum Ghost_Type type;
    enum Ghost_Mode mode;
    // Add more properties as needed
};

// Example structure for Pellet
struct Pellet {
    struct Tile_Coordinates position;   // pellet position on board
    int is_energyzer;                   // tracks if current pellet is an energizer one
    int eaten;                          // flag indicates if the pellet was already ate by pacman
};

// Example structure for Fruit and Other Bonus Items
struct Bonus_Item {
    struct Tile_Coordinates position;
    enum Bonus_Type type;
    char symbol;
    int points;
    int eaten;
};

struct Maze {
    // define the maze layout, walls, etc
};

// 2D board array contains the index to the pellet array to be O(1) complexity search
struct Board_Element {
    char element;
    int index_to_pellet_array;
};

// To track in which level Pacman is playing
int level = 1;

// Display Bonus Item when pellet eaten number is reached array
int pellets_to_display_bonus[2] = {70, 170};

// Global variable to track when its time to remove bonus from game board
int remove_bonus = 0;

// Global variable to track if bonus was displayed already
int bonus_displayed = 0;

// Global variable mutex
pthread_mutex_t remove_bonus_mutex = PTHREAD_MUTEX_INITIALIZER;

// ###### End of 3rd step

// ###### 2nd step - Designing the Game Board

// Function to initialize the game board from file
void initialize_board_from_file(struct Board_Element board[ROWS][COLS], const char* filename) {
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    int index_pellet_array = 0;
    for (int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {

            // fscanf(FILE *stream, const char *format, ...)
            fscanf(file, " %c", &board[i][j].element); // add space to format to consume extra characters(new line , spaces, Tabs etc)

            // Check if the element is a pellet and assign its index int the pellet array
            if(board[i][j].element == '.' || board[i][j].element == '*') {
                board[i][j].index_to_pellet_array = index_pellet_array++;
            } else {
                board[i][j].index_to_pellet_array = -1; // not a pellet
            }
        }
    }
    fclose(file);
}

// Function to print the game board to stdout
void print_board(struct Board_Element board[ROWS][COLS]) {
    clear(); // Clear the screen before displaying the updated board
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            switch(board[i][j].element) {
                case 'V':
                case 'T':
                case '!':
                    printw("  ");
                    break;
                default:
                    printw("%c ", board[i][j].element);
                    break;
            } 
        }
        printw("\n");
    }
    refresh(); // Refresh the screen to show the updated content
}

// Function to print the game board to a file
void print_board_to_file(struct Board_Element board[ROWS][COLS], const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            fprintf(file, "%c", board[i][j].element);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

void print_score(int score) {
    mvprintw(1, 2, "SCORE: %d", score);
}

void print_lives(int lives) {
    mvprintw(35, 2, "LIVES: %d", lives);
}

void print_fruit(char fruit) {
    mvprintw(35, 45, "FRUIT: %c", fruit);
}

void print_current_level(int level) {
    mvprintw(1, 45, "LEVEL: %d", level);
}

void print_ready_banner() {
    mvprintw(20, 22, "R E A D Y !");
}

void hide_cursor() {
    curs_set(0);
    noecho();
}

/*int check_next_level(struct Board_Element board[ROWS][COLS], const char* filename, 
                        int total_pellets, struct Pacman* pacman, int pellets_eaten) {
    int next_level = 0;
    if(pellets_eaten == total_pellets) {
        initialize_board_from_file(board, filename);
        pacman->current.x = 26;
        pacman->current.y = 13;
        pacman->direction = NONE;
        next_level = 1;
    }
    return next_level;
}*/

int next_level(int total_pellets, int pellets_eaten) {
    return (pellets_eaten == total_pellets);
}

void initialize_level(struct Board_Element board[ROWS][COLS], const char* filename,
                        struct Pacman* pacman, int* pellets_eaten) {
    initialize_board_from_file(board, filename);
    pacman->current.x = 26;
    pacman->current.y = 13;
    pacman->direction = NONE;
    level++;
    *pellets_eaten = 0;
}

// Function to build an array of pellets from the 2D game board
int build_pellet_array(struct Board_Element board[ROWS][COLS], struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS]) {
    int pellet_count = 0; // Counter for the number of pellets found

    // Iterate over each cell of the game board
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            // check if the cell contains a pellet or energized pellet
            if(board[i][j].element == '.' || board[i][j].element == '*') {
                // Add pellet to the pellet array
                pellets[pellet_count].position.x = i;
                pellets[pellet_count].position.y = j;
                pellets[pellet_count].eaten = 0;        // initialize pellets (at the beginning every pellet state not ate)
                // Set is_energizer based on the cell content
                pellets[pellet_count].is_energyzer = (board[i][j].element == '*');

                // Increment the pellet count
                pellet_count++;

                // Check if we've reached the maximum number of pellets
                if(pellet_count >= (NUM_PELLETS + NUM_ENERGIZER_PELLETS)) {
                    // Maximum number of pellets reached, exit the loop
                    break;
                }
            }
        }
    }
    // Return the number of pellets found
    return pellet_count;
}

// ###### End of 2nd step

// ###### Game Logic 

// ###### 4th step - Handle user input and move pacman

// Function to handle user input and update Pacman's direction
enum Direction handle_input(char input, enum Direction current, struct Board_Element board[ROWS][COLS], struct Pacman* pacman) {
    switch (input) {
        case 'w':
            if(board[pacman->current.x - 1][pacman->current.y].element != '#') {
                return UP;
            }
            break;
        case 's':
            if(board[pacman->current.x + 1][pacman->current.y].element != '#') {
                return DOWN;
            }
            break;
        case 'a':
            if(board[pacman->current.x][pacman->current.y - 1].element != '#') {
                return LEFT;
            }
            break;
        case 'd':
            if(board[pacman->current.x][pacman->current.y + 1].element != '#') {
                return RIGHT;
            }
            break;
        // add more cases for other keys if needed
        default:
            break; // no change in direction if an invalid key is pressed
    }
    return current;
}

char get_input() {
    return getch();
}

int is_valid_move(struct Tile_Coordinates target) {
    return (target.x >= 0 && target.x < ROWS && target.y >= 0 && target.y < COLS);
}

void update_pacman_speed(enum Speed speed) {
    int move_delay = 0;
    switch(speed) {
        case VERY_SLOW: 
            move_delay = 1000000;
            usleep(move_delay);
            break;
        case SLOW:
            move_delay = 500000;
            usleep(move_delay);
            break;
        case NORMAL:
            move_delay = 250000;
            usleep(move_delay);
            break;
        case FAST:
            move_delay = 200000;
            usleep(move_delay);
            break;
        case VERY_FAST:
            move_delay = 100000;
            usleep(move_delay);
            break;
        default:
            move_delay = 250000;
            usleep(move_delay);
            break;
    }
}

void check_wall_or_gate_collision(struct Board_Element board[ROWS][COLS], struct Pacman* pacman) {
    // get Pacman's next position
    int next_x = pacman->next.x;
    int next_y = pacman->next.y;

    // Check if Pacman's next position collides with a wall
    if(board[next_x][next_y].element == '#' || board[next_x][next_y].element == 'G') {
        // Pacman collided with a wall, reset his next position to his current position
            pacman->next.x = pacman->current.x;
            pacman->next.y = pacman->current.y;
    }
}

void check_pellet_collision(struct Board_Element board[ROWS][COLS], struct Pacman* pacman, 
                            struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS], int* pellets_eaten) {
    // Get Pacman's next position
    int next_x = pacman->next.x;
    int next_y = pacman->next.y;
    // Check if Pacman's next position collides with a pellet
    if(board[next_x][next_y].element == '.' || board[next_x][next_y].element == '*') {
        // Pacman collided with a pellet
        
        // update Pacman's score based on the pellet type
        if(pellets[board[next_x][next_y].index_to_pellet_array].is_energyzer) {
            // Energyzer pellet
            pacman->score += 50; // Increase Pacman's score
        } else {
            // Regular pellet
            pacman->score += 10; // Increase Pacman's score
        }
        // Mark the pellet as eaten in pellet's array (not used at the moment but can be helpfull in future)
        pellets[board[next_x][next_y].index_to_pellet_array].eaten = 1;
        (*pellets_eaten)++;
    } 
}

void check_bonus_collision(struct Board_Element board[ROWS][COLS], struct Bonus_Item* bonus, struct Pacman* pacman) {
    // Get Pacman's next position
    int next_x = pacman->next.x;
    int next_y = pacman->next.y;
    if(board[next_x][next_y].element == bonus->symbol) {
        pacman->score += bonus->points;
        bonus->eaten++;

        pthread_mutex_lock(&remove_bonus_mutex);
        remove_bonus = 1;
        pthread_mutex_unlock(&remove_bonus_mutex);
    }
}

int check_tunnel_collision(struct Pacman* pacman) {
    int collision = 0;
    // Check if Pacman's current position is within the left tunnel
    if((pacman->current.x == tunnel.left_entrance.x && 
        pacman->current.y >= tunnel.left_exit.y &&
        pacman->current.y <= tunnel.left_entrance.y)){
        // Check if Pacman is at exit of the left tunnel
        if(pacman->next.y == tunnel.left_exit.y) {
            // Set Pacman's next position to the exit of the right tunnel
            pacman->next.x = tunnel.right_exit.x;
            pacman->next.y = tunnel.right_exit.y;
           
            pacman->direction = LEFT;
            collision = 1;
        }    
        pacman->speed = FAST;
    
    } else if ((pacman->current.x == tunnel.right_entrance.x &&
            pacman->current.y <= tunnel.right_exit.y &&
            pacman->current.y >= tunnel.right_entrance.y)) {
            // Check if Pacman is at exit of the right tunnel
            if(pacman->next.y == tunnel.right_exit.y) {
                // Set Pacman's next position to the exit of the left tunnel
                pacman->next.x = tunnel.left_exit.x;
                pacman->next.y = tunnel.left_exit.y;
            
                pacman->direction = RIGHT;
                collision = 1;
            }
            
            pacman->speed = FAST;

    } else {
        pacman->speed = NORMAL;
    }
    return collision;
}

void move_pacman(struct Board_Element board[ROWS][COLS], struct Pacman* pacman, 
                    struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS], int* pellets_eaten,
                    struct Bonus_Item* bonus) {

    pacman->next.x = pacman->current.x;
    pacman->next.y = pacman->current.y;
    update_pacman_speed(pacman->speed);
    switch (pacman->direction) {
        case UP: // Move up
            pacman->next.x--;
            break;
        case DOWN: // Move down
            pacman->next.x++;
            break;
        case LEFT: // Move left
            pacman->next.y--;
            break;
        case RIGHT: // Move right
            pacman->next.y++;
            break;
        default:
            break;
        // Add more cases for other keys if needed
    }
    if(is_valid_move(pacman->next)) {
        check_wall_or_gate_collision(board, pacman);
        check_pellet_collision(board, pacman, pellets, pellets_eaten);
        check_bonus_collision(board, bonus, pacman);
        if(!check_tunnel_collision(pacman)) { // if not collision update pacman normally
            // Clear the current position on the board
            board[pacman->current.x][pacman->current.y].element = 'V';
            // Update Pacman's current position
            pacman->current.x = pacman->next.x;
            pacman->current.y = pacman->next.y;
            // Place Pacman in the new position on the board
            board[pacman->next.x][pacman->next.y].element = 'O';
        } else { // if collision update pacman differently 
            // Clear the current position on the board
            board[pacman->current.x][pacman->current.y].element = 'V';
            // Place pacman at tunnel exit
            if(pacman->direction == LEFT) {
                board[pacman->current.x][pacman->current.y - 1].element = 'O';
            } else {
                board[pacman->current.x][pacman->current.y + 1].element = 'O';
            }
            // to make pacman traverse from left end tile of the tunnel to the right end on opposite side or vice versa cleaner
            print_board(board);
            print_score(pacman->score);
            print_lives(pacman->lives);
            print_current_level(level);
            print_fruit(bonus->symbol);
            refresh();
            usleep(200000);
            // Clear Pacman's current position
            if(pacman->direction == LEFT) {
                board[pacman->current.x][pacman->current.y - 1].element = 'V';
            } else {
                board[pacman->current.x][pacman->current.y + 1].element = 'V';
            }
            
            // Update Pacman's current position
            pacman->current.x = pacman->next.x;
            pacman->current.y = pacman->next.y;
            // Place Pacman in the new position on the board
            board[pacman->next.x][pacman->next.y].element = 'O';
            print_board(board);
            refresh();
        }
        
    }
}

struct Bonus_Item get_bonus_item(int level) {
    struct Bonus_Item bonus = {.position = {20, 13}, .eaten = 0};
    switch(level) {
        case 1:
            bonus.type = FRUIT;
            bonus.symbol = 'F';
            bonus.points = 100;
            break;
        case 2:
            bonus.type = FRUIT;
            bonus.symbol = 'F';
            bonus.points = 300;
            break;
        case 3:
            bonus.type = FRUIT;
            bonus.symbol = 'F';
            bonus.points = 500;
            break;
        case 4:
            bonus.type = FRUIT;
            bonus.symbol = 'F';
            bonus.points = 700;
            break;
        case 5:
            bonus.type = FRUIT;
            bonus.symbol = 'F';
            bonus.points = 1000;
            break;
        case 6:
            bonus.type = GALAXIAN;
            bonus.symbol = 'W';
            bonus.points = 2000;
            break;
        case 7:
            bonus.type = BELL;
            bonus.symbol = 'A';
            bonus.points = 3000;
            break;
        case 8:
            bonus.type = KEY;
            bonus.symbol = 'K';
            bonus.points = 5000;
            break;
        default:
            bonus.type = KEY;
            bonus.symbol = 'K';
            bonus.points = 5000;
            break;
    }
    return bonus;
}

int bonus_reached(int* pellets_eaten, int* bonus_displayed) {
    int result = 0;
    int length = sizeof(pellets_to_display_bonus) / sizeof(pellets_to_display_bonus[0]);
    for(int i = 0; i < length; i++) {
        if(*pellets_eaten == pellets_to_display_bonus[i] && !(*bonus_displayed)){
            result = 1;
            
            pthread_mutex_lock(&remove_bonus_mutex);
            *bonus_displayed = 1;
            pthread_mutex_unlock(&remove_bonus_mutex);
            break;
        }
    }
    return result;
}

void* remove_bonus_thread(void* arg) {
    int bonus_time = *((int*)arg);
    free(arg);

    sleep(bonus_time);

    pthread_mutex_lock(&remove_bonus_mutex);
    remove_bonus = 1;
    bonus_displayed = 0;
    pthread_mutex_unlock(&remove_bonus_mutex);

    pthread_exit(NULL);
}

void set_bonus_timer(int bonus_time) {
    // Ensure that thread function operates on a copy of the bonus_time preventing 
    // changes to the original variable in main thread
    int* bonus_time_ptr = (int*)malloc(sizeof(int)); 
    if(bonus_time_ptr == NULL) {
        return;
    }
    *bonus_time_ptr = bonus_time;
    pthread_t bonus_thread;
    pthread_create(&bonus_thread, NULL, remove_bonus_thread, (void*)bonus_time_ptr);
}

void chase_behavior(enum Ghost_Type type) {
    // Implement chase behavior based on ghost type
    switch (type) {
        case BLINKY:
            // Blinky's chase behavior
            //todo();
            break;
        case PINKY:
            //todo();
            break;
        case INKY:
            //todo();
            break;
        case CLYDE:
            //todo();
            break;
    }
}

void scatter_behavior(enum Ghost_Type type) {

}

void frightened_behavior(enum Ghost_Type type) {

}

void move_ghost(enum Ghost_Type type, enum Ghost_Mode mode) {
    switch (mode) {
        case CHASE: // CHASE MODE
            chase_behavior(type);
            break;
        case SCATTER: // SCATTER MODE
            scatter_behavior(type);
            break;
        case FRIGHTENED: // FRIGHTENED MODE
            frightened_behavior(type);
            break; 
    }
}

// ###### End of 4th step

// ###### 5th step - Update Game State 
/*
void update_ghost_behavior(struct Ghost *ghost, struct Pacman *pacman, char board[ROWS][COLS]) {
    // Store the last position of the ghost
    int lastX = ghost->x;
    int lastY = ghost->y;
    // calculate the direction to move towards Pacman
    int x_diff = pacman->x - ghost->x;
    int y_diff = pacman->y - ghost->y;
    // Determine the direction
    int newX = ghost->x;
    int newY = ghost->y;

    if(abs(x_diff) > abs(y_diff)) {
        // move in the x-direction
        newX += (x_diff > 0) ? 1 : -1;
    } else {
        // move in the y-direction
        newY += (y_diff > 0) ? 1 : -1;
    }

    if(is_valid_move(board, newX, newY)) {
        // Clear the current position on the board
        board[lastX][lastY] = '.';
        // Update the ghost's position
        ghost->x = newX;
        ghost->y = newY;
        // Place ghost in the new position on the board
        board[ghost->x][ghost->y] = 'G';
    } else {
        board[ghost->x][ghost->y] = 'G';
    }
}
*/

void log_met_bonus(int value) {
       FILE *file = fopen("inside_thread_log.txt", "a"); // Open the log file in append mode
    if (file != NULL) {
        fprintf(file, "pellets_eaten: %d\n", value);
        fclose(file);
    } else {
        printf("Error opening log file!\n");
    }
}

void update_game_state(struct Board_Element board[ROWS][COLS], struct Pacman* pacman, 
                        struct Ghost ghosts[NUM_GHOSTS], struct Pellet pellets[NUM_PELLETS], 
                        int total_of_pellets, int* pellets_eaten, struct Bonus_Item* bonus_ptr) {


    move_pacman(board, pacman, pellets, pellets_eaten, bonus_ptr);
    if(next_level(total_of_pellets, *pellets_eaten)) {
            initialize_level(board, "maze.txt", pacman, pellets_eaten);
            struct Bonus_Item bonus = get_bonus_item(level);
            bonus_ptr->position = bonus.position;
            bonus_ptr->type = bonus.type;
            bonus_ptr->symbol = bonus.symbol;
            bonus_ptr->points = bonus.points;
            bonus_ptr->eaten = bonus.eaten;
            bonus_displayed = 0;
    }

    if(bonus_reached(pellets_eaten, &bonus_displayed)) {
        log_met_bonus(*pellets_eaten);
        board[bonus_ptr->position.x][bonus_ptr->position.y].element = bonus_ptr->symbol;
        set_bonus_timer(BONUS_TIME);
    }
    if(remove_bonus != 0) {
        board[bonus_ptr->position.x][bonus_ptr->position.y].element = 'V';

        pthread_mutex_lock(&remove_bonus_mutex);
        remove_bonus = 0;
        //bonus_displayed = 0;
        pthread_mutex_unlock(&remove_bonus_mutex);
    }

    // Update ghost behavior for each ghost
    for(int i = 0; i < NUM_GHOSTS; i++) {
        // Implement the update_ghost_behavior function according to your logic
        //update_ghost_behavior(&ghosts[i], pacman, board);
    }
    //refresh();
}

// ###### End of 5th step


// ###### Main Game Logic

int is_game_over(struct Pacman* pacman) {
    return (pacman->lives == 0);
}

// ###### Main Loop Of The Game

int main() {
    // Initialize ncurses
    initscr();
    // Hide the cursor
    hide_cursor();
    keypad(stdscr, TRUE);   // Enable special keys
    timeout(100);           // Set a timeout for getch

    // Declare and initialize the game board from a file
    struct Board_Element game_board[ROWS][COLS];
    struct Pacman pacman;
    struct Ghost ghosts[NUM_GHOSTS];
    struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS];
    struct Bonus_Item bonus = get_bonus_item(level);
    // Initialize game board from a file
    initialize_board_from_file(game_board, "maze.txt");
    // Build the Pellet array based on game board
    int total_of_pellets = build_pellet_array(game_board, pellets);
    // To Track how many pellets were eaten at the moment in current level
    int pellets_eaten = 0;
    // Display the initial game board
    print_board(game_board);

    // Add more initialization code for Pacman, ghosts and pellets if needed
    pacman.current.x = 26;
    pacman.current.y = 13;
    pacman.lives = 3;
    pacman.score = 0;
    pacman.speed = VERY_FAST;
    pacman.direction = NONE;

    ghosts[0].current.x = 14;
    ghosts[0].current.y = 13;
    ghosts[0].type = BLINKY;
    ghosts[0].mode = CHASE;
    ghosts[0].speed = NORMAL;

    ghosts[1].current.x = 17;
    ghosts[1].current.y = 13;
    ghosts[1].type = PINKY;
    ghosts[1].mode = CHASE;
    ghosts[1].speed = NORMAL;
    
    // Game loop
    while(!is_game_over(&pacman)){
        char input = get_input();
        // Handle user input and update Pacman's direction
        pacman.direction = handle_input(input, pacman.direction, game_board, &pacman);

        update_game_state(game_board, &pacman, ghosts, pellets, total_of_pellets, &pellets_eaten, &bonus);
        print_board(game_board);
        if(pacman.direction == NONE) {
            print_ready_banner();
        }
        print_score(pacman.score);
        print_lives(pacman.lives);
        print_current_level(level);
        print_fruit(bonus.symbol); // print strawberry or banana (todo)
        //mvprintw(36, 28, "%d", pellets_eaten);
        //mvprintw(36, 33, "%d", pacman.direction);
    }

    // Display game over message and final score
    refresh();
    // End ncurses
    endwin();

    return 0;
}

// ###### End Of Main Loop

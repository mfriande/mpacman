#include <stdio.h>
#include <stdlib.h>
// install library: sudo apt-get install libncurses5-dev libncursesw5-dev
// compile with: gcc -g game_board_design.c -o game_board -lncurses
#include <ncurses.h> 
#include <unistd.h> // Include the header for usleep
                    
#define ROWS 36
#define COLS 28
#define NUM_GHOSTS 4
#define NUM_PELLETS 240
#define NUM_ENERGIZER_PELLETS 4

// ###### 3rd step - Define Game Entities

// Enum to represent Pacman's direction
enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
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
    CHASE
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
    TUNNEL_END          // W
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
    enum Speed speed;
    int lives; // Number of lives
    int score; // Player's score
    struct Tile_Coordinates current;    // current position
    struct Tile_Coordinates next;       // next position 
};


// Example structure for Ghost
struct Ghost {
    int speed;
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

struct Maze {
    // define the maze layout, walls, etc
};

// 2D board array contains the index to the pellet array to be O(1) complexity search
struct Board_Element {
    char element;
    int index_to_pellet_array;
};

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
    mvprintw(1, 5, "SCORE: %d", score);
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
enum Direction handle_input(char input, enum Direction current) {
    switch (input) {
        case 'w':
            return UP;
        case 's':
            return DOWN;
        case 'a':
            return LEFT;
        case 'd':
            return RIGHT;
        // add more cases for other keys if needed
        default:
            return current; // no change in direction if an invalid key is pressed
    }
}

char get_input() {
    return getch();
}
/*
int is_valid_move(char board[ROWS][COLS], int x, int y) {
    return (x >= 0 && x < ROWS && y >= 0 && y < COLS && board[x][y] != '#');
}
*/

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

void check_tunnel_collision(struct Pacman* pacman) {
    // Check if Pacman's current position is within the left tunnel
    if((pacman->current.x == tunnel.left_entrance.x && 
        pacman->current.y >= tunnel.left_exit.y &&
        pacman->current.y <= tunnel.left_entrance.y)){
        // Check if Pacman is at exit of the left tunnel
        if(pacman->next.y == tunnel.left_exit.y) {
            // Set Pacman's next position to the entrance of the right
            pacman->next.y = tunnel.right_exit.y;
        }    
        pacman->speed = VERY_FAST;
    
    } 
    // Check if Pacman's current position is within the right tunnel
    else if((pacman->current.x == tunnel.right_entrance.x &&
            pacman->current.y <= tunnel.right_exit.y &&
            pacman->current.y >= tunnel.right_entrance.y)) {
            // Check if Pacman is at exit of the right tunnel
            if(pacman->next.y == tunnel.right_exit.y) {
                // Set Pacman's next position to the entrance of the left
                pacman->next.y = tunnel.left_exit.y;
            }
            pacman->speed = VERY_FAST;  
    } else {
        pacman->speed = NORMAL;
    }
    update_pacman_speed(pacman->speed);
}

void check_wall_collision(struct Board_Element board[ROWS][COLS], struct Pacman* pacman) {
    // get Pacman's next position
    int next_x = pacman->next.x;
    int next_y = pacman->next.y;

    // Check if Pacman's next position collides with a wall
    if(board[next_x][next_y].element == '#') {
        // Pacman collided with a wall, reset his next position to his current position
        pacman->next.x = pacman->current.x;
        pacman->next.y = pacman->current.y;
    }
}

void check_pellet_collision(struct Board_Element board[ROWS][COLS], struct Pacman* pacman, struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS]) {
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
        // Mark the pellet as eaten in pellet's array
        pellets[board[next_x][next_y].index_to_pellet_array].eaten = 1;
    }
}
//void check_collisions(char board[ROWS][COLS], struct Pacman* pacman, struct Ghost ghosts[NUM_GHOSTS], struct Pellet pellets[NUM_PELLETS]) {
void check_collisions(struct Board_Element board[ROWS][COLS], struct Pacman* pacman, struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS]) {
    // Implement collision logic based on your game design
    // For example, check if Pacman collides with pellets, ghosts or walls
    // Update the game state accordingly
    check_wall_collision(board, pacman);
    check_pellet_collision(board, pacman, pellets);
    check_tunnel_collision(pacman);
}

void move_pacman(struct Board_Element board[ROWS][COLS], struct Pacman* pacman, enum Direction direction, struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS]) {
    pacman->next.x = pacman->current.x;
    pacman->next.y = pacman->current.y;
    update_pacman_speed(pacman->speed);
    switch (direction) {
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
        // Add more cases for other keys if needed
    }
    if(is_valid_move(pacman->next)) {
        check_collisions(board, pacman, pellets);
        // Clear the current position on the board
        board[pacman->current.x][pacman->current.y].element = ' ';
        // Update Pacman's current position
        pacman->current.x = pacman->next.x;
        pacman->current.y = pacman->next.y;
        // Place Pacman in the new position on the board
        board[pacman->next.x][pacman->next.y].element = 'O';
    }
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

void update_game_state(struct Board_Element board[ROWS][COLS], enum Direction current, struct Pacman* pacman, struct Ghost ghosts[NUM_GHOSTS], struct Pellet pellets[NUM_PELLETS]) {
    
    move_pacman(board, pacman, current, pellets);
    // Update ghost behavior for each ghost
    for(int i = 0; i < NUM_GHOSTS; i++) {
        // Implement the update_ghost_behavior function according to your logic
        //update_ghost_behavior(&ghosts[i], pacman, board);
    }
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
    keypad(stdscr, TRUE);   // Enable special keys
    timeout(100);           // Set a timeout for getch

    // Declare and initialize the game board from a file
    struct Board_Element game_board[ROWS][COLS];
    struct Pacman pacman;
    struct Ghost ghosts[NUM_GHOSTS];
    struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS];
    // Initialize game board from a file
    initialize_board_from_file(game_board, "maze2.txt");
    // Build the Pellet array based on game board
    int total_of_pellets = build_pellet_array(game_board, pellets);
    printf("NUM OF PELLETS : %d", total_of_pellets);
    // Display the initial game board
    print_board(game_board);

    // Add more initialization code for Pacman, ghosts and pellets if needed
    pacman.current.x = 26;
    pacman.current.y = 13;
    pacman.lives = 3;
    pacman.score = 0;
    pacman.speed = NORMAL;

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

    // Initialize Pacman's direction
    enum Direction current_direction = RIGHT;
    
    // Game loop
    while(!is_game_over(&pacman)){
        char input = get_input();

        // Handle user input and update Pacman's direction
        current_direction = handle_input(input, current_direction);

        update_game_state(game_board, current_direction, &pacman, ghosts, pellets);
        print_board(game_board);
        print_score(pacman.score);
    }

    // Display game over message and final score

    // End ncurses
    endwin();

    return 0;
}

// ###### End Of Main Loop
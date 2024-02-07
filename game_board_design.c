#include <stdio.h>
#include <stdlib.h>
// install library: sudo apt-get install libncurses5-dev libncursesw5-dev
// compile with: gcc -g game_board_design.c -o game_board -lncurses
#include <ncurses.h> 
#include <unistd.h> // Include the header for usleep
                    
#define ROWS 36
#define COLS 28
#define NUM_GHOSTS 2
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

struct Tile_Coordinates {
    int x, y;
};

// Example structure for Pacman
struct Pacman {
    int speed;
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
    int eated;                          // flag indicates if the pellet was already eated by pacman
};

struct Maze {
    // define the maze layout, walls, etc
};

// ###### End of 3rd step

// ###### 2nd step - Designing the Game Board

// Function to initialize the game board from file
void initialize_board_from_file(char board[ROWS][COLS], const char* filename) {
    FILE* file = fopen(filename, "r");
    if(file == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            // fscanf(FILE *stream, const char *format, ...)
            fscanf(file, " %c", &board[i][j]); // add space to format to consume extra characters(new line , spaces, Tabs etc)
        }
    }
    fclose(file);
}

// Function to print the game board to stdout
void print_board(char board[ROWS][COLS]) {
    clear(); // Clear the screen before displaying the updated board
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            printw("%c ", board[i][j]);
        }
        printw("\n");
    }
    refresh(); // Refresh the screen to show the updated content
}

// Function to print the game board to a file
void print_board_to_file(char board[ROWS][COLS], const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            fprintf(file, "%c", board[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

// Function to build an array of pellets from the 2D game board
int build_pellet_array(char board[ROWS][COLS], struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS]) {
    int pellet_count = 0; // Counter for the number of pellets found

    // Iterate over each cell of the game board
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            // check if the cell contains a pellet or energized pellet
            if(board[i][j] == '.' || board[i][j] == '*') {
                // Add pellet to the pellet array
                pellets[pellet_count].position.x = i;
                pellets[pellet_count].position.y = j;
                pellets[pellet_count].eated = 0;        // initialize pellets (at the beginning every pellet state not eated)
                // Set is_energizer based on the cell content
                pellets[pellet_count].is_energyzer = (board[i][j] == '*');

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

int is_valid_move(char board[ROWS][COLS], struct Tile_Coordinates target) {
    return (target.x >= 0 && target.x < ROWS && target.y >= 0 && target.y < COLS);
}

void check_wall_collision(char board[ROWS][COLS], struct Pacman* pacman) {
    // get Pacman's next position
    int next_x = pacman->next.x;
    int next_y = pacman->next.y;

    // Check if Pacman's next position collides with a wall
    if(board[next_x][next_y] == '#') {
        // Pacman collided with a wall, reset his next position to his current position
        pacman->next.x = pacman->current.x;
        pacman->next.y = pacman->current.y;
    }
}
//void check_collisions(char board[ROWS][COLS], struct Pacman* pacman, struct Ghost ghosts[NUM_GHOSTS], struct Pellet pellets[NUM_PELLETS]) {
void check_collisions(char board[ROWS][COLS], struct Pacman* pacman) {
    // Implement collision logic based on your game design
    // For example, check if Pacman collides with pellets, ghosts or walls
    // Update the game state accordingly
    check_wall_collision(board, pacman);
}

void move_pacman(char board[ROWS][COLS], struct Pacman* pacman, enum Direction direction) {
    pacman->next.x = pacman->current.x;
    pacman->next.y = pacman->current.y;

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
    if(is_valid_move(board, pacman->next)) {
        check_collisions(board, pacman);
        // Clear the current position on the board
        board[pacman->current.x][pacman->current.y] = ' ';
        // Update Pacman's current position
        pacman->current.x = pacman->next.x;
        pacman->current.y = pacman->next.y;
        // Place Pacman in the new position on the board
        board[pacman->next.x][pacman->next.y] = 'O';
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

void update_score(struct Pacman* pacman, struct Pellet pellets[NUM_PELLETS]) {
    // Implement scoring logic on your game design
    // For example, update the score when Pacman collects pellets or interacts with ghosts
}

void update_game_state(char board[ROWS][COLS], enum Direction current, struct Pacman* pacman, struct Ghost ghosts[NUM_GHOSTS], struct Pellet pellets[NUM_PELLETS]) {
    
    move_pacman(board, pacman, current);
    // Update ghost behavior for each ghost
    for(int i = 0; i < NUM_GHOSTS; i++) {
        // Implement the update_ghost_behavior function according to your logic
        //update_ghost_behavior(&ghosts[i], pacman, board);
    }

    // Check for collisions
    // check_collisions(board, pacman, ghosts, pellets);

    // Update score
    update_score(pacman, pellets);
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
    char game_board[ROWS][COLS];
    struct Pacman pacman;
    struct Ghost ghosts[NUM_GHOSTS];
    struct Pellet pellets[NUM_PELLETS + NUM_ENERGIZER_PELLETS];
    // Initialize game board from a file
    initialize_board_from_file(game_board, "maze.txt");
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

    
    // Introduce a delay between moves to limit Pacman speed (adjust the value as needed)
    int move_delay = 250000; // 100,000 microseconds = 0.1 seconds
    // Initialize Pacman's direction
    enum Direction current_direction = RIGHT;
        
    // Game loop
    while(!is_game_over(&pacman)){
        char input = get_input();

        // Handle user input and update Pacman's direction
        current_direction = handle_input(input, current_direction);

        update_game_state(game_board, current_direction, &pacman, ghosts, pellets);
        print_board(game_board);
        // introduce a delay between moves
        usleep(move_delay);
    }

    // Display game over message and final score

    // End ncurses
    endwin();

    return 0;
}

// ###### End Of Main Loop
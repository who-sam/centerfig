#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <termios.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 1000

// Global variables
static struct termios old_term;
static char figlet_output[MAX_LINES][MAX_LINE_LENGTH];
static int figlet_lines = 0;
static time_t start_time;
static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t need_redraw = 0;
static char *input_text = NULL;

// Function prototypes
void cleanup(void);
void signal_handler(int sig);
void setup_terminal(void);
void restore_terminal(void);
void get_terminal_size(int *rows, int *cols);
void generate_figlet(const char *text);
void draw_display(void);
void draw_timer(void);

// Cleanup function
void cleanup(void) {
    restore_terminal();
    printf("\033[?25h"); // Show cursor
    printf("\033[2J");   // Clear screen
    printf("\033[H");    // Move cursor to home
    fflush(stdout);
}

// Signal handler
void signal_handler(int sig) {
    if (sig == SIGWINCH) {
        need_redraw = 1;
    } else if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
    }
}

// Setup terminal for raw mode
void setup_terminal(void) {
    struct termios new_term;
    
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 0;
    new_term.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
}

// Restore terminal settings
void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
}

// Get terminal size
void get_terminal_size(int *rows, int *cols) {
    struct winsize ws;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        *rows = 24;
        *cols = 80;
    } else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    }
}

// Generate figlet output
void generate_figlet(const char *text) {
    FILE *fp;
    char cmd[1024];
    
    snprintf(cmd, sizeof(cmd), "figlet %s", text);
    fp = popen(cmd, "r");
    
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not run figlet\n");
        exit(1);
    }
    
    figlet_lines = 0;
    while (fgets(figlet_output[figlet_lines], MAX_LINE_LENGTH, fp) != NULL && 
           figlet_lines < MAX_LINES) {
        // Remove newline
        size_t len = strlen(figlet_output[figlet_lines]);
        if (len > 0 && figlet_output[figlet_lines][len-1] == '\n') {
            figlet_output[figlet_lines][len-1] = '\0';
        }
        figlet_lines++;
    }
    
    pclose(fp);
}

// Draw the centered display
void draw_display(void) {
    int rows, cols;
    get_terminal_size(&rows, &cols);
    
    // Regenerate figlet for new terminal size
    generate_figlet(input_text);
    
    // Clear screen
    printf("\033[2J");
    
    // Calculate vertical centering
    int top_padding = (rows - figlet_lines) / 2;
    if (top_padding < 0) top_padding = 0;
    
    // Move to starting position
    printf("\033[%d;1H", top_padding + 1);
    
    // Draw each line centered
    for (int i = 0; i < figlet_lines; i++) {
        int line_len = strlen(figlet_output[i]);
        int left_padding = (cols - line_len) / 2;
        if (left_padding < 0) left_padding = 0;
        
        printf("\033[%dC%s\n", left_padding, figlet_output[i]);
    }
    
    fflush(stdout);
}

// Draw timer at bottom
void draw_timer(void) {
    int rows, cols;
    get_terminal_size(&rows, &cols);
    
    time_t current_time = time(NULL);
    time_t elapsed = current_time - start_time;
    
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;
    
    // Move to bottom center
    int timer_col = (cols - 8) / 2;
    if (timer_col < 0) timer_col = 0;
    
    printf("\033[%d;1H", rows);  // Move to last line
    printf("\033[2K");            // Clear line
    printf("\033[%dC", timer_col); // Move to center
    printf("%02d:%02d:%02d", hours, minutes, seconds);
    
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [text to display]\n", argv[0]);
        fprintf(stderr, "Example: %s Hello World\n", argv[0]);
        return 1;
    }
    
    // Combine all arguments into input text
    size_t total_len = 0;
    for (int i = 1; i < argc; i++) {
        total_len += strlen(argv[i]) + 1;
    }
    
    input_text = malloc(total_len);
    if (input_text == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    input_text[0] = '\0';
    for (int i = 1; i < argc; i++) {
        strcat(input_text, argv[i]);
        if (i < argc - 1) strcat(input_text, " ");
    }
    
    // Setup
    atexit(cleanup);
    signal(SIGWINCH, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    setup_terminal();
    
    printf("\033[?25l"); // Hide cursor
    
    start_time = time(NULL);
    
    // Initial draw
    draw_display();
    draw_timer();
    
    // Main loop
    while (running) {
        // Check for window resize
        if (need_redraw) {
            draw_display();
            need_redraw = 0;
        }
        
        // Update timer
        draw_timer();
        
        // Check for keypress
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            break; // Any key exits
        }
        
        usleep(100000); // Sleep 100ms
    }
    
    free(input_text);
    
    return 0;
}

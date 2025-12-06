#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <getopt.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 1000

// Timer modes
typedef enum {
    MODE_TIMER,      // Count down from set time
    MODE_STOPWATCH,  // Count up from zero
    MODE_NONE        // No timer display
} TimerMode;

// Global variables
static struct termios old_term;
static char figlet_output[MAX_LINES][MAX_LINE_LENGTH];
static int figlet_lines = 0;
static time_t start_time;
static time_t initial_seconds = 0;
static TimerMode timer_mode = MODE_STOPWATCH;
static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t need_redraw = 0;
static char *input_text = NULL;
static int use_color = 0;
static char *font = NULL;

// Color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

// Function prototypes
void cleanup(void);
void signal_handler(int sig);
void setup_terminal(void);
void restore_terminal(void);
void get_terminal_size(int *rows, int *cols);
void generate_figlet(const char *text);
void draw_display(void);
void draw_timer(void);
void print_usage(const char *program_name);
int parse_time(const char *time_str);

// Cleanup function
void cleanup(void) {
    restore_terminal();
    printf("\033[?25h"); // Show cursor
    printf("\033[2J");   // Clear screen
    printf("\033[H");    // Move cursor to home
    printf(COLOR_RESET); // Reset colors
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
    char cmd[2048];
    
    if (font != NULL) {
        snprintf(cmd, sizeof(cmd), "figlet -f %s %s", font, text);
    } else {
        snprintf(cmd, sizeof(cmd), "figlet %s", text);
    }
    
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
    if (timer_mode != MODE_NONE) {
        top_padding = (rows - figlet_lines - 2) / 2; // Leave space for timer
    }
    if (top_padding < 0) top_padding = 0;
    
    // Move to starting position
    printf("\033[%d;1H", top_padding + 1);
    
    // Apply color if enabled
    if (use_color) {
        printf(COLOR_CYAN);
    }
    
    // Draw each line centered
    for (int i = 0; i < figlet_lines; i++) {
        int line_len = strlen(figlet_output[i]);
        int left_padding = (cols - line_len) / 2;
        if (left_padding < 0) left_padding = 0;
        
        printf("\033[%dC%s\n", left_padding, figlet_output[i]);
    }
    
    if (use_color) {
        printf(COLOR_RESET);
    }
    
    fflush(stdout);
}

// Draw timer at bottom
void draw_timer(void) {
    if (timer_mode == MODE_NONE) return;
    
    int rows, cols;
    get_terminal_size(&rows, &cols);
    
    time_t current_time = time(NULL);
    time_t elapsed;
    
    if (timer_mode == MODE_STOPWATCH) {
        elapsed = current_time - start_time;
    } else { // MODE_TIMER
        elapsed = initial_seconds - (current_time - start_time);
        if (elapsed < 0) elapsed = 0;
    }
    
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;
    
    // Move to bottom center
    int timer_col = (cols - 8) / 2;
    if (timer_col < 0) timer_col = 0;
    
    printf("\033[%d;1H", rows);  // Move to last line
    printf("\033[2K");            // Clear line
    printf("\033[%dC", timer_col); // Move to center
    
    // Color timer based on mode and time
    if (use_color) {
        if (timer_mode == MODE_TIMER && elapsed <= 60) {
            printf(COLOR_RED); // Red when less than 1 minute in timer mode
        } else if (timer_mode == MODE_TIMER && elapsed <= 300) {
            printf(COLOR_YELLOW); // Yellow when less than 5 minutes
        } else {
            printf(COLOR_GREEN);
        }
    }
    
    printf("%02d:%02d:%02d", hours, minutes, seconds);
    
    if (use_color) {
        printf(COLOR_RESET);
    }
    
    fflush(stdout);
    
    // Exit if timer reaches zero
    if (timer_mode == MODE_TIMER && elapsed <= 0) {
        running = 0;
    }
}

// Parse time string (supports formats: 90, 1:30, 1:30:00)
int parse_time(const char *time_str) {
    int hours = 0, minutes = 0, seconds = 0;
    int count = sscanf(time_str, "%d:%d:%d", &hours, &minutes, &seconds);
    
    if (count == 1) {
        // Just seconds or minutes (assume minutes if > 60)
        if (hours > 60) {
            minutes = hours;
            hours = 0;
        } else {
            seconds = hours;
            hours = 0;
        }
    } else if (count == 2) {
        // MM:SS format
        seconds = minutes;
        minutes = hours;
        hours = 0;
    }
    
    return hours * 3600 + minutes * 60 + seconds;
}

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] [text to display]\n\n", program_name);
    printf("Options:\n");
    printf("  -t, --timer TIME      Start countdown timer from TIME\n");
    printf("                        Formats: seconds (90), MM:SS (1:30), HH:MM:SS (0:1:30)\n");
    printf("  -s, --stopwatch       Count up from zero (default)\n");
    printf("  -n, --no-timer        Display text without timer\n");
    printf("  -c, --color           Use colored output\n");
    printf("  -f, --font FONT       Use specific figlet font\n");
    printf("  -h, --help            Display this help and exit\n\n");
    printf("Examples:\n");
    printf("  %s Break Time                    # Stopwatch mode\n", program_name);
    printf("  %s -t 300 Take a Break           # 5 minute timer\n", program_name);
    printf("  %s -t 25:00 Pomodoro             # 25 minute timer\n", program_name);
    printf("  %s -n \"Meeting at 3PM\"           # No timer\n", program_name);
    printf("  %s -c -f slant \"Be Awesome\"      # Colored with slant font\n", program_name);
}

int main(int argc, char *argv[]) {
    int opt;
    
    static struct option long_options[] = {
        {"timer",      required_argument, 0, 't'},
        {"stopwatch",  no_argument,       0, 's'},
        {"no-timer",   no_argument,       0, 'n'},
        {"color",      no_argument,       0, 'c'},
        {"font",       required_argument, 0, 'f'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    // Parse options
    while ((opt = getopt_long(argc, argv, "t:sncf:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 't':
                timer_mode = MODE_TIMER;
                initial_seconds = parse_time(optarg);
                if (initial_seconds <= 0) {
                    fprintf(stderr, "Error: Invalid time format\n");
                    return 1;
                }
                break;
            case 's':
                timer_mode = MODE_STOPWATCH;
                break;
            case 'n':
                timer_mode = MODE_NONE;
                break;
            case 'c':
                use_color = 1;
                break;
            case 'f':
                font = strdup(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Get remaining arguments as text
    if (optind >= argc) {
        fprintf(stderr, "Error: No text provided\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Combine all remaining arguments into input text
    size_t total_len = 0;
    for (int i = optind; i < argc; i++) {
        total_len += strlen(argv[i]) + 1;
    }
    
    input_text = malloc(total_len);
    if (input_text == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    input_text[0] = '\0';
    for (int i = optind; i < argc; i++) {
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
    if (timer_mode != MODE_NONE) {
        draw_timer();
    }
    
    // Main loop
    while (running) {
        // Check for window resize
        if (need_redraw) {
            draw_display();
            need_redraw = 0;
        }
        
        // Update timer
        if (timer_mode != MODE_NONE) {
            draw_timer();
        }
        
        // Check for keypress
        char c;
        if (read(STDIN_FILENO, &c, 1) > 0) {
            break; // Any key exits
        }
        
        usleep(100000); // Sleep 100ms
    }
    
    free(input_text);
    if (font != NULL) free(font);
    
    return 0;
}

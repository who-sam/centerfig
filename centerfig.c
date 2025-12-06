#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <getopt.h>
#include <errno.h>
#include <dirent.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 1000
#define MAX_PATH 512
#define MAX_TASK_NAME 256

// Timer modes
typedef enum {
    MODE_TIMER,      // Count down from set time
    MODE_STOPWATCH,  // Count up from zero
    MODE_NONE        // No timer display
} TimerMode;

// Configuration structure
typedef struct {
    char logdir[MAX_PATH];
    char dateformat[64];
    char timeformat[64];
    int use_color;
    char default_font[64];
} Config;

// Session data
typedef struct {
    char task_name[MAX_TASK_NAME];
    time_t start_time;
    time_t end_time;
    int duration_seconds;
} Session;

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
static int enable_logging = 0;
static Config config;

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

// Config functions
void init_default_config(void);
void load_config(void);
void save_config(void);
void show_config(void);
void set_config(const char *key, const char *value);
void reset_config(void);

// Logging functions
void ensure_log_directory(const char *date_str);
void log_session(Session *session);
void show_log(const char *date_filter);
void show_stats(const char *filter);
void show_report(const char *period);
char* get_log_path(const char *date_str);
void parse_date_filter(const char *filter, char *date_out);
void get_today_date(char *buffer, size_t size);
void calculate_task_totals(const char *filepath, const char *task_filter);
void parse_log_file(const char *filepath, void (*callback)(const char*, int, void*), void *data);
void get_date_range(const char *period, char *start_date, char *end_date);
void scan_log_files(const char *start_date, const char *end_date, void (*callback)(const char*, int, void*), void *data);

// Initialize default configuration
void init_default_config(void) {
    char *home = getenv("HOME");
    snprintf(config.logdir, MAX_PATH, "%s/.config/centerfig/logs", home ? home : ".");
    strcpy(config.dateformat, "%Y-%m-%d");
    strcpy(config.timeformat, "%H:%M:%S");
    config.use_color = 0;
    strcpy(config.default_font, "standard");
}

// Get config directory path
void get_config_dir(char *buffer, size_t size) {
    char *home = getenv("HOME");
    snprintf(buffer, size, "%s/.config/centerfig", home ? home : ".");
}

// Get config file path
void get_config_path(char *buffer, size_t size) {
    char config_dir[MAX_PATH];
    get_config_dir(config_dir, sizeof(config_dir));
    snprintf(buffer, size, "%s/config.ini", config_dir);
}

// Load configuration from file
void load_config(void) {
    init_default_config();
    
    char config_path[MAX_PATH];
    get_config_path(config_path, sizeof(config_path));
    
    FILE *fp = fopen(config_path, "r");
    if (!fp) return;
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') continue;
        
        // Parse key=value
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char *key = line;
            char *value = eq + 1;
            
            // Trim whitespace
            while (*key == ' ') key++;
            while (*value == ' ') value++;
            
            if (strcmp(key, "logdir") == 0) {
                strncpy(config.logdir, value, MAX_PATH - 1);
            } else if (strcmp(key, "dateformat") == 0) {
                strncpy(config.dateformat, value, 63);
            } else if (strcmp(key, "timeformat") == 0) {
                strncpy(config.timeformat, value, 63);
            } else if (strcmp(key, "color") == 0) {
                config.use_color = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "default_font") == 0) {
                strncpy(config.default_font, value, 63);
            }
        }
    }
    
    fclose(fp);
}

// Save configuration to file
void save_config(void) {
    char config_dir[MAX_PATH];
    get_config_dir(config_dir, sizeof(config_dir));
    
    // Create config directory
    mkdir(config_dir, 0755);
    
    char config_path[MAX_PATH];
    get_config_path(config_path, sizeof(config_path));
    
    FILE *fp = fopen(config_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: Could not write config file\n");
        return;
    }
    
    fprintf(fp, "# centerfig configuration\n\n");
    fprintf(fp, "logdir=%s\n", config.logdir);
    fprintf(fp, "dateformat=%s\n", config.dateformat);
    fprintf(fp, "timeformat=%s\n", config.timeformat);
    fprintf(fp, "color=%s\n", config.use_color ? "true" : "false");
    fprintf(fp, "default_font=%s\n", config.default_font);
    
    fclose(fp);
    printf("Configuration saved to %s\n", config_path);
}

// Show current configuration
void show_config(void) {
    printf("Current Configuration:\n\n");
    printf("  logdir        = %s\n", config.logdir);
    printf("  dateformat    = %s\n", config.dateformat);
    printf("  timeformat    = %s\n", config.timeformat);
    printf("  color         = %s\n", config.use_color ? "true" : "false");
    printf("  default_font  = %s\n", config.default_font);
}

// Set configuration value
void set_config(const char *key, const char *value) {
    if (strcmp(key, "logdir") == 0) {
        strncpy(config.logdir, value, MAX_PATH - 1);
    } else if (strcmp(key, "dateformat") == 0) {
        strncpy(config.dateformat, value, 63);
    } else if (strcmp(key, "timeformat") == 0) {
        strncpy(config.timeformat, value, 63);
    } else if (strcmp(key, "color") == 0) {
        config.use_color = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "default_font") == 0) {
        strncpy(config.default_font, value, 63);
    } else {
        fprintf(stderr, "Error: Unknown config key '%s'\n", key);
        return;
    }
    
    save_config();
    printf("Set %s = %s\n", key, value);
}

// Reset configuration to defaults
void reset_config(void) {
    init_default_config();
    save_config();
    printf("Configuration reset to defaults\n");
}

// Get today's date
void get_today_date(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, config.dateformat, tm_info);
}

// Get log file path for a given date
char* get_log_path(const char *date_str) {
    static char path[MAX_PATH];
    struct tm tm_info = {0};
    
    // Parse date string
    strptime(date_str, config.dateformat, &tm_info);
    
    char year[8], month[8];
    strftime(year, sizeof(year), "%Y", &tm_info);
    strftime(month, sizeof(month), "%m", &tm_info);
    
    snprintf(path, MAX_PATH, "%s/%s/%s/%s.md", 
             config.logdir, year, month, date_str);
    
    return path;
}

// Ensure log directory exists
void ensure_log_directory(const char *date_str) {
    struct tm tm_info = {0};
    strptime(date_str, config.dateformat, &tm_info);
    
    char year[8], month[8];
    strftime(year, sizeof(year), "%Y", &tm_info);
    strftime(month, sizeof(month), "%m", &tm_info);
    
    char dir_path[MAX_PATH];
    
    // Create base log directory
    mkdir(config.logdir, 0755);
    
    // Create year directory
    snprintf(dir_path, MAX_PATH, "%s/%s", config.logdir, year);
    mkdir(dir_path, 0755);
    
    // Create month directory
    snprintf(dir_path, MAX_PATH, "%s/%s/%s", config.logdir, year, month);
    mkdir(dir_path, 0755);
}

// Log a session
void log_session(Session *session) {
    char date_str[32];
    struct tm *tm_info = localtime(&session->start_time);
    strftime(date_str, sizeof(date_str), config.dateformat, tm_info);
    
    ensure_log_directory(date_str);
    
    char *log_path = get_log_path(date_str);
    
    // Check if file exists
    int file_exists = (access(log_path, F_OK) == 0);
    
    FILE *fp = fopen(log_path, "a");
    if (!fp) {
        fprintf(stderr, "Error: Could not open log file %s\n", log_path);
        return;
    }
    
    // Write header if new file
    if (!file_exists) {
        fprintf(fp, "# Time Log - %s\n\n", date_str);
        fprintf(fp, "| Task | Duration | Start Time | End Time |\n");
        fprintf(fp, "|------|----------|------------|----------|\n");
    }
    
    // Format times
    char start_str[32], end_str[32];
    struct tm *start_tm = localtime(&session->start_time);
    struct tm *end_tm = localtime(&session->end_time);
    strftime(start_str, sizeof(start_str), config.timeformat, start_tm);
    strftime(end_str, sizeof(end_str), config.timeformat, end_tm);
    
    // Format duration
    int hours = session->duration_seconds / 3600;
    int minutes = (session->duration_seconds % 3600) / 60;
    int seconds = session->duration_seconds % 60;
    
    // Write entry
    fprintf(fp, "| %s | %02d:%02d:%02d | %s | %s |\n",
            session->task_name, hours, minutes, seconds, start_str, end_str);
    
    fclose(fp);
    
    printf("\n✓ Logged: %s - %02d:%02d:%02d (from %s to %s)\n", 
           session->task_name, hours, minutes, seconds, start_str, end_str);
    printf("  Saved to: %s\n", log_path);
}

// Show log for a specific date
void show_log(const char *date_filter) {
    char date_str[32];
    parse_date_filter(date_filter, date_str);
    
    char *log_path = get_log_path(date_str);
    
    FILE *fp = fopen(log_path, "r");
    if (!fp) {
        printf("No log found for %s\n", date_str);
        return;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    
    fclose(fp);
}

// Parse date filter (today, yesterday, or YYYY-MM-DD)
void parse_date_filter(const char *filter, char *date_out) {
    if (!filter || strcmp(filter, "today") == 0) {
        get_today_date(date_out, 32);
    } else if (strcmp(filter, "yesterday") == 0) {
        time_t yesterday = time(NULL) - 86400;
        struct tm *tm_info = localtime(&yesterday);
        strftime(date_out, 32, config.dateformat, tm_info);
    } else {
        strncpy(date_out, filter, 31);
    }
}

// Structure to hold task statistics
typedef struct {
    char task_name[MAX_TASK_NAME];
    int total_seconds;
    int session_count;
} TaskStats;

typedef struct {
    TaskStats *tasks;
    int count;
    int capacity;
} TaskStatsCollection;

// Initialize task stats collection
void init_task_stats_collection(TaskStatsCollection *collection) {
    collection->capacity = 50;
    collection->count = 0;
    collection->tasks = malloc(sizeof(TaskStats) * collection->capacity);
}

// Free task stats collection
void free_task_stats_collection(TaskStatsCollection *collection) {
    free(collection->tasks);
}

// Add or update task in collection
void add_task_time(TaskStatsCollection *collection, const char *task_name, int duration_seconds) {
    // Look for existing task
    for (int i = 0; i < collection->count; i++) {
        if (strcmp(collection->tasks[i].task_name, task_name) == 0) {
            collection->tasks[i].total_seconds += duration_seconds;
            collection->tasks[i].session_count++;
            return;
        }
    }
    
    // Add new task
    if (collection->count >= collection->capacity) {
        collection->capacity *= 2;
        collection->tasks = realloc(collection->tasks, sizeof(TaskStats) * collection->capacity);
    }
    
    strncpy(collection->tasks[collection->count].task_name, task_name, MAX_TASK_NAME - 1);
    collection->tasks[collection->count].total_seconds = duration_seconds;
    collection->tasks[collection->count].session_count = 1;
    collection->count++;
}

// Parse a log file and call callback for each entry
void parse_log_file(const char *filepath, void (*callback)(const char*, int, void*), void *data) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return;
    
    char line[512];
    int in_table = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        // Skip header lines
        if (strstr(line, "| Task |") || strstr(line, "|------|")) {
            in_table = 1;
            continue;
        }
        
        if (!in_table || line[0] != '|') continue;
        
        // Parse: | task | HH:MM:SS | start | end |
        char task[MAX_TASK_NAME] = {0};
        char duration_str[32] = {0};
        int hours, minutes, seconds;
        
        // Manual parsing to handle fields correctly
        char *p = line + 1; // Skip first |
        int field = 0;
        char buffer[MAX_TASK_NAME];
        int buf_idx = 0;
        
        while (*p) {
            if (*p == '|') {
                buffer[buf_idx] = '\0';
                
                // Trim whitespace
                char *start = buffer;
                while (*start == ' ') start++;
                char *end = start + strlen(start) - 1;
                while (end > start && (*end == ' ' || *end == '\n')) *end-- = '\0';
                
                if (field == 0 && strlen(start) > 0) { // Task name
                    strncpy(task, start, MAX_TASK_NAME - 1);
                } else if (field == 1 && strlen(start) > 0) { // Duration
                    strncpy(duration_str, start, 31);
                    if (sscanf(duration_str, "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
                        int total_seconds = hours * 3600 + minutes * 60 + seconds;
                        if (strlen(task) > 0) {
                            callback(task, total_seconds, data);
                        }
                    }
                    break; // We have what we need
                }
                
                field++;
                buf_idx = 0;
            } else {
                if (buf_idx < MAX_TASK_NAME - 1) {
                    buffer[buf_idx++] = *p;
                }
            }
            p++;
        }
    }
    
    fclose(fp);
}

// Callback for collecting task stats
void collect_task_stats_callback(const char *task_name, int duration_seconds, void *data) {
    TaskStatsCollection *collection = (TaskStatsCollection *)data;
    add_task_time(collection, task_name, duration_seconds);
}

// Get date range for period (today, week, month)
void get_date_range(const char *period, char *start_date, char *end_date) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    if (strcmp(period, "today") == 0) {
        strftime(start_date, 32, config.dateformat, tm_info);
        strftime(end_date, 32, config.dateformat, tm_info);
    } else if (strcmp(period, "week") == 0) {
        // Start of week (Monday)
        time_t start = now - (tm_info->tm_wday == 0 ? 6 : tm_info->tm_wday - 1) * 86400;
        struct tm *start_tm = localtime(&start);
        strftime(start_date, 32, config.dateformat, start_tm);
        strftime(end_date, 32, config.dateformat, tm_info);
    } else if (strcmp(period, "month") == 0) {
        // Start of month
        tm_info->tm_mday = 1;
        mktime(tm_info);
        strftime(start_date, 32, config.dateformat, tm_info);
        
        // End of month (today)
        struct tm *end_tm = localtime(&now);
        strftime(end_date, 32, config.dateformat, end_tm);
    } else {
        // Default to today
        strftime(start_date, 32, config.dateformat, tm_info);
        strftime(end_date, 32, config.dateformat, tm_info);
    }
}

// Scan log files in date range
void scan_log_files(const char *start_date, const char *end_date, 
                    void (*callback)(const char*, int, void*), void *data) {
    struct tm start_tm = {0}, end_tm = {0};
    strptime(start_date, config.dateformat, &start_tm);
    strptime(end_date, config.dateformat, &end_tm);
    
    time_t start_time = mktime(&start_tm);
    time_t end_time = mktime(&end_tm);
    
    // Iterate through each day in range
    for (time_t t = start_time; t <= end_time; t += 86400) {
        struct tm *tm_info = localtime(&t);
        char date_str[32];
        strftime(date_str, sizeof(date_str), config.dateformat, tm_info);
        
        char *log_path = get_log_path(date_str);
        parse_log_file(log_path, callback, data);
    }
}

// Show statistics
void show_stats(const char *filter) {
    TaskStatsCollection collection;
    init_task_stats_collection(&collection);
    
    char start_date[32], end_date[32];
    
    if (!filter || strcmp(filter, "all") == 0) {
        // Scan all logs
        printf("Calculating statistics for all time...\n\n");
        
        // This is simplified - would need to recursively scan all log directories
        get_date_range("month", start_date, end_date);
        scan_log_files(start_date, end_date, collect_task_stats_callback, &collection);
    } else if (strcmp(filter, "today") == 0 || strcmp(filter, "week") == 0 || strcmp(filter, "month") == 0) {
        get_date_range(filter, start_date, end_date);
        printf("Statistics for %s (%s to %s):\n\n", filter, start_date, end_date);
        scan_log_files(start_date, end_date, collect_task_stats_callback, &collection);
    } else {
        // Specific task name
        get_date_range("month", start_date, end_date);
        scan_log_files(start_date, end_date, collect_task_stats_callback, &collection);
        
        // Filter for specific task
        printf("Statistics for task '%s':\n\n", filter);
        for (int i = 0; i < collection.count; i++) {
            if (strstr(collection.tasks[i].task_name, filter) != NULL) {
                int hours = collection.tasks[i].total_seconds / 3600;
                int minutes = (collection.tasks[i].total_seconds % 3600) / 60;
                int seconds = collection.tasks[i].total_seconds % 60;
                
                printf("  Total Time: %02d:%02d:%02d\n", hours, minutes, seconds);
                printf("  Sessions: %d\n", collection.tasks[i].session_count);
                printf("  Avg Session: %02d:%02d:%02d\n", 
                       (collection.tasks[i].total_seconds / collection.tasks[i].session_count) / 3600,
                       ((collection.tasks[i].total_seconds / collection.tasks[i].session_count) % 3600) / 60,
                       (collection.tasks[i].total_seconds / collection.tasks[i].session_count) % 60);
            }
        }
        free_task_stats_collection(&collection);
        return;
    }
    
    if (collection.count == 0) {
        printf("No data found.\n");
        free_task_stats_collection(&collection);
        return;
    }
    
    // Display statistics table
    printf("┌────────────────────────────────┬────────────┬──────────┬────────────┐\n");
    printf("│ Task                           │ Total Time │ Sessions │ Avg/Session│\n");
    printf("├────────────────────────────────┼────────────┼──────────┼────────────┤\n");
    
    int grand_total = 0;
    int grand_sessions = 0;
    
    for (int i = 0; i < collection.count; i++) {
        int hours = collection.tasks[i].total_seconds / 3600;
        int minutes = (collection.tasks[i].total_seconds % 3600) / 60;
        int seconds = collection.tasks[i].total_seconds % 60;
        
        int avg_seconds = collection.tasks[i].total_seconds / collection.tasks[i].session_count;
        int avg_hours = avg_seconds / 3600;
        int avg_minutes = (avg_seconds % 3600) / 60;
        int avg_secs = avg_seconds % 60;
        
        // Truncate task name if too long
        char task_display[32];
        strncpy(task_display, collection.tasks[i].task_name, 30);
        task_display[30] = '\0';
        if (strlen(collection.tasks[i].task_name) > 30) {
            task_display[29] = '.';
            task_display[28] = '.';
        }
        
        printf("│ %-30s │ %02d:%02d:%02d   │ %8d │ %02d:%02d:%02d   │\n",
               task_display, hours, minutes, seconds,
               collection.tasks[i].session_count,
               avg_hours, avg_minutes, avg_secs);
        
        grand_total += collection.tasks[i].total_seconds;
        grand_sessions += collection.tasks[i].session_count;
    }
    
    printf("├────────────────────────────────┼────────────┼──────────┼────────────┤\n");
    
    int total_hours = grand_total / 3600;
    int total_minutes = (grand_total % 3600) / 60;
    int total_seconds = grand_total % 60;
    
    printf("│ %-30s │ %02d:%02d:%02d   │ %8d │            │\n",
           "TOTAL", total_hours, total_minutes, total_seconds, grand_sessions);
    printf("└────────────────────────────────┴────────────┴──────────┴────────────┘\n");
    
    free_task_stats_collection(&collection);
}

// Show report
void show_report(const char *period) {
    char start_date[32], end_date[32];
    get_date_range(period, start_date, end_date);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  TIME TRACKING REPORT - %s\n", period);
    printf("  Period: %s to %s\n", start_date, end_date);
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    TaskStatsCollection collection;
    init_task_stats_collection(&collection);
    scan_log_files(start_date, end_date, collect_task_stats_callback, &collection);
    
    if (collection.count == 0) {
        printf("No data found for this period.\n");
        free_task_stats_collection(&collection);
        return;
    }
    
    // Calculate totals
    int grand_total = 0;
    int max_sessions = 0;
    TaskStats *most_time_task = NULL;
    
    for (int i = 0; i < collection.count; i++) {
        grand_total += collection.tasks[i].total_seconds;
        if (collection.tasks[i].session_count > max_sessions) {
            max_sessions = collection.tasks[i].session_count;
        }
        if (!most_time_task || collection.tasks[i].total_seconds > most_time_task->total_seconds) {
            most_time_task = &collection.tasks[i];
        }
    }
    
    // Summary
    printf("📊 SUMMARY\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("  Total Time Tracked: %02d:%02d:%02d\n", 
           grand_total / 3600, (grand_total % 3600) / 60, grand_total % 60);
    printf("  Total Tasks: %d\n", collection.count);
    printf("  Total Sessions: ");
    int total_sessions = 0;
    for (int i = 0; i < collection.count; i++) {
        total_sessions += collection.tasks[i].session_count;
    }
    printf("%d\n", total_sessions);
    
    if (most_time_task) {
        printf("  Most Time Spent: %s (%02d:%02d:%02d)\n",
               most_time_task->task_name,
               most_time_task->total_seconds / 3600,
               (most_time_task->total_seconds % 3600) / 60,
               most_time_task->total_seconds % 60);
    }
    printf("\n");
    
    // Task breakdown
    printf("📋 TASK BREAKDOWN\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    for (int i = 0; i < collection.count; i++) {
        int hours = collection.tasks[i].total_seconds / 3600;
        int minutes = (collection.tasks[i].total_seconds % 3600) / 60;
        
        // Calculate percentage
        float percentage = (float)collection.tasks[i].total_seconds / grand_total * 100;
        
        printf("\n  %s\n", collection.tasks[i].task_name);
        printf("    Time: %02d:%02d:%02d (%.1f%%)\n", 
               hours, minutes, collection.tasks[i].total_seconds % 60, percentage);
        printf("    Sessions: %d\n", collection.tasks[i].session_count);
        
        // Visual bar
        int bar_width = (int)(percentage / 2); // Max 50 chars
        printf("    ");
        for (int j = 0; j < bar_width; j++) printf("█");
        printf("\n");
    }
    
    printf("\n═══════════════════════════════════════════════════════════════\n\n");
    
    free_task_stats_collection(&collection);
}

// Cleanup function
void cleanup(void) {
    restore_terminal();
    printf("\033[?25h"); // Show cursor
    printf("\033[2J");   // Clear screen
    printf("\033[H");    // Move cursor to home
    printf("\033[0m");   // Reset colors
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
    char *use_font = font ? font : config.default_font;
    
    snprintf(cmd, sizeof(cmd), "figlet -f %s '%s'", use_font, text);
    
    fp = popen(cmd, "r");
    
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not run figlet\n");
        exit(1);
    }
    
    figlet_lines = 0;
    while (fgets(figlet_output[figlet_lines], MAX_LINE_LENGTH, fp) != NULL && 
           figlet_lines < MAX_LINES) {
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
    
    generate_figlet(input_text);
    
    printf("\033[2J");
    
    int top_padding = (rows - figlet_lines) / 2;
    if (timer_mode != MODE_NONE) {
        top_padding = (rows - figlet_lines - 2) / 2;
    }
    if (top_padding < 0) top_padding = 0;
    
    printf("\033[%d;1H", top_padding + 1);
    
    int color_enabled = use_color || config.use_color;
    if (color_enabled) {
        printf("\033[36m");
    }
    
    for (int i = 0; i < figlet_lines; i++) {
        int line_len = strlen(figlet_output[i]);
        int left_padding = (cols - line_len) / 2;
        if (left_padding < 0) left_padding = 0;
        
        printf("\033[%dC%s\n", left_padding, figlet_output[i]);
    }
    
    if (color_enabled) {
        printf("\033[0m");
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
    } else {
        elapsed = initial_seconds - (current_time - start_time);
        if (elapsed < 0) elapsed = 0;
    }
    
    int hours = elapsed / 3600;
    int minutes = (elapsed % 3600) / 60;
    int seconds = elapsed % 60;
    
    int timer_col = (cols - 8) / 2;
    if (timer_col < 0) timer_col = 0;
    
    printf("\033[%d;1H", rows);
    printf("\033[2K");
    printf("\033[%dC", timer_col);
    
    int color_enabled = use_color || config.use_color;
    if (color_enabled) {
        if (timer_mode == MODE_TIMER && elapsed <= 60) {
            printf("\033[31m");
        } else if (timer_mode == MODE_TIMER && elapsed <= 300) {
            printf("\033[33m");
        } else {
            printf("\033[32m");
        }
    }
    
    printf("%02d:%02d:%02d", hours, minutes, seconds);
    
    if (color_enabled) {
        printf("\033[0m");
    }
    
    fflush(stdout);
    
    if (timer_mode == MODE_TIMER && elapsed <= 0) {
        running = 0;
    }
}

// Parse time string
int parse_time(const char *time_str) {
    int hours = 0, minutes = 0, seconds = 0;
    int count = sscanf(time_str, "%d:%d:%d", &hours, &minutes, &seconds);
    
    if (count == 1) {
        if (hours > 60) {
            minutes = hours;
            hours = 0;
        } else {
            seconds = hours;
            hours = 0;
        }
    } else if (count == 2) {
        seconds = minutes;
        minutes = hours;
        hours = 0;
    }
    
    return hours * 3600 + minutes * 60 + seconds;
}

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s <command> [OPTIONS] [arguments]\n\n", program_name);
    printf("Commands:\n");
    printf("  start [OPTIONS] \"task\"    Start a timer/display with optional logging\n");
    printf("  log [filter]              Show log (today/yesterday/YYYY-MM-DD)\n");
    printf("  stats [filter]            Show statistics\n");
    printf("  report [period]           Generate report (today/week/month)\n");
    printf("  config <action> [args]    Manage configuration\n\n");
    printf("Start Options:\n");
    printf("  -t, --timer TIME          Countdown timer (90, 1:30, 1:30:00)\n");
    printf("  -s, --stopwatch           Count up from zero (default)\n");
    printf("  -n, --no-timer            Display without timer\n");
    printf("  -l, --log                 Enable logging for this session\n");
    printf("  -c, --color               Use colored output\n");
    printf("  -f, --font FONT           Use specific figlet font\n\n");
    printf("Config Actions:\n");
    printf("  show                      Show current configuration\n");
    printf("  set <key> <value>         Set configuration value\n");
    printf("  reset                     Reset to defaults\n\n");
    printf("Examples:\n");
    printf("  %s start -t 25:00 -l \"Deep Work\"      # 25min timer with logging\n", program_name);
    printf("  %s start -l \"Writing\"                 # Stopwatch with logging\n", program_name);
    printf("  %s log today                          # Show today's log\n", program_name);
    printf("  %s log 2024-12-05                     # Show specific date\n", program_name);
    printf("  %s config set logdir ~/my-logs        # Change log directory\n", program_name);
    printf("  %s config show                        # Show configuration\n", program_name);
}

int main(int argc, char *argv[]) {
    load_config();
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    char *command = argv[1];
    
    // Handle config command
    if (strcmp(command, "config") == 0) {
        if (argc < 3) {
            printf("Error: config requires an action (show/set/reset)\n");
            return 1;
        }
        
        char *action = argv[2];
        if (strcmp(action, "show") == 0) {
            show_config();
        } else if (strcmp(action, "set") == 0) {
            if (argc < 5) {
                printf("Error: config set requires key and value\n");
                return 1;
            }
            set_config(argv[3], argv[4]);
        } else if (strcmp(action, "reset") == 0) {
            reset_config();
        } else {
            printf("Error: Unknown config action '%s'\n", action);
            return 1;
        }
        return 0;
    }
    
    // Handle log command
    if (strcmp(command, "log") == 0) {
        char *filter = argc > 2 ? argv[2] : "today";
        show_log(filter);
        return 0;
    }
    
    // Handle stats command
    if (strcmp(command, "stats") == 0) {
        char *filter = argc > 2 ? argv[2] : "month";
        show_stats(filter);
        return 0;
    }
    
    // Handle report command
    if (strcmp(command, "report") == 0) {
        char *period = argc > 2 ? argv[2] : "today";
        show_report(period);
        return 0;
    }
    
    // Handle start command
    if (strcmp(command, "start") == 0) {
        int opt;
        optind = 2; // Start parsing from argv[2]
        
        static struct option long_options[] = {
            {"timer",      required_argument, 0, 't'},
            {"stopwatch",  no_argument,       0, 's'},
            {"no-timer",   no_argument,       0, 'n'},
            {"log",        no_argument,       0, 'l'},
            {"color",      no_argument,       0, 'c'},
            {"font",       required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        
        while ((opt = getopt_long(argc, argv, "t:snlcf:", long_options, NULL)) != -1) {
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
                case 'l':
                    enable_logging = 1;
                    break;
                case 'c':
                    use_color = 1;
                    break;
                case 'f':
                    font = strdup(optarg);
                    break;
                default:
                    print_usage(argv[0]);
                    return 1;
            }
        }
        
        if (optind >= argc) {
            fprintf(stderr, "Error: No task name provided\n\n");
            print_usage(argv[0]);
            return 1;
        }
        
        // Get task name
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
        
        printf("\033[?25l");
        
        start_time = time(NULL);
        
        draw_display();
        if (timer_mode != MODE_NONE) {
            draw_timer();
        }
        
        // Main loop
        while (running) {
            if (need_redraw) {
                draw_display();
                need_redraw = 0;
            }
            
            if (timer_mode != MODE_NONE) {
                draw_timer();
            }
            
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                break;
            }
            
            usleep(100000);
        }
        
        time_t end_time = time(NULL);
        
        // Log session if enabled
        if (enable_logging) {
            Session session;
            strncpy(session.task_name, input_text, MAX_TASK_NAME - 1);
            session.start_time = start_time;
            session.end_time = end_time;
            session.duration_seconds = end_time - start_time;
            
            log_session(&session);
        }
        
        free(input_text);
        if (font != NULL) free(font);
        
        return 0;
    }
    
    printf("Error: Unknown command '%s'\n\n", command);
    print_usage(argv[0]);
    return 1;
}

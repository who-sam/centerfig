# centerfig

> A lightweight, feature-rich time tracking and focus timer for the terminal

**centerfig** is a C-based terminal application that combines beautiful ASCII art displays with powerful time tracking capabilities. Perfect for Pomodoro sessions, productivity tracking, and managing your work sessions with style.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Language](https://img.shields.io/badge/language-C-brightgreen.svg)

---

## Features

### Visual Display
- **Centered ASCII Art** - Beautiful figlet-based text centered in your terminal
- **Custom Fonts** - Support for any figlet font
- **Color Support** - Optional colored output with smart color-coding
- **Responsive** - Automatically adjusts to terminal resize

### Timer Modes
- **Countdown Timer** - Set specific time limits (e.g., 25:00 for Pomodoro)
- **Stopwatch** - Count up from zero (default mode)
- **No Timer** - Display text without any timer

### Time Tracking
- **Automatic Logging** - Track all your work sessions automatically
- **Daily Logs** - Organized by date in markdown format
- **Task Aggregation** - Combine multiple sessions for the same task
- **Rich Statistics** - View detailed stats by day, week, or month
- **Beautiful Reports** - Generate visual reports with progress bars

### Configuration
- **Customizable Paths** - Configure log directory location
- **Format Options** - Customize date and time formats
- **Persistent Settings** - Configuration saved in `~/.config/centerfig/`

---

## Installation

### Prerequisites

- **figlet** - ASCII art text generator
- **gcc** - C compiler

```bash
# Ubuntu/Debian
sudo apt-get install figlet gcc

# macOS
brew install figlet gcc

# Fedora/RHEL
sudo dnf install figlet gcc
```

### Compile from Source

```bash
# Clone or download centerfig.c
gcc -o centerfig centerfig.c

# Install system-wide (optional)
sudo mv centerfig /usr/local/bin/

# Verify installation
centerfig --help
```

---

## Usage

### Starting a Timer

```bash
# Basic stopwatch with logging
centerfig start -l "Deep Work"

# 25-minute Pomodoro timer
centerfig start -t 25:00 -l "Writing Code"

# 5-minute break
centerfig start -t 5:00 -l "Break"

# 90-second quick timer
centerfig start -t 90 -l "Stretch"

# Display without timer
centerfig start -n "Meeting Notes"

# With custom font and colors
centerfig start -t 25:00 -l -c -f slant "Focus Time"
```

**Time Format Options:**
- `90` - 90 seconds
- `5:30` - 5 minutes 30 seconds
- `1:30:00` - 1 hour 30 minutes

**Start Options:**
- `-t, --timer TIME` - Countdown timer
- `-s, --stopwatch` - Stopwatch mode (default)
- `-n, --no-timer` - No timer
- `-l, --log` - Enable logging
- `-c, --color` - Colored output
- `-f, --font FONT` - Custom figlet font

### Viewing Logs

```bash
# Today's log
centerfig log
centerfig log today

# Yesterday's log
centerfig log yesterday

# Specific date
centerfig log 2024-12-05
```

**Example Log Output:**
```markdown
# Time Log - 2024-12-06

| Task | Duration | Start Time | End Time |
|------|----------|------------|----------|
| Deep Work | 01:24:00 | 09:00:00 | 10:24:00 |
| Break | 00:15:00 | 10:24:00 | 10:39:00 |
| Writing Code | 02:15:00 | 10:45:00 | 13:00:00 |
```

### Statistics

```bash
# This month's stats (default)
centerfig stats

# This week's stats
centerfig stats week

# Today's stats
centerfig stats today

# Stats for specific task
centerfig stats "Deep Work"
```

**Example Stats Output:**
```
┌────────────────────────────────┬────────────┬──────────┬────────────┐
│ Task                           │ Total Time │ Sessions │ Avg/Session│
├────────────────────────────────┼────────────┼──────────┼────────────┤
│ Deep Work                      │ 04:30:00   │ 6        │ 00:45:00   │
│ Writing Code                   │ 03:15:00   │ 4        │ 00:48:45   │
│ Break                          │ 01:15:00   │ 8        │ 00:09:22   │
├────────────────────────────────┼────────────┼──────────┼────────────┤
│ TOTAL                          │ 09:00:00   │ 18       │            │
└────────────────────────────────┴────────────┴──────────┴────────────┘
```

### Reports

```bash
# Today's report
centerfig report today

# Weekly report
centerfig report week

# Monthly report
centerfig report month
```

**Example Report Output:**
```
╔═══════════════════════════════════════════════════════════════╗
║  TIME TRACKING REPORT - week                                  ║
║  Period: 2024-12-02 to 2024-12-06                             ║
╚═══════════════════════════════════════════════════════════════╝

 SUMMARY
───────────────────────────────────────────────────────────────
  Total Time Tracked: 9h 0m 0s
  Total Tasks: 3
  Total Sessions: 18
  Most Time Spent: Deep Work (4h 30m 0s)

 TASK BREAKDOWN
───────────────────────────────────────────────────────────────

  • Deep Work
    Time: 4h 30m 0s (50.0%)
    Sessions: 6 | Avg: 45m 0s
    [█████████████████████████░░░░░░░░░░░░░░░░░░░░░░░]

  • Writing Code
    Time: 3h 15m 0s (36.1%)
    Sessions: 4 | Avg: 48m 45s
    [██████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░]

  • Break
    Time: 1h 15m 0s (13.9%)
    Sessions: 8 | Avg: 9m 22s
    [███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░]
```

### Configuration

```bash
# Show current configuration
centerfig config show

# Set custom log directory
centerfig config set logdir ~/my-time-logs

# Enable color by default
centerfig config set color true

# Change default font
centerfig config set default_font slant

# Reset to defaults
centerfig config reset
```

**Configuration Options:**
- `logdir` - Path to log directory
- `dateformat` - Date format (strftime format)
- `timeformat` - Time format (strftime format)
- `color` - Enable color by default (true/false)
- `default_font` - Default figlet font

---

## File Structure

```
~/.config/centerfig/
├── config.ini              # Configuration file
└── logs/
    └── 2024/
        └── 12/
            ├── 2024-12-01.md
            ├── 2024-12-02.md
            └── ...
```

**Log Format:** Daily logs are stored as markdown files organized by year and month for easy browsing and version control.

---

## Use Cases

### Pomodoro Technique
```bash
# 25-minute work session
centerfig start -t 25:00 -l "Focus Work"

# 5-minute break
centerfig start -t 5:00 -l "Break"
```

### Time Tracking
```bash
# Track work on different projects
centerfig start -l "Project A - Backend"
centerfig start -l "Project B - Frontend"
centerfig start -l "Code Review"

# View daily summary
centerfig report today
```

### Meeting Timer
```bash
# 30-minute meeting with countdown
centerfig start -t 30:00 "Team Standup"
```

### Stretch Reminders
```bash
# Quick 2-minute stretch break
centerfig start -t 2:00 "Stretch Break"
```

---

## Keyboard Controls

- **Any Key** - Exit the timer and save session (if logging enabled)
- **Ctrl+C** - Exit immediately (session still saved if logging enabled)

---

## Customization Examples

### Custom Fonts

```bash
# List available fonts
figlet -l

# Use different fonts
centerfig start -f slant "Coding"
centerfig start -f banner "BREAK TIME"
centerfig start -f small "Focus"
```

### Color Themes

Timer colors automatically adjust based on time remaining:
- **Green** - Normal operation
- **Yellow** - Less than 5 minutes (countdown mode)
- **Red** - Less than 1 minute (countdown mode)

---

## 🔧 Technical Details

- **Language:** C (C99 standard)
- **Dependencies:** figlet, standard POSIX libraries
- **Terminal:** ANSI escape sequences for cursor control and colors
- **Signal Handling:** SIGWINCH for resize, SIGINT/SIGTERM for cleanup
- **Performance:** Minimal CPU usage, efficient rendering
- **Storage:** Plain markdown files for easy access and version control

---

## Command Reference

```
centerfig <command> [options] [arguments]

Commands:
  start [options] "task"    Start timer/display with optional logging
  log [filter]              Show log (today/yesterday/YYYY-MM-DD)
  stats [filter]            Show statistics (today/week/month/task)
  report [period]           Generate report (today/week/month)
  config <action> [args]    Manage configuration (show/set/reset)

Options:
  -t, --timer TIME          Countdown timer
  -s, --stopwatch           Stopwatch mode (default)
  -n, --no-timer            No timer display
  -l, --log                 Enable session logging
  -c, --color               Use colored output
  -f, --font FONT           Custom figlet font
  -h, --help                Show help message
```

---

## Contributing

Contributions are welcome! Feel free to:
- Report bugs
- Suggest new features
- Submit pull requests
- Improve documentation

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- **figlet** - For beautiful ASCII art generation
- Inspired by **tty-clock** and other terminal-based productivity tools

---

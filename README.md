# centerfig

A lightweight C program that displays ASCII art text in the center of your terminal with flexible timer modes.

## Description

`centerfig` is a C program that takes text input, converts it to ASCII art using `figlet`, centers it both horizontally and vertically in your terminal window, and optionally displays a timer. Perfect for productivity techniques like Pomodoro, break reminders, or simple message displays.

## Prerequisites

- **figlet** - ASCII art text generator
- **gcc** - C compiler (or any C compiler)

## Installation

## Installation

1. **Install dependencies:**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install figlet gcc
   
   # macOS
   brew install figlet gcc
   
   # Fedora/RHEL
   sudo dnf install figlet gcc
   ```

2. **Compile the program:**
   ```bash
   gcc -o centerfig centerfig.c
   ```

3. **Install system-wide (optional):**
   ```bash
   sudo mv centerfig /usr/local/bin/
   ```

4. **Verify installation:**
   ```bash
   centerfig --help
   ```

## Usage

```bash
centerfig [OPTIONS] [text to display]
```

### Options

- `-t, --timer TIME` - Start countdown timer from TIME (formats: 90, 1:30, 1:30:00)
- `-s, --stopwatch` - Count up from zero (default mode)
- `-n, --no-timer` - Display text without any timer
- `-c, --color` - Use colored output (cyan text, color-coded timer)
- `-f, --font FONT` - Use specific figlet font
- `-h, --help` - Display help message

### Examples

```bash
# Stopwatch mode (default)
centerfig Break Time

# 5 minute countdown timer
centerfig -t 300 Take a Break

# 25 minute Pomodoro timer
centerfig -t 25:00 Focus Time

# 1.5 hour timer
centerfig -t 1:30:00 Deep Work

# Display without timer
centerfig -n "Meeting at 3PM"

# Colored output with custom font
centerfig -c -f slant "Be Awesome"

# Quick 90 second timer
centerfig -t 90 Stretch Break
```

## Features

- **Centered Display**: Text is centered both horizontally and vertically in your terminal
- **Multiple Timer Modes**: 
  - Stopwatch: Count up from zero (default)
  - Countdown: Count down from a specified time
  - No timer: Just display the text
- **Flexible Time Format**: Support for seconds (90), MM:SS (5:30), or HH:MM:SS (1:30:00)
- **Color Support**: Optional colored output with color-coded timer warnings
- **Custom Fonts**: Support for any figlet font
- **Responsive**: Automatically adjusts to terminal resize
- **Auto-exit**: Timer mode exits automatically when countdown reaches zero
- **Clean Exit**: Press any key to exit and return to normal terminal operation
- **Cursor Management**: Hides cursor during display and restores it on exit

## How It Works

1. The program captures your input text and passes it to `figlet` to generate ASCII art
2. It calculates your terminal dimensions and centers the output dynamically
3. Depending on the mode selected, it displays a countdown timer, stopwatch, or no timer
4. The display automatically adjusts when you resize your terminal window
5. The display persists until you press any key (or timer reaches zero in countdown mode)
6. On exit, the cursor is restored and the screen is cleared

## Use Cases

- **Pomodoro Technique**: 25-minute focus timers with breaks
- **Work Break Reminders**: Set timed breaks during work sessions
- **Presentation Timers**: Countdown for presentations or talks
- **Stretch Reminders**: Quick 1-2 minute stretch breaks
- **Meeting Countdowns**: Display time until next meeting
- **Celebration Messages**: Display messages without timers
- **Meditation Timers**: Timed meditation or breathing exercises
- **Cooking Timers**: Keep track of cooking times
- **Workout Intervals**: Time your workout sets and rest periods

## Technical Details

- Written in C for performance and reliability
- Uses ANSI escape sequences for terminal control
- Handles SIGWINCH signal for terminal resize detection
- Non-blocking input for responsive keypress detection
- Efficient rendering with minimal CPU usage

## License

MIT License

# centerfig

A bash script that displays ASCII art text in the center of your terminal with a running timer.

## Description

`centerfig` takes any text input, converts it to ASCII art using `figlet`, centers it both horizontally and vertically in your terminal window, and displays a timer at the bottom showing how long the message has been displayed.

## Prerequisites

- `figlet` - ASCII art text generator
  ```bash
  # Install on Ubuntu/Debian
  sudo apt-get install figlet
  
  # Install on macOS
  brew install figlet
  
  # Install on Fedora/RHEL
  sudo dnf install figlet
  ```

## Installation

1. Download the script and save it as `centerfig`
2. Make it executable:
   ```bash
   chmod +x centerfig
   ```
3. Optionally, move it to a directory in your PATH:
   ```bash
   sudo mv centerfig /usr/local/bin/
   ```

## Usage

```bash
centerfig [text to display]
```

### Examples

```bash
# Display a simple message
centerfig Hello World

# Display multiple words
centerfig Welcome to Linux

# Display a single word
centerfig SUCCESS
```

## Features

- **Centered Display**: Text is centered both horizontally and vertically in your terminal
- **Running Timer**: Shows elapsed time in MM:SS format at the bottom center of the screen
- **Clean Exit**: Press any key to exit and return to normal terminal operation
- **Cursor Management**: Hides cursor during display and restores it on exit

## How It Works

1. The script captures your input text and passes it to `figlet` to generate ASCII art
2. It calculates your terminal dimensions and centers the output
3. A background timer process updates the elapsed time every second
4. The display persists until you press any key
5. On exit, the cursor is restored and the timer is cleaned up

## Use Cases

- Break timers during work sessions
- Presentation transitions
- Reminder displays
- Celebration messages
- Focus session indicators

## Notes

- The script will exit if run without arguments
- Terminal resizing while the script is running may affect the display
- The timer continues running until you press any key

## License

Free to use and modify.

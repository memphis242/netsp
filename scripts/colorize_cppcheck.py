import sys
import re

# Define colors
RED = "\033[1;31m"
YEL = "\033[0;33m"
BLU = "\033[1;34m"
RESET = "\033[0m"

# Combined regex to match severity levels
severity_pattern = re.compile(r"(error)|(warning)|(style|performance|information|info)")

def colorize(match):
    if match.group(1):  # Matches "error"
        return f"{RED}{match.group(1)}{RESET}"
    elif match.group(2):  # Matches "warning"
        return f"{YEL}{match.group(2)}{RESET}"
    elif match.group(3):  # Matches "style", "performance", or "information"
        return f"{BLU}{match.group(3)}{RESET}"

# Check for input
if sys.stdin.isatty() and sys.stderr.isatty():
    print("No input provided. Please pipe cppcheck output into this script.", file=sys.stderr)
    sys.exit(1)

# Process input line by line from stderr
for line in sys.stdin if not sys.stdin.isatty() else sys.stderr:
    colored_line = severity_pattern.sub(colorize, line)
    print(colored_line, end="")
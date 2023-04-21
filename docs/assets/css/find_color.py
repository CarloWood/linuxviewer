#!/usr/bin/env python3

"""
                            find_color.py

The script provides a command-line tool to perform a binary search for a line
containing a specific color code in a text file. It works by progressively
replacing color codes in the given file (i.e. a .css file) with a target color
(#ff0000) and allowing the user to provide feedback through a simple graphical
interface.

Usage:

    ./find_color.py some_file.css

Environment:

    FIND_COLOR_X_OFFSET : the window normally pops up in the center of the main
                          monitor. This can be used to shift it horizontally.

The script works as follows:

1. The script creates a backup of the file by appending .bak to the filename.
2. All color codes are replaced with the target color (#ff0000).
3. A window with two buttons (white and red) pops up, representing the users feedback.
4. The user clicks the white button if replacing the color codes had no effect, or the red button if
   it did. The script then performs a binary search based on the users input.
5. The search continues until the range narrows down to a single line or the user chooses to terminate.
6. Upon termination the file is restored to its original content.

Created by Carlo Wood and an OpenAI Assistant using the GPT-4 model.

"""

import sys
import os
import shutil
import re
import atexit
from tkinter import Tk, Button, Label

def restore_backup(backup_file, file):
    shutil.copy(backup_file, file)
    print("Backup restored.")

def replace_colors(backup_file, file, L1, L2):
    print(f"Calling replace_colors(L1={L1+1}, L2={L2+1})")

    with open(backup_file, 'r') as f:
        lines = f.readlines()

    with open(file, 'w') as f:
        for i, line in enumerate(lines):
            if L1 <= i <= L2:
                f.write(re.sub(r'#[0-9a-fA-F]{3,6}', '#ff0000', line))
            else:
                f.write(line)

window_position = None

def binary_search(file, backup_file, L1, L2, L3=None, clicked_red=False):
    found = L1 == L2

    if not found and clicked_red:
        L3 = L2
        L2 = L1 + (L2 + 1 - L1) // 2 - 1

    replace_colors(backup_file, file, L1, L2)

    def on_white_click():
        global window_position
        window_position = root.winfo_rootx() - 1, root.winfo_rooty() - 18
        root.destroy()
        if not clicked_red or found:
            print("Failure to find the right line!")
            shutil.copy(backup_file, file)
            sys.exit()
        else:
            binary_search(file, backup_file, L2 + 1, L3, clicked_red=True)

    def on_red_click():
        global window_position
        window_position = root.winfo_rootx() - 1, root.winfo_rooty() - 18
        root.destroy()
        if found or L1 == L2:
            shutil.copy(backup_file, file)
            print(f"The location is {file}:{L1 + 1}")
            sys.exit()
        else:
            binary_search(file, backup_file, L1, L2, clicked_red=True)

    root = Tk()
    root.title("Binary search")

    # Set window size.
    window_width = 400
    window_height = 200
    main_monitor_width = int(os.environ.get("FIND_COLOR_X_OFFSET", 0))

    if window_position is None:
        # Calculate the position for the first time
        screen_width = root.winfo_screenwidth()
        screen_height = root.winfo_screenheight()
        x = (screen_width // 2) - (window_width // 2) + main_monitor_width
        y = (screen_height // 2) - (window_height // 2)
    else:
        x, y = window_position

    root.geometry(f"{window_width}x{window_height}+{x}+{y}")

    # Create and configure the buttons.
    label = Label(root, text="The target color is:", font=("Helvetica", 18))
    label.pack(pady=(35, 0))
    white_button = Button(root, text="Original", command=on_white_click, bg="white", width=20, height=4)
    white_button.pack(side="left", padx=10, pady=10)
    red_button = Button(root, text="Changed", command=on_red_click, bg="red", width=20, height=4, activebackground="#ee0000")
    red_button.pack(side="right", padx=10, pady=10)

    root.mainloop()

def main():
    if len(sys.argv) != 2:
        script_name = sys.argv[0]
        print(f"Usage: python {script_name} <file>")
        sys.exit(1)

    file = sys.argv[1]
    backup_file = file + ".bak"

    if not os.path.isfile(file):
        print(f"File {file} does not exist.")
        sys.exit(1)

    shutil.copy(file, backup_file)

    atexit.register(restore_backup, backup_file, file)

    with open(file) as f:
        num_lines = sum(1 for line in f)

    try:
        binary_search(file, backup_file, 0, num_lines - 1)
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")
        sys.exit(1)

if __name__ == "__main__":
    main()


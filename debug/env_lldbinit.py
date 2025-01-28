# Filename: env_lldbinit.py
# Copyright 2024 Lukas Guz
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root or at
# http://www.apache.org/licenses/LICENSE-2.0 for full license information.

import platform
import shutil
from pathlib import Path

if platform.system() == "Windows":
    raise OSError("This script is only supported on Linux and macOS.")

DEFAULT_LLDB_FORMATTERS_PATH = Path("~/.lldb/formatters").expanduser()

formatter_script_name = "lldb_formatter_dataframe.py"
formatter_script_path = Path.cwd() / formatter_script_name

def create_backup_lldbinit(lldbinit_path):
    """Create a numbered backup for .lldbinit file if ENABLE_BACKUP is True."""

    # Find the next available backup filename
    for i in range(1, 100):  # Limit to 100 backups to avoid infinite loop
        backup_path = lldbinit_path.with_suffix(f".lldbinit.bak{i}")
        if not backup_path.exists():
            shutil.copy(lldbinit_path, backup_path)
            print(f"Backup of .lldbinit created at {backup_path}")
            break

def main():
    # ask user
    # for lldb formatters path
    target_folder_input = input(f"Enter the directory to store the LLDB formatter script (enter for default: {DEFAULT_LLDB_FORMATTERS_PATH}): ")
    target_folder = Path(target_folder_input).expanduser().resolve() if target_folder_input else DEFAULT_LLDB_FORMATTERS_PATH

    # for backup of lldbinit
    create_backup_input = input("Would you like to create backups of .lldbinit? (y/n, default: y): ").strip().lower()
    if create_backup_input not in ("yes", "y", "", "no", "n"):
        raise ValueError("Invalid input. Please enter 'yes', 'y', 'no', 'n', or press Enter for the default.")

    enable_backup = create_backup_input in ("yes", "y", "")

    # create the target folder if it doesn't exist and copy it
    target_folder.mkdir(parents=True, exist_ok=True)
    new_formatter_script_path = target_folder / formatter_script_name

    shutil.copy(formatter_script_path, new_formatter_script_path)
    print(f"Formatter script copied to {new_formatter_script_path}")

    # define the .lldbinit path and optionally create backup
    lldbinit_path = Path.home() / ".lldbinit"
    if lldbinit_path.exists() and enable_backup:
        create_backup_lldbinit(lldbinit_path)

    # define the command to import the formatter script in .lldbinit
    lldb_init_line = f'command script import "{new_formatter_script_path}"\n'

    # update .lldbinit with the formatter script path
    if lldbinit_path.exists():
        # read current .lldbinit content
        with lldbinit_path.open("r") as f:
            lines = f.readlines()

        # check if the formatter line already exists and replace it if needed
        updated = False
        for i, line in enumerate(lines):
            if "command script import" in line and formatter_script_name in line:
                lines[i] = lldb_init_line
                updated = True
                break

        if not updated:
            lines.append(lldb_init_line)

        # write back the modified .lldbinit
        with lldbinit_path.open("w") as f:
            f.writelines(lines)

        action = "Updated" if updated else "Appended"
        print(f"{action} .lldbinit with the formatter script path.")
    else:
        # create .lldbinit with the formatter script path
        with lldbinit_path.open("w") as f:
            f.write(lldb_init_line)
        print("Created .lldbinit and added the formatter script path.")


if __name__ == '__main__':

    if not formatter_script_path.exists():
        print(f"Error: The formatter script '{formatter_script_name}' does not exist in the current directory.")
        exit(1)

    main()
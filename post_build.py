import os
import subprocess
import shutil
import sys
import tkinter as tk
import threading
from datetime import datetime


arguments_string = ' '.join(sys.argv[1:])
# arguments_string = ' --no-renaming '
# arguments_string += ' --no-inlining '

# === BEGIN: Add size logging for out/leviathan-release.exe with date check ===

release_exe = os.path.join('out', 'leviathan-release.exe')
sizes_txt = 'sizes.txt'

if os.path.isfile(release_exe):
    mod_time = os.path.getmtime(release_exe)
    mod_datetime = datetime.fromtimestamp(mod_time)
    mod_str = mod_datetime.strftime('%Y-%m-%d %H:%M:%S')

    size_bytes = os.path.getsize(release_exe)

    write_entry = True

    if os.path.exists(sizes_txt):
        with open(sizes_txt, 'r') as f:
            lines = [line.strip() for line in f if line.strip()]
        if lines:
            last_line = lines[-1]
            try:
                last_date_str = last_line.split()[0] + ' ' + last_line.split()[1]
                last_date = datetime.strptime(last_date_str, '%Y-%m-%d %H:%M:%S')
                if mod_datetime <= last_date:
                    write_entry = False
            except (IndexError, ValueError):
                # malformed line in sizes.txt, proceed with write
                pass

    if write_entry:
        with open(sizes_txt, 'a') as f:
            f.write(f'{mod_str}  {size_bytes}\n')
        print(f'{mod_str}  {size_bytes} bytes written to {sizes_txt}')
    else:
        print('No update: binary not modified since last record.')
    def show_popup(message):
        def close_after_delay():
            root.after(3000, root.destroy)

        root = tk.Tk()
        root.overrideredirect(True)  # No window decorations
        root.attributes("-topmost", True)

        label = tk.Label(root, text=message, bg="black", fg="white", font=("Segoe UI", 14), padx=20, pady=10)
        label.pack()

        # Position in bottom-right corner
        root.update_idletasks()
        screen_width = root.winfo_screenwidth()
        screen_height = root.winfo_screenheight()
        window_width = root.winfo_width()
        window_height = root.winfo_height()
        x = (screen_width // 2) - (window_width // 2)
        y = (screen_height // 2) - (window_height // 2)
        root.geometry(f"{window_width}x{window_height}+{x}+{y}")

        threading.Thread(target=close_after_delay).start()
        root.mainloop()

        # Show popup only if we wrote a new size entry
    write_entry = True
    if write_entry:
        size_kb_str = f"{size_bytes} KB"
        show_popup(f"Binary size: {size_kb_str}")
# === END: Size logging ===


python
import os

# Symbol files to load (relative to GDB working directory)
symbol_files = [
    ["build/src/bootloader/stage2/boot_stage2", "0xA030"],
    # Add more here
]

def add_symbols_once(event):
    # Parse working directory from 'pwd' output (your method)
    cwd = gdb.execute('pwd', to_string=True).strip().split(" ")[2][:-1]

    # Check already loaded sources
    sources_output = gdb.execute('info sources', to_string=True)

    for rel_path in symbol_files:
        if rel_path[0] in sources_output:
            continue  # Already loaded

        full_path = os.path.join(cwd, rel_path[0])

        try:
            gdb.execute('add-symbol-file "{}" {}'.format(full_path, rel_path[1]))
        except gdb.error as e:
            gdb.write("Failed to load symbols from {} at {}: {}\n".format(full_path, rel_path[1], str(e)))

# Connect once
gdb.events.new_objfile.connect(add_symbols_once)
end

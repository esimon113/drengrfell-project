#!/usr/bin/env python3
import re
from pathlib import Path

asan_files = list(Path('.').glob('../build/asan_report*'))
output_file = "asan_direct_leaks.txt"

direct_leak_re = re.compile(r'^Direct leak of')
src_path_re = re.compile(r'drengrfell-project/src')


with open(output_file, 'w') as out_f:
    for file in asan_files:
        with open(file, 'r') as f:
            in_block = False
            leak_line = ""

            for line in f:
                line = line.rstrip()

                # Start of Direct leak block
                if direct_leak_re.match(line):
                    in_block = True
                    leak_line = line
                    src_lines = []  # lines referencing your code
                    continue

                if in_block:
                    # End of block: empty line or next Direct leak
                    if line == '' or direct_leak_re.match(line):
                        if src_lines:

                            # Only write the leak if there are lines from your code
                            out_f.write(leak_line + '\n')

                            for sl in src_lines:
                                out_f.write(sl + '\n')
                            out_f.write('\n')

                        in_block = direct_leak_re.match(line) is not None
                        if in_block:
                            leak_line = line
                            src_lines = []
                        else:
                            leak_line = ""
                            src_lines = []
                        continue

                    # Keep lines only from your source code
                    if src_path_re.search(line):
                        src_lines.append(line)

            # Flush last block if needed
            if in_block and src_lines:
                out_f.write(leak_line + '\n')

                for sl in src_lines:
                    out_f.write(sl + '\n')
                out_f.write('\n')

print(f"Filtered Direct leaks in src/ written to: {output_file}")

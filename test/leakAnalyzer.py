import re
from pathlib import Path

in_file = list(Path('.').glob('../build/asan_report*'))
if not in_file:
    raise FileNotFoundError('No file found: ../build/asan_report*')
in_file = in_file[0]

out_file = 'asan_direct_leaks.txt'

re_block_header = re.compile(r'^Direct leak of')
re_source_path = re.compile(r'drengrfell-project/src')

with open(out_file, 'w') as out:
    with open(in_file, 'r') as f:
        source_block = False
        current_block = []

        for line in f:
            if re_block_header.match(line):
                if source_block:
                    out.writelines(current_block)
                current_block.clear()
                source_block = False

            if re_source_path.search(line):
                source_block = True

            current_block.append(line)

        if source_block:
            out.writelines(current_block)

print(f'Filtered Direct Leaks in src/ folder, written to: {out_file}')

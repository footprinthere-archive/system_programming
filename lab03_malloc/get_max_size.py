from os import listdir
from os.path import isfile, join

# Get trace files
trace_dir = 'traces/'
traces = [join(trace_dir, f) for f in listdir(trace_dir) if isfile(join(trace_dir, f))]

# Read files to get the max size
max_size = 0

for f in traces:
    with open(f, 'r', encoding='utf-8') as file:
        while line := file.readline():
            tokens = line.split(' ')
            if len(tokens) < 3:
                continue

            size = int(tokens[2])
            if size > max_size:
                print(f"size = {size}")
                max_size = size

print()
print(max_size) # 614784

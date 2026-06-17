import os

GOLDEN_INDEX = 8
DIRECTORY = "./llfi/std_output"

mismatches = []
total_files = 0

for filename in os.listdir(DIRECTORY):
    path = os.path.join(DIRECTORY, filename)

    if not os.path.isfile(path):
        continue

    total_files += 1

    try:
        with open(path, "r") as f:
            last_line = f.readlines()[-1].strip()

        # Extract everything after "is:"
        values_str = last_line.split("is:", 1)[1].strip()
        values = [float(x) for x in values_str.split()]

        max_index = values.index(max(values))

        if max_index != GOLDEN_INDEX:
            mismatches.append((path, max_index))

    except Exceptpath:
        print(f"Skipping {path}: {e}")

for path, idx in mismatches:
    print(f"Different: {path}: predicted class = {idx}")

print(f"Total Number of Runs: {total_files}")
print(f"NUmber of Critical SDCs: {len(mismatches)}")


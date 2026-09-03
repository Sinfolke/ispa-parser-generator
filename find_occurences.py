import os

def find_std_variant(root_dir):
    for dirpath, _, filenames in os.walk(root_dir):
        for filename in filenames:
            if filename.endswith(('.cpp', '.cppm', '.isc')):
                file_path = os.path.join(dirpath, filename)
                try:
                    with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                        for lineno, line in enumerate(f, 1):
                            if 'cpuf::printf' in line:
                                print(f"{file_path}:{lineno}: {line.strip()}")
                except Exception as e:
                    print(f"Error reading {file_path}: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("Usage: python find_variant.py <project_root_directory>")
        sys.exit(1)

    root_directory = sys.argv[1]
    find_std_variant(root_directory)

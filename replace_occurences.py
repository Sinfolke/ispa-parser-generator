import os
import re

def replace_std_vector_in_file(filepath):

    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    # Regex pattern to match std::vector exactly (not part of bigger word)
    pattern = r'import NFA_OLD;'

    new_content, count = re.subn(pattern, 'import NFA.TNFA.API;', content)

    if count > 0:
        print(f"Replaced {count} occurrences in {filepath}")
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(new_content)

def replace_in_project(root_dir):
    for dirpath, dirnames, filenames in os.walk(root_dir):
        for filename in filenames:
            # You can restrict to certain file extensions if needed
            if filename.endswith(('.cpp', '.hpp', '.h', '.c', '.cc', '.cxx', '.cppm')):
                filepath = os.path.join(dirpath, filename)
                replace_std_vector_in_file(filepath)

if __name__ == "__main__":
    project_root = '.'  # Change if your project root is different
    replace_in_project(project_root)

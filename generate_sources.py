
import os

engine_dir = 'src/engine'
excluded_dirs = ['vendor', 'scripts', 'test', 'tests']
excluded_files = ['main.c'] # Handled by add_executable

sources = []
for root, dirs, files in os.walk(engine_dir):
    # Filter directories
    dirs[:] = [d for d in dirs if d not in excluded_dirs]
    
    for file in files:
        if file.endswith(('.c', '.m', '.mm', '.cpp')):
            if file in excluded_files:
                continue
            path = os.path.join(root, file)
            # Normalize path
            path = path.replace('\\', '/')
            sources.append(path)

sources.sort()
print("set(ENGINE_SOURCES")
for source in sources:
    print(f"    {source}")
print(")")

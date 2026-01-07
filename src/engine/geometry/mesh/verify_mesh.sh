#!/bin/bash
set -e

echo "Compiling mesh_bounds.c..."
gcc -c mesh_bounds.c -I. -I..

echo "Compiling mesh_data.c..."
gcc -c mesh_data.c -I. -I..

echo "Compiling mesh_loader.c..."
gcc -c mesh_loader.c -I. -I..

echo "Compiling mesh_optimizer.c..."
gcc -c mesh_optimizer.c -I. -I..

echo "Compiling submesh_manager.c..."
gcc -c submesh_manager.c -I. -I..

echo "Compiling mesh_gpu_data.m (syntax check only)..."
clang -c mesh_gpu_data.m -I. -I.. -fsyntax-only -framework Metal -fmodules

echo "Verification success!"

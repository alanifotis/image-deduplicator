# Image Comparison CLI Tool

A lightweight C utility to compare two images for similarity or duplication using feature matching (ORB) and transform analysis.

## Prerequisites
- A C compiler (e.g., `gcc` or MSVC `cl`)
- The project files: `main.c`, `orb_pattern.c`, `orb_pattern.h`, `stb_image.h`, and `stb_image_write.h`.

---

## Installing

```bash
git clone https://github.com/alanifotis/image-deduplicator.git
cd imgdedup
```

## Building

### Linux / WSL (`gcc`)

```bash
gcc main.c orb_pattern.c -o image_deduplicator -lm
```

### Windows (`cl`)

```powershell
cl main.c orb_pattern.c /Fe:image_deduplicator.exe
```

## Usage

### Linux / WSL

```bash
./image_deduplicator image1.png image2.png
```

### Windows (`cl`)

```powershell
image_deduplicator.exe image1.png image2.png
```

## Recommended Speed Up

# Linux / WSL (GCC / Clang)#
```bash
gcc -O3 -march=native -flto main.c orb_pattern.c -o image_deduplicator -lm
```

# Windows (MSVC)
```powershell
cl /O2 /Oi /GL main.c orb_pattern.c /Fe:image_deduplicator.exe
```
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

```cmd
cl main.c orb_pattern.c -o image_deduplicator
```

## Usage

### Linux / WSL

```bash
./image_deduplicator image1.png image2.png
```

### Windows (`cl`)

```cmd
image_deduplicator.exe image1.png image2.png
```
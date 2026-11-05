# 🖼️ C Image Deduplicator & CV Engine

> A high-performance, zero-dependency image deduplication and computer vision engine written from scratch in pure C (C99), featuring custom ORB feature matching, RANSAC homography estimation, and DCT perceptual hashing.

[![C Language](https://img.shields.io/badge/Language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## 🚀 Overview

Most image utilities rely on heavy frameworks like OpenCV or Python runtimes. **C Image Deduplicator** was built from the ground up to solve duplicate detection and image matching using low-level systems programming. 

It uses a dual-pronged approach:
1. **Global Perceptual Hashing (DCT):** Fast grid-based frequency analysis to catch resized, recompressed, or structurally similar images.
2. **Local Feature Matching (ORB + RANSAC):** Custom FAST keypoint detection, orientation-compensated BRIEF binary descriptors, Hamming distance matching, and robust homography matrix estimation via Gaussian elimination.

---

## ✨ Core Features

- **⚡ Zero Bloat, Pure C:** Built with standard C99 and single-header libraries (`stb_image`, `stb_image_write`) for minimal footprint and maximum execution speed.
- **🧠 Custom ORB Implementation:** Computes image gradients, corner scoring, patch intensity comparisons, and 256-bit binary descriptors entirely manually.
- **📐 Geometric Verification (RANSAC):** Implements robust outlier rejection using 4-point random sampling and Gaussian elimination with partial pivoting to calculate homography matrices.
- **🪞 Mirror & Transform Detection:** Capable of identifying mirrored duplicates and rotated assets.
- **💾 Custom Dynamic Arrays:** Utilizes high-performance macro-based dynamic arrays for memory efficiency.

---

## 🛠️ Tech Stack & Concepts

* **Language:** Pure C (C99)
* **Computer Vision:** Discrete Cosine Transform (DCT), FAST corner detection, BRIEF descriptors, Hamming distance metrics.
* **Linear Algebra:** Gaussian elimination with partial pivoting, homography projection, RANSAC iterative loops.
* **Systems:** Cross-platform support (POSIX `arc4random_uniform` and Windows `BCryptGenRandom`).

---

## 📦 Building & Compilation

Compile the project using any standard C compiler (e.g., `gcc` or `clang`):

```bash
# Clone the repository
git clone [https://github.com/alanifotis/image-deduplicator.git](https://github.com/alanifotis/image-deduplicator.git)
cd image-deduplicator

# Compile with optimization flags
gcc -O3 main.c -o image_deduplicator -lm

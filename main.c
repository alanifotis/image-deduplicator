#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "./orb_pattern.h"

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb_image_write.h"



#define R_C(r) ((r) * 0.299)
#define G_C(g) ((g) * 0.587)
#define B_C(b) ((b) * 0.114)
#define MATCH_THRESHOLD(x) ((x) * 0.8)
#define BLOCK_SIZE      32
#define RESIZE_SIZE     96
#define THRESHOLD       20
#define PATCH_RADIUS    15
#define ORB_PADDING     22

// all credits to @tsoding aka. the C dynamic array guy.
// https://github.com/tsoding/nob.h/blob/main/nob.h
/**
 *   MIT License Copyright (c) 2024 Alexey Kutepov
 *   ermission is hereby granted, free of charge, to any person obtaining a copy of
 *   this software and associated documentation files (the "Software"), to deal in
 *   the Software without restriction, including without limitation the rights to
 *   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *   of the Software, and to permit persons to whom the Software is furnished to do
 *   so, subject to the following conditions:
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * 
 * */
#define DECLTYPE_CAST(T)
#define REALLOC realloc
#define FREE free
#define DA_INIT_CAP 256
#define ASSERT assert

#define da_append(da, item) \
    do { \
        da_reserve((da), (da)->count + 1); \
        (da)->items[(da)->count++] = (item); \
    } while (0)

#define da_reserve(da, expected_capacity) \
    do { \
        if ((expected_capacity) > (da)->capacity) { \
            if ((da)->capacity == 0) { \
                (da)->capacity = DA_INIT_CAP; \
            } \
            while ((expected_capacity) > (da)->capacity) { \
                (da)->capacity *= 2; \
            } \
            (da)->items = DECLTYPE_CAST((da)->items)REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
            ASSERT((da)->items != NULL && "Buy more RAM lol"); \
        } \
    } while (0)
// End credits.

typedef struct {
    char *image_path;
    int width;
    int height;
    int channels;
    unsigned char *pixel_data;
} Image;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t pixel_index;
    double angle;
    uint64_t descriptor[4];
} Keypoint;

typedef struct {
    Keypoint *items;
    size_t count;
    size_t capacity;
} Keypoints;

typedef struct {
    size_t   query_idx;
    size_t   train_idx;
    uint64_t  distance;
} Match;

typedef struct {
    Match *items;
    size_t count;
    size_t capacity;
} Matches;

typedef struct {    
    int x;
    int y;
} Point;

int matrix_neighbor_check(uint64_t *hashes_a, uint64_t *hashes_b);
void resize_32(Image *image, uint64_t *hashes);
void resize_image(Image *image);
int hamming_distance(uint64_t hash1, uint64_t hash2);
void gray_scale_image(Image *image);
void is_mirrored(Image *img1, Image *img2); 
uint64_t calculate_dct(Image *image);
char *name_extension(char *filename, char *ext);
void orb_faster(Image *image, Keypoints *keypoints);
uint64_t hamming_orb(uint64_t *hash1, uint64_t *hash2);
void orb_matcher(Keypoints *k1, Keypoints *k2, Matches *matches);
void draw_matches(Image *img1, Image *img2, Keypoints *k1, Keypoints *k2, Matches *matches);
void draw_line(Image *canvas, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
void project_point(int matrix[3][3], int x, int y, Point *p);

int main(int argc, char **argv) {

    if(argc < 3) {
        fprintf(stderr, "Error: the path of images wasn't provided.\nUsage: ./main path IMAGE_1 IMAGE_2\n");
        return 1;
    }

    Image image_1 = {0};
    image_1.image_path = argv[1];
    Image image_2    = {0};
    image_2.image_path = argv[2];

    image_1.pixel_data = stbi_load(image_1.image_path, &image_1.width, &image_1.height, &image_1.channels, 3);
    image_1.channels = 3;
    image_2.pixel_data = stbi_load(image_2.image_path, &image_2.width, &image_2.height, &image_2.channels, 3);
    image_2.channels = 3;
    gray_scale_image(&image_1);
    Keypoints* k1 = calloc(1, sizeof(Keypoints));

    if (k1 == NULL) {
        fprintf(stderr, "Failed to allocate memory for K1\n");
        return 1;
    }

    orb_faster(&image_1, k1);

    gray_scale_image(&image_2);
    Keypoints* k2 = calloc(1, sizeof(Keypoints));
    if (k2 == NULL) {
    fprintf(stderr, "Failed to allocate memory for K2\n");
        return 1;
    }

    orb_faster(&image_2, k2);
    
    is_mirrored(&image_1, &image_2);

    Matches* m = calloc(1, sizeof(Matches));

    if (m == NULL) {
    fprintf(stderr, "Failed to allocate memory for Matches\n");
        return 1;
    }

    orb_matcher(k1, k2, m);

    draw_matches(&image_1, &image_2, k1, k2, m);

    printf("\x1b[1;32mORB finished succesfully\x1b[0m\n");

    if (k1->items != NULL) free(k1->items);
    if (k2->items != NULL) free(k2->items);
    if (m->items != NULL)  free(m->items);

    free(k1);
    free(k2);
    free(m);

    resize_image(&image_1); 
    resize_image(&image_2);
    uint64_t image_1_hashes[9] = {0};
    resize_32(&image_1, image_1_hashes);
    uint64_t hash_l = calculate_dct(&image_1); 
    uint64_t image_2_hashes[9] = {0};
    resize_32(&image_2, image_2_hashes);
    uint64_t hash_s = calculate_dct(&image_2);
    int distance = hamming_distance(hash_l, hash_s);
    printf("Hash image_1 = %lu\nHash Screenshot = %lu\nand the Distance = %d\n", hash_l, hash_s, distance); 
    int matrix = matrix_neighbor_check(image_1_hashes, image_2_hashes);

    printf("\x1b[1;32mFinal matrix result = %d\x1b[0m\n", matrix); 

    stbi_image_free(image_1.pixel_data);
    stbi_image_free(image_2.pixel_data);
    return 0;
}


void gray_scale_image(Image *image) {
    for (int i = 0; i < image->width * image->height; ++i) {
        int lum = (int) (R_C(image->pixel_data[i*3]) + G_C(image->pixel_data[i*3+1]) + B_C(image->pixel_data[i*3+2]));
        image->pixel_data[i*3]   = lum;
        image->pixel_data[i*3+1] = lum;
        image->pixel_data[i*3+2] = lum;
    }
    char *ext = "monochrome_";
    char *result = name_extension(image->image_path, ext);  

    stbi_write_png(result, image->width, image->height, image->channels, image->pixel_data, image->width * image->channels);
    printf("Wrote %s\n", result);
    free(result);
    result = NULL; 
}

void resize_image(Image *image) {
    int crop_size = image->width < image->height ? image->width : image->height;
    int offset_x = (image->width  - crop_size) / 2;
    int offset_y = (image->height - crop_size) / 2;

    int new_size = RESIZE_SIZE * RESIZE_SIZE * 3; 
    unsigned char *resized_image = calloc(new_size, sizeof(uint8_t));
    for(size_t x = 0; x < RESIZE_SIZE; ++x) {
        for(size_t y = 0; y < RESIZE_SIZE; ++y) {
            int old_y = offset_x + (y * crop_size) / RESIZE_SIZE;
            int old_x = offset_y + (x * crop_size) / RESIZE_SIZE;
            int old_index = (old_x * image->width + old_y) * 3;
            int new_index = (x  * RESIZE_SIZE  + y) * 3;
            resized_image[new_index] = image->pixel_data[old_index];
            resized_image[new_index + 1] = image->pixel_data[old_index + 1];
            resized_image[new_index + 2] = image->pixel_data[old_index + 2];
        }
    }


    free(image->pixel_data);

    image->pixel_data = resized_image;

    image->width  = RESIZE_SIZE;
    image->height = RESIZE_SIZE;

    char *ext = "mini_";
    char *result = name_extension(image->image_path, ext);  

    stbi_write_png(result, image->width, image->height, image->channels, image->pixel_data, image->width * image->channels);

    printf("Wrote %s\n", result);
    free(result);
    result = NULL;
}

uint64_t calculate_dct(Image *image) {
    double dct[8][8] = {0};

    for(size_t i= 0; i < 8; ++i) {

        for(size_t j = 0; j < 8; ++j) {

            double sum = 0.0;

            for(int y = 0; y < image->height; ++y) {

                for(int x = 0; x < image->width; ++x) {
                    int p_i = (y * image->width + x) * image->channels;
                    int pixel = image->pixel_data[p_i];

                    double pixel_value = (double)pixel * cos((2.0 * (double)x + 1.0) * (double)i * M_PI / (2.0 * (double)image->width)) * cos((2.0 * (double)y + 1.0) * (double)j * M_PI / (2.0 * (double)image->height));
                    sum += pixel_value ;
                } 
            }
            double C_I = i == 0 ? 1.0 / sqrt(2) : 1.0;  
            double C_J = j == 0 ? 1.0 / sqrt(2) : 1.0;  
            dct[i][j] = sum * C_I * C_J;
        }
    }

    double sum = 0.0;
    for(size_t i = 0; i < 8; ++i) {
        for(size_t j = 0; j < 8; ++j) {
            if (i == 0 && j == 0) continue;
            sum += dct[i][j];
        }
    }

    double avg = sum / 63.0;
    uint64_t hash = 0;
    for(size_t i = 0; i < 8; ++i) {
        for(size_t j = 0; j < 8; ++j) {
            hash <<= 1;
            if(dct[i][j] > avg) {
                hash |= 1;
            }
        }
    }

    return hash;
}

int hamming_distance(uint64_t hash1, uint64_t hash2) {
    uint64_t xor_result = hash1 ^ hash2;
    int count = 0;
    while(xor_result > 0 ) {
        if (xor_result & 1) count++;
        xor_result >>= 1; 
    }
    return count;
}

char *name_extension(char *filename, char *ext) {
    size_t len_o = strlen(filename);
    size_t len_a = strlen(ext);
    char *result = calloc((len_o + len_a + 1), sizeof(unsigned char));

    if(result == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }

    memcpy(result, ext, len_a);
    memcpy(result + len_a, filename, len_o + 1);

    return result;
}

void resize_32(Image *image, uint64_t *hashes) {
    int block_size = BLOCK_SIZE * BLOCK_SIZE * image->channels;

    for(size_t grid_row = 0; grid_row < 3; ++grid_row) {
        for(size_t grid_col = 0; grid_col < 3; ++grid_col) {
            unsigned char *tiny = calloc(block_size, sizeof(uint8_t));
            if (tiny == NULL) {
                fprintf(stderr, "We failed misserably to allocate the necessary memory needed for the tiny image\n");
                exit(1);
            }
            for(size_t pixel_row = 0; pixel_row < BLOCK_SIZE; ++pixel_row) {
                for(size_t pixel_col = 0; pixel_col < BLOCK_SIZE; ++pixel_col) {
                    int absolute_row = (grid_row * BLOCK_SIZE) + pixel_row;
                    int absolute_col = (grid_col * BLOCK_SIZE) + pixel_col;
                    int old_index = (absolute_row * image->width + absolute_col) * image->channels;
                    int new_index = (pixel_row  * BLOCK_SIZE  + pixel_col) * image->channels;
                    tiny[new_index] = image->pixel_data[old_index];
                    tiny[new_index + 1] = image->pixel_data[old_index + 1];
                    tiny[new_index + 2] = image->pixel_data[old_index + 2];
                }
            }
            Image tiny_image = {0};
            tiny_image.image_path = "temp_tiny_image";
            tiny_image.height = BLOCK_SIZE;
            tiny_image.width  = BLOCK_SIZE;
            tiny_image.pixel_data = tiny;
            tiny_image.channels = image->channels;

            int hash_index = grid_row * 3 + grid_col;
            hashes[hash_index] = calculate_dct(&tiny_image);

            free(tiny);
        }
    }
}


int matrix_neighbor_check(uint64_t *hashes_a, uint64_t *hashes_b) {
    int votes = 0;
    uint8_t used[9] = {0};
    for (size_t i = 0; i < 9; ++i) {
        int row_a = i / 3;
        int col_a = i % 3;
        int row_b = 0;
        int col_b = 0;
        uint8_t matched = 0;

        for (int d_row = -1; d_row < 2; ++d_row) {
            row_b = row_a + d_row;
            for (int d_col = -1; d_col < 2; ++d_col) {
                col_b = col_a + d_col;

                if (row_b < 0 || row_b > 2 || col_b < 0 || col_b > 2) continue;

                int index = row_b * 3 + col_b;

                if (used[index]) continue;

                uint64_t result = hamming_distance(hashes_a[i], hashes_b[index]);

                if (result <= 15) {
                    votes++;
                    used[index] = 1;
                    matched = 1;
                    break;
                }
            }
            if (matched) break;
        }
    }

    if (votes <= 5) {
        printf("\x1b[1;32mDTC(pHash) Result: Confidently a duplicate. Total matches: %d.\x1b[0m\n", votes);
    } else if (votes > 5 && votes <= 15) {
        printf("\x1b[1;33mDTC(pHash) Result: Likely a duplicate. Total matches: %d.\x1b[0m\n", votes);
    } else {
        printf("\x1b[1;31mDTC(pHash) Result: Likely distinct images (mirrored, rotated etc.). Total matches: %d.\x1b[0m\n", votes);
    }

    return votes;
}

void orb_faster(Image *image, Keypoints *keypoints) {
    for (int y = ORB_PADDING; y < image->height - ORB_PADDING; ++y) {
        for (int x = ORB_PADDING; x < image->width - ORB_PADDING; ++x) {
            int p_i = (y * image->width + x) * image->channels;
            int brighter = image->pixel_data[p_i] + THRESHOLD;
            int darker   = image->pixel_data[p_i] - THRESHOLD;

            int main_indexes[4] = {
                p_i - (3 * image->width * image->channels),
                p_i + (3 * image->width * image->channels),
                p_i + (3 * image->channels),
                p_i - (3 * image->channels),
            };

            size_t d_votes = 0;
            size_t b_votes = 0;

            for (size_t i = 0; i < 4; ++i) {
                int value = image->pixel_data[main_indexes[i]];
                if (value < darker) d_votes++;
                if (value > brighter) b_votes++;
            }

            if (b_votes < 3 && d_votes < 3 ) continue;

            signed char votes[25] = {0};

            for (int i = 0; i < 16 ; ++i) {
                int c_p = image->pixel_data[p_i + ((coordinates[i][1] * image->width + coordinates[i][0]) * image->channels)];
                if (c_p < darker) votes[i] = -1;
                else if (c_p > brighter) votes[i] = 1;
                else votes[i] = 0;

            }

            for (uint8_t i = 0; i < 9; ++i) {
                votes[i + 16] = votes[i];
            }

            d_votes = 0;
            b_votes = 0;

            for (int i = 0; i < 25; i++) {
                if (votes[i] == -1) { 
                    d_votes++;
                    b_votes = 0;
                } else if (votes[i] == 1) {
                    b_votes++;
                    d_votes = 0;
                } else if(!votes[i]) {
                    d_votes = 0;
                    b_votes = 0;
                }

                if (b_votes >= 9 || d_votes >= 9) {
                    Keypoint k = { x, y, p_i, 0.0, {0} };
                    da_append(keypoints, k);
                    break;
                }

                if (b_votes + (25 - i) < 9 && d_votes + (25 - i) < 9 ) break;
            }
        }
    }

    double m10 = 0.0;
    double m01 = 0.0;

    for (size_t i = 0; i < keypoints->count; ++i) {
        m10 = 0;
        m01 = 0;
        for (int dy = -PATCH_RADIUS; dy <= PATCH_RADIUS; dy++) {
            for (int dx = -PATCH_RADIUS; dx <= PATCH_RADIUS; dx++) {
                int patch_p_i = keypoints->items[i].pixel_index + ((dy * image->width + dx) * image->channels);
                int val = image->pixel_data[patch_p_i];
                m10 += dx * val;
                m01 += dy * val;
            }
        }
        keypoints->items[i].angle = atan2(m01,m10);
        double sin_theta = sin(keypoints->items[i].angle);
        double cos_theta = cos(keypoints->items[i].angle);

        for(int j = 0; j < 256; j++) {
            int x1_r = round(bit_pattern_31_[j * 4 + 0] * cos_theta - bit_pattern_31_[j * 4 + 1] * sin_theta);
            int x2_r = round(bit_pattern_31_[j * 4 + 2] * cos_theta - bit_pattern_31_[j * 4 + 3] * sin_theta);
            int y1_r = round(bit_pattern_31_[j * 4 + 0] * sin_theta + bit_pattern_31_[j * 4 + 1] * cos_theta);
            int y2_r = round(bit_pattern_31_[j * 4 + 2] * sin_theta + bit_pattern_31_[j * 4 + 3] * cos_theta);

            int p1_i = keypoints->items[i].pixel_index + ((y1_r * image->width + x1_r) * image->channels);
            int p2_i = keypoints->items[i].pixel_index + ((y2_r * image->width + x2_r) * image->channels);

            if (image->pixel_data[p1_i] < image->pixel_data[p2_i]) {
                keypoints->items[i].descriptor[j >> 6] |= (1ULL << (j & 63)); 
            }
        }
    }
    printf("Keypoints = %zu\n", keypoints->count);
}

uint64_t hamming_orb(uint64_t *hash1, uint64_t *hash2) {
    uint64_t count = 0;
    for (int i = 0; i < 4; i++) {
        uint64_t xor_result = hash1[i] ^ hash2[i];
        count += __builtin_popcountll(xor_result);
    }
    return count;
}

void orb_matcher(Keypoints *k1, Keypoints *k2, Matches *matches) {

    uint64_t lowest_distance = 300;
    uint64_t second_lowest_distance = 300;
    uint64_t test_value      = 300;
    int64_t best_j_index     =  -1;
    for (size_t i = 0; i < k1->count; ++i) {
        best_j_index = -1;
        lowest_distance = 300;
        second_lowest_distance = 300;
        test_value = 300;
 
        for (size_t j = 0; j < k2->count; ++j) {
            test_value = hamming_orb(k1->items[i].descriptor, k2->items[j].descriptor);

            if (test_value < lowest_distance) {
                second_lowest_distance = lowest_distance;
                lowest_distance = test_value;
                best_j_index = j;
            } else if (test_value < second_lowest_distance) {
                second_lowest_distance = test_value;
            }
        }

        if (lowest_distance < MATCH_THRESHOLD(second_lowest_distance)) {
            Match m = { i, best_j_index, lowest_distance };
            da_append(matches, m);
        }
   }

    if (matches->count >= 500) {
        printf("\x1b[1;32mORB Result: Confidently a duplicate. Total matches: %zu.\x1b[0m\n", matches->count);
    } else if (matches->count < 500 && matches->count > 100) {
        printf("\x1b[1;33mORB Result: Likely a duplicate. Total matches: %zu.\x1b[0m\n", matches->count);
    } else {
        printf("\x1b[1;31mORB Result: Likely distinct images (or mirrored). Total matches: %zu.\x1b[0m\n", matches->count);
    }
}

void draw_matches(Image *img1, Image *img2, Keypoints *k1, Keypoints *k2, Matches *matches) {
    Image canvas = {0};
    canvas.image_path = "canvas.png";
    canvas.width  = img1->width + img2->width;
    canvas.height = img1->height > img2->height ? img1->height : img2->height;
    canvas.channels = 3;

    canvas.pixel_data = calloc((canvas.height * canvas.width * canvas.channels), sizeof(unsigned char));

    if (canvas.pixel_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for the image buffer\n");
        exit(1);
    }
    for (int y = 0; y < img1->height; y++) {
        for (int x = 0; x < img1->width; x++) {
            int src    = (y * img1->width + x) * img1->channels;
            int target = (y * canvas.width + x) * canvas.channels;
            canvas.pixel_data[target]     = img1->pixel_data[src];
            canvas.pixel_data[target + 1] = img1->pixel_data[src + 1];
            canvas.pixel_data[target + 2] = img1->pixel_data[src + 2];
            }
    }
    
    for (int y = 0; y < img2->height; y++) {
        for (int x = 0; x < img2->width; x++) {
            int src    = (y * img2->width + x) * img2->channels;
            int target = (y * canvas.width + (x + img1->width)) * canvas.channels;
            canvas.pixel_data[target]     = img2->pixel_data[src];
            canvas.pixel_data[target + 1] = img2->pixel_data[src + 1];
            canvas.pixel_data[target + 2] = img2->pixel_data[src + 2];
        }
    }

    for(size_t i = 0; i < matches->count; i++) {
        int x0 = k1->items[matches->items[i].query_idx].x;
        int y0 = k1->items[matches->items[i].query_idx].y;
        int x1 = k2->items[matches->items[i].train_idx].x + img1->width;
        int y1 = k2->items[matches->items[i].train_idx].y;
        draw_line(&canvas, x0, y0, x1, y1, 0, 255, 0);
    }

    stbi_write_png(canvas.image_path, canvas.width, canvas.height, canvas.channels, canvas.pixel_data, canvas.width * canvas.channels);

    free(canvas.pixel_data);
}

void draw_line(Image *canvas, int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
    int dx =  abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while(1) {
        if (x0 >= 0 && x0 < canvas->width && y0 >= 0 && y0 < canvas->height
        ) {
            int target = (y0 * canvas->width + x0) * canvas->channels;
            canvas->pixel_data[target]     = r;
            canvas->pixel_data[target + 1] = g;
            canvas->pixel_data[target + 2] = b;
        }

        if (x0 == x1 && y0 == y1) break;

        int err2 = 2 * err;

        if (err2 >= dy ) {
            err += dy;
            x0  += sx;
        }

        if (err2 <= dx) {
            err += dx;
            y0  += sy;
        }
    }
}

void is_mirrored(Image *img1, Image *img2) {
    if (img1->width != img2->width || img1->height != img2->height) {
        printf("\x1b[1;31mProbably not a mirrored duplicate as dimensions do not match!\x1b[0m\n");
        return;
    }

    uint64_t errors = 0;

    for (int y = 0; y < img1->height; y++) {
        for (int x = 0; x < img1->width; x++) {
            int i1 = (y * img1->width + x) * img1->channels;
            int i2 = (y * img2->width + (img2->width - 1 - x)) * img2->channels;
            errors += abs(img1->pixel_data[i1] - img2->pixel_data[i2]);
        }
    }

    double mean_abs = (double)errors / (img1->width * img1->height);

    if (mean_abs < 5.0) {
        printf("\x1b[1;32mMirror Result: Confidently a mirrored duplicate!\x1b[0m\n");
    } else {
        printf("\x1b[1;32mMirror Result: Most likely not a mirror!\x1b[0m\n");
    }

    return;
}

void project_point(int matrix[3][3], int x, int y, Point *p) {
    int new_x = (matrix[0][0] * x) + (matrix[0][1] * y) + matrix[0][2];
    int new_y = (matrix[1][0] * x) + (matrix[1][1] * y) + matrix[1][2];
    int w     = (matrix[2][0] * x) + (matrix[2][1] * y) + matrix[2][2];
    p->x = (int)(new_x / w);
    p->y = (int)(new_y / w);
}

uint32_t *unique_samples(uint32_t max_index) {
    uint32_t* ind = calloc(4, sizeof(uint32_t));

    uint8_t count = 0;


    while(count < 4) {
        uint32_t n = 0;
        n += arc4random_uniform(max_index);
        for (uint8_t i = 0; i < 4; i++) {
            if (ind[i] != n ) {
                ind[count] = n;
                count++;
            }
        }
    }

    return ind;
}

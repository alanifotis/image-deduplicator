#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb_image_write.h"

#define IMAGE_PATH "./fk.jpg"
#define OUTPUT_PATH "./test.jpg"
#define R_C(r) ((r) * 0.299)
#define BLOCK_SIZE  32
#define RESIZE_SIZE 96
#define G_C(g) ((g) * 0.587)
#define B_C(b) ((b) * 0.114)
#define IMAGE_2 "./me_s.jpg"
#define IMAGE_1 "./me.jpg"




typedef struct {
    char *image_path;
    int width;
    int height;
    int channels;
    unsigned char *pixel_data;
} Image;

int matrix_neighbor_check(uint64_t *hashes_a, uint64_t *hashes_b);
void resize_32(Image *image, uint64_t *hashes);
void resize_image(Image *image);
int hamming_distance(uint64_t hash1, uint64_t hash2);
void gray_scale_image(Image *image);
uint64_t calculate_dct(Image *image);
char *name_extension(char *filename, char *ext);

int main(int argc, char **argv) {

    if(argc < 3) {
        fprintf(stderr, "Error: the path of images wasn't provided.\nUsage: ./main path IMAGE_1 IMAGE_2\n");
        return 1;
    }

    Image image = {0};
    image.image_path = IMAGE_PATH;

    image.pixel_data = stbi_load(IMAGE_PATH, &image.width, &image.height, &image.channels, 3);

    if (image.pixel_data == NULL) {
        fprintf(stderr,"Error: Unable to load %s\n", IMAGE_PATH);
        return 1;
    }

    Image london = {0};
    london.image_path = argv[1];
    Image ssh    = {0};
    ssh.image_path = argv[2];

    london.pixel_data = stbi_load(london.image_path, &london.width, &london.height, &london.channels, 3);
    london.channels = 3;
    ssh.pixel_data = stbi_load(ssh.image_path, &ssh.width, &ssh.height, &ssh.channels, 3);
    ssh.channels = 3;
    gray_scale_image(&image);
    gray_scale_image(&london);
    gray_scale_image(&ssh);
    resize_image(&image); 
    resize_image(&london); 
    resize_image(&ssh);
    uint64_t london_hashes[9] = {0};
    resize_32(&london, london_hashes);
    uint64_t hash_l = calculate_dct(&london); 
    uint64_t ssh_hashes[9] = {0};
    resize_32(&ssh, ssh_hashes);
    uint64_t hash_s = calculate_dct(&ssh);
    int distance = hamming_distance(hash_l, hash_s);
    printf("Hash london = %lu\nHash Screenshot = %lu\nand the Distance = %d\n", hash_l, hash_s, distance); 
    int matrix = matrix_neighbor_check(london_hashes, ssh_hashes);


    printf("\x1b[1;32mFinal matrix result = %d\x1b[0m\n", matrix); 

    stbi_image_free(image.pixel_data);
    stbi_image_free(london.pixel_data);
    stbi_image_free(ssh.pixel_data);
    return 0;
}


void gray_scale_image(Image *image) {
    for (size_t i = 0; i < image->width * image->height; ++i) {
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

            for(size_t y = 0; y < image->height; ++y) {

                for(size_t x = 0; x < image->width; ++x) {
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

    printf("Super hash = %lu\n", hash);

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

                printf("The result for i = %d is: %ul\n", i, result);

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
    return votes;
}

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

void resize_image(Image *image);
int hamming_distance(uint64_t hash1, uint64_t hash2);
void gray_scale_image(Image *image);
uint64_t calculate_dct(Image *image);
char *name_extension(char *filename, char *ext);

int main(int argc, char **argv) {

    if(argc < 3) {
        printf("\x1b[1;31mError: the path of images wasn't provided.\nUsage: ./main path IMAGE_1 IMAGE_2  \x1b[0m\n");
        return 1;
    }

    printf("argc = %d\n", argc);

    Image image = {0};
    image.image_path = IMAGE_PATH;

    image.pixel_data = stbi_load(IMAGE_PATH, &image.width, &image.height, &image.channels, 3);

    if (image.pixel_data == NULL) {
        printf("\x1b[1;31mError: Unable to load %s\x1b[0m\n", IMAGE_PATH);
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
    uint64_t hash_l= calculate_dct(&london); 
    uint64_t hash_s = calculate_dct(&ssh);
    int distance = hamming_distance(hash_l, hash_s);
    printf("Hash london = %lu\nHash Screenshot = %lu\nand the Distance = %d\n", hash_l, hash_s, distance); 

    stbi_image_free(image.pixel_data);
    stbi_image_free(london.pixel_data);
    stbi_image_free(ssh.pixel_data);
    printf("Height=%d\tWidth=%d\n", image.height, image.width);
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
    int new_size = 32 * 32 * 3; 
    unsigned char *resized_image = calloc(new_size, sizeof(uint8_t));
    for(size_t x = 0; x < 32; ++x) {
        for(size_t y = 0; y < 32; ++y) {
            int old_y = (y * image->width) / 32;
            int old_x = (x * image->height) / 32;
            int old_index = (old_x * image->width + old_y) * 3;
            int new_index = (x  * 32  + y) * 3;
            resized_image[new_index] = image->pixel_data[old_index];
            resized_image[new_index + 1] = image->pixel_data[old_index + 1];
            resized_image[new_index + 2] = image->pixel_data[old_index + 2];
        }
    }


    free(image->pixel_data);

    image->pixel_data = resized_image;

    image->width  = 32;
    image->height = 32;

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

            for(size_t y = 0; y < 32; ++y) {

                for(size_t x = 0; x < 32; ++x) {
                    int p_i = (x * image->width + y) * image->channels;
                    int pixel = image->pixel_data[p_i];

                    double pixel_value = pixel * cos((2 * x + 1) * i * M_PI / (2.0 * 32.0)) * cos((2 * y + 1) * j * M_PI / (2.0 * 32.0));
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

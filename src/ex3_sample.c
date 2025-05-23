#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>

#define IMAGE_WIDTH  32
#define IMAGE_HEIGHT 32

// 生成随机灰度图像矩阵
void generate_random_gray_matrix(unsigned char matrix[IMAGE_HEIGHT][IMAGE_WIDTH]) {
    int x, y;
    srand(rt_tick_get());
    for (y = 0; y < IMAGE_HEIGHT; y++) {
        for (x = 0; x < IMAGE_WIDTH; x++) {
            matrix[y][x] = (unsigned char)(rand() % 256);
        }
    }
}

// 简单的游程编码压缩函数
void compress_image(unsigned char *input, unsigned char *output, int *output_size) {
    int input_index = 0;
    int output_index = 0;
    int total_pixels = IMAGE_WIDTH * IMAGE_HEIGHT;

    while (input_index < total_pixels) {
        unsigned char current_pixel = input[input_index];
        int count = 1;
        input_index++;

        while (input_index < total_pixels && input[input_index] == current_pixel && count < 255) {
            count++;
            input_index++;
        }

        output[output_index++] = count;
        output[output_index++] = current_pixel;
    }

    *output_size = output_index;
}

// 简单的游程编码解压函数
void decompress_image(unsigned char *input, int input_size, unsigned char *output) {
    int input_index = 0;
    int output_index = 0;

    while (input_index < input_size) {
        unsigned char count = input[input_index++];
        unsigned char pixel = input[input_index++];

        for (int i = 0; i < count; i++) {
            output[output_index++] = pixel;
        }
    }
}

// 打印图像矩阵
void print_image_matrix(unsigned char matrix[IMAGE_HEIGHT][IMAGE_WIDTH]) {
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            rt_kprintf("%3d ", matrix[y][x]);
        }
        rt_kprintf("\n");
    }
}

// 打印压缩后的数据
void print_compressed_data(unsigned char *compressed, int compressed_size) {
    for (int i = 0; i < compressed_size; i += 2) {
        rt_kprintf("Count: %3d, Pixel: %3d\n", compressed[i], compressed[i + 1]);
    }
}

// 线程入口函数
void ex3_sample(void *parameter) {
    unsigned char original_image[IMAGE_HEIGHT][IMAGE_WIDTH];
    generate_random_gray_matrix(original_image);

    // 转换为一维数组
    unsigned char original_1d[IMAGE_WIDTH * IMAGE_HEIGHT];
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            original_1d[y * IMAGE_WIDTH + x] = original_image[y][x];
        }
    }

    // 打印压缩前的图像矩阵
    rt_kprintf("压缩前的图像矩阵\n");
    print_image_matrix(original_image);

    // 分配压缩后图像的内存
    unsigned char compressed_image[IMAGE_WIDTH * IMAGE_HEIGHT * 2];
    int compressed_size;

    // 压缩图像
    compress_image(original_1d, compressed_image, &compressed_size);
    rt_kprintf("图像压缩完成，压缩后大小: %d 字节\n", compressed_size);

    // 打印压缩后的数据
    rt_kprintf("压缩后的数据\n");
    print_compressed_data(compressed_image, compressed_size);

    // 分配解压后图像的内存
    unsigned char decompressed_1d[IMAGE_WIDTH * IMAGE_HEIGHT];

    // 解压图像
    decompress_image(compressed_image, compressed_size, decompressed_1d);
    rt_kprintf("图像解压完成\n");

    // 将一维解压数组转换为二维矩阵
    unsigned char decompressed_image[IMAGE_HEIGHT][IMAGE_WIDTH];
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            decompressed_image[y][x] = decompressed_1d[y * IMAGE_WIDTH + x];
        }
    }

    // 打印解压后的图像矩阵
    rt_kprintf("解压后的图像矩阵\n");
    print_image_matrix(decompressed_image);

}

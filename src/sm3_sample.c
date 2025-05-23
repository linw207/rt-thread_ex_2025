#include <rtthread.h>
#include <stdio.h>
#include <string.h>

// SM3 常量定义
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define P0(x) ((x) ^ ROTL((x), 9) ^ ROTL((x), 17))
#define P1(x) ((x) ^ ROTL((x), 15) ^ ROTL((x), 23))

#define FF0(x, y, z) ((x) ^ (y) ^ (z))
#define FF1(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))

#define GG0(x, y, z) ((x) ^ (y) ^ (z))
#define GG1(x, y, z) (((x) & (y)) | ((~(x)) & (z)))

#define T0 0x79cc4519
#define T1 0x7a879d8a

// SM3 初始化常量
static const uint32_t IV[8] = {
    0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
    0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
};

// SM3 压缩函数
static void sm3_compress(uint32_t *v, const uint32_t *block) {
    uint32_t W[68], W1[64];
    uint32_t A, B, C, D, E, F, G, H;
    uint32_t SS1, SS2, TT1, TT2;
    int j;

    // 消息扩展
    for (j = 0; j < 16; j++) {
        W[j] = block[j];
    }
    for (j = 16; j < 68; j++) {
        W[j] = P1(W[j - 16] ^ W[j - 9] ^ ROTL(W[j - 3], 15)) ^ ROTL(W[j - 13], 7) ^ W[j - 6];
    }
    for (j = 0; j < 64; j++) {
        W1[j] = W[j] ^ W[j + 4];
    }

    // 初始化寄存器
    A = v[0];
    B = v[1];
    C = v[2];
    D = v[3];
    E = v[4];
    F = v[5];
    G = v[6];
    H = v[7];

    // 压缩
    for (j = 0; j < 64; j++) {
        uint32_t T = (j < 16) ? T0 : T1;
        SS1 = ROTL((ROTL(A, 12) + E + ROTL(T, j % 32)), 7);
        SS2 = SS1 ^ ROTL(A, 12);
        TT1 = ((j < 16) ? FF0(A, B, C) : FF1(A, B, C)) + D + SS2 + W1[j];
        TT2 = ((j < 16) ? GG0(E, F, G) : GG1(E, F, G)) + H + SS1 + W[j];
        D = C;
        C = ROTL(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = ROTL(F, 19);
        F = E;
        E = P0(TT2);
    }

    // 更新寄存器
    v[0] ^= A;
    v[1] ^= B;
    v[2] ^= C;
    v[3] ^= D;
    v[4] ^= E;
    v[5] ^= F;
    v[6] ^= G;
    v[7] ^= H;
}

// SM3 加密函数
void sm3(const uint8_t *message, size_t len, uint8_t digest[32]) {
    uint32_t v[8];
    uint32_t block[16];
    size_t i, j;
    size_t remainder = len % 64;
    size_t padding_len = (remainder < 56) ? (56 - remainder) : (120 - remainder);
    size_t padded_len = len + padding_len + 8;
    uint64_t bit_len = (uint64_t)len * 8;

    // 初始化寄存器
    for (i = 0; i < 8; i++) {
        v[i] = IV[i];
    }

    // 处理消息块
    for (i = 0; i < len / 64; i++) {
        for (j = 0; j < 16; j++) {
            block[j] = (message[i * 64 + j * 4] << 24) |
                       (message[i * 64 + j * 4 + 1] << 16) |
                       (message[i * 64 + j * 4 + 2] << 8) |
                       (message[i * 64 + j * 4 + 3]);
        }
        sm3_compress(v, block);
    }

    // 填充消息
    uint8_t *padded_message = (uint8_t *)rt_malloc(padded_len);
    if (padded_message == NULL) {
        return;
    }
    memcpy(padded_message, message, len);
    padded_message[len] = 0x80;
    for (i = len + 1; i < len + padding_len; i++) {
        padded_message[i] = 0;
    }
    for (i = 0; i < 8; i++) {
        padded_message[len + padding_len + i] = (bit_len >> ((7 - i) * 8)) & 0xff;
    }

    // 处理填充后的消息块
    for (i = 0; i < padded_len / 64; i++) {
        for (j = 0; j < 16; j++) {
            block[j] = (padded_message[i * 64 + j * 4] << 24) |
                       (padded_message[i * 64 + j * 4 + 1] << 16) |
                       (padded_message[i * 64 + j * 4 + 2] << 8) |
                       (padded_message[i * 64 + j * 4 + 3]);
        }
        sm3_compress(v, block);
    }

    // 输出摘要
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 4; j++) {
            digest[i * 4 + j] = (v[i] >> ((3 - j) * 8)) & 0xff;
        }
    }

    rt_free(padded_message);
}

// SM3 示例函数，作为线程入口
void sm3_sample(void *parameter) {
    const uint8_t message[] = "!Hello, SM3";
    rt_kprintf("message: !Hello, SM3");
    uint8_t digest[32];
    int i;

    sm3(message, strlen((const char *)message), digest);

    rt_kprintf("SM3 digest: ");
    for (i = 0; i < 32; i++) {
        rt_kprintf("%02x", digest[i]);
    }
    rt_kprintf("\n");
}

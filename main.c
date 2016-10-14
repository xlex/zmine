// Copyright Douglas Goddard 2016
// Licensed under the TGPPL

#define _BSD_SOURCE
#include <endian.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "csolv.h"

// random data
uint8_t crash_case[140] = { \
    0x79, 0xf8, 0x1e, 0xf3, 0x1c, 0x4d, 0x4c, 0x46, 0xa4, 0xb3, 0x65, 0x1b, 0x5b, 0x47, \
    0xf7, 0x2c, 0x49, 0x98, 0x51, 0x0c, 0x91, 0xe1, 0x02, 0x5a, 0x7c, 0x87, 0x3d, 0x7e, \
    0x9e, 0x15, 0x97, 0x99, 0x79, 0x82, 0xd7, 0x0d, 0x1f, 0xa4, 0x0b, 0x9e, 0x2f, 0x2f, \
    0x9c, 0xa9, 0xf0, 0x18, 0x00, 0xdb, 0xd0, 0x02, 0xee, 0x7b, 0x75, 0x94, 0xbc, 0xc1, \
    0xf8, 0xa2, 0x12, 0x32, 0x00, 0x8d, 0xdc, 0xff, 0xcd, 0x89, 0x4d, 0x32, 0x0b, 0x6f, \
    0x4f, 0x48, 0xd9, 0xba, 0xec, 0x4c, 0xec, 0xa3, 0xbe, 0x38, 0x1e, 0x32, 0xdf, 0x98, \
    0x5d, 0xc9, 0xdb, 0x8c, 0x10, 0x21, 0x05, 0xa4, 0x53, 0xa7, 0x2c, 0x19, 0x48, 0xa7, \
    0xc7, 0x78, 0xb9, 0xc3, 0xc8, 0x78, 0x91, 0x95, 0x65, 0x0f, 0x82, 0x3c, 0x2d, 0x99, \
    0x75, 0xc8, 0xfc, 0xd1, 0xd2, 0xde, 0x6c, 0xbd, 0x34, 0x29, 0x09, 0x5a, 0xa2, 0x9f, \
    0xda, 0x00, 0x28, 0x85, 0x4f, 0x17, 0x43, 0x9e, 0x85, 0x28, 0x88, 0xa2, 0xf4, 0x78 };

uint8_t random_header[140] = { \
    0x79, 0xf8, 0x1e, 0xf3, 0x1c, 0x4d, 0x4c, 0x46, 0xa4, 0xb3, 0x65, 0x1b, 0x5b, 0x47, \
    0xf7, 0x2c, 0x49, 0x98, 0x51, 0x0c, 0x91, 0xe1, 0x12, 0x5a, 0x7c, 0x87, 0x3d, 0x7e, \
    0x9e, 0x15, 0x97, 0x99, 0x79, 0x82, 0xd7, 0x0d, 0x1f, 0xa4, 0x1b, 0x9e, 0x2f, 0x2f, \
    0x9c, 0xa9, 0xf0, 0x18, 0x00, 0xdb, 0xd0, 0x02, 0xef, 0x7b, 0x75, 0x94, 0xbc, 0xc1, \
    0xf8, 0xa2, 0x12, 0x32, 0x00, 0x8d, 0xdc, 0xff, 0xcd, 0x89, 0x4d, 0x32, 0x0b, 0x6f, \
    0x4f, 0x48, 0xd9, 0xba, 0xec, 0x4c, 0xec, 0xa3, 0xbe, 0x38, 0x1e, 0x32, 0xdf, 0x98, \
    0x5d, 0xc9, 0xdb, 0x8c, 0x10, 0x21, 0x05, 0xa4, 0x53, 0xa7, 0x2c, 0x19, 0x48, 0xa7, \
    0xc7, 0x78, 0xb9, 0xc3, 0xc8, 0x78, 0x91, 0x95, 0x65, 0x0f, 0x82, 0x3c, 0x2d, 0x99, \
    0x75, 0xc8, 0xfc, 0xd1, 0xd2, 0xde, 0x6c, 0xbd, 0x34, 0x29, 0x09, 0x5a, 0xa2, 0x9f, \
    0xda, 0x00, 0x28, 0x85, 0x4f, 0x17, 0x43, 0x9e, 0x85, 0x28, 0x88, 0xa2, 0xf4, 0x78 };

int init(CUDA_BLAKE2B_STATE *state) {
    unsigned char personal[crypto_generichash_blake2b_PERSONALBYTES] = {};
    uint32_t le_N = htole32(CUDA_N);
    uint32_t le_K = htole32(CUDA_K);
    memcpy(personal, "ZcashPoW", 8);
    memcpy(personal, &le_N, 4);
    memcpy(personal, &le_K, 4);
    return crypto_generichash_blake2b_init_salt_personal(
            state, 
            NULL, 
            0,
            (512/CUDA_N) * CUDA_N/8,
            NULL,
            personal);
}

void generate_hash(CUDA_BLAKE2B_STATE *base_state, uint32_t index, unsigned char *hash, size_t hash_len) {
    CUDA_BLAKE2B_STATE state = *base_state;
    uint32_t le_index = htole32(index);
    crypto_generichash_blake2b_update(&state, (const unsigned char*)&le_index, 4);
    crypto_generichash_blake2b_final(&state, hash, hash_len);
}

#define NRUNS 5

int main(int argc, char **argv) {
    if(sodium_init() == -1)
        return 1;

    CUDA_BLAKE2B_STATE state;
    init(&state);

    crypto_generichash_blake2b_update(&state, (unsigned char*)&random_header[0], 140);

    uint32_t indices_per_hash = 512/CUDA_N;
    uint32_t cbit_len = CUDA_N/(CUDA_K+1);
    uint32_t hash_len = (indices_per_hash)*CUDA_N/8;
    
    uint32_t hash_count = (1 << (cbit_len + 1)) / indices_per_hash;
    uint32_t out_size = hash_len * hash_count * sizeof(unsigned char);

    printf("INDICES_PER: %u\n", 512/CUDA_N);
    printf("HASH_LEN: %u\n", hash_len);
    printf("CBIT_LEN: %u\n", cbit_len);

    printf("Generating %u hashes...\n", hash_count);

    clock_t begin, end;   
    double time_spent;

    // generate 1m hashes
    unsigned char *fast_hashes;
    fast_hashes = (unsigned char *)malloc(out_size);
    cuda_generate_hash(&state, hash_count, fast_hashes);

    // for(int i=0; i<NRUNS; i++) {
    //     begin = clock();

    //     end = clock();
    //     time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    //     printf("GPU Hash R%d:\t%f seconds\n", i, time_spent);
    //     if(i != (NRUNS-1))
    //         free(fast_hashes);
    // }

    // expand hashes 
    uint32_t expanded_size = out_size*6/5;
    unsigned char *expanded_hashes = (unsigned char *)malloc(expanded_size);
    printf("Expanded size * 5: %u\n", out_size*6);
    printf("Expanded size: %u\n", expanded_size);
    cuda_solve_hashes(fast_hashes, out_size, expanded_hashes, expanded_size);

    // spot check expanded hashes
    for(int index=0; index<50; index++) {
        printf("%02x ", fast_hashes[index]);
        if(index == 24)
            printf("\n");
    }
    printf("\n");
    
    for(int index=0; index<60; index++) {
        printf("%02x ", expanded_hashes[index]);
        if(index == 29)
            printf("\n");
    }
    printf("\n");
    free(fast_hashes);

    
    

    free(expanded_hashes);
    return 0;
}

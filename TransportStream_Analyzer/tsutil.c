//
//  tsutil.c
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 16..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#include "tsutil.h"

void print_binary(char *c, int count) {
    // 10000000 01000000 00100000 00010000 00001000 00000100 00000010 00000001
    for(int j = 0; j < count; j++) {
        for(int i = 0; i < 8; i++) {
            printf("%d", (c[j] >> (7-i)) & 0x01);
            if (i % 8 == 7) {
                printf(" ");
            }
        }
    }
    printf("\n");
}

void print_binary_int(uint32_t num) {
    // 10000000 01000000 00100000 00010000 00001000 00000100 00000010 00000001
    for(int i = 0; i < 32; i++) {
        printf("%d", (num >> (31-i)) & 0x01);
        if (i % 8 == 7) {
            printf(" ");
        }
    }
    printf("\n");
}
void print_binary_int64(uint64_t num) {
    // 10000000 01000000 00100000 00010000 00001000 00000100 00000010 00000001
    for(int i = 0; i < 64; i++) {
        printf("%llu", (num >> (63-i)) & 0x01);
        if (i % 8 == 7) {
            printf(" ");
        }
    }
    printf("\n");
}

uint8_t readSmallData(DataReader *reader, int length) {
    int index = reader->offset / 8;
    int loc = reader->offset % 8;
    
    //    printf("offset: %d, length: %d, index: %d, loc: %d\n", reader->offset, length, index, loc);
    uint8_t result;
    if (loc + length > 8) {
        // 두 index에 걸쳐 있음
        uint8_t r1 = readSmallData(reader, 8 - loc);
        uint8_t r2 = readSmallData(reader, length - (8 - loc));
        
        result = (r1 << (8-loc)) | r2;
    } else {
        result = (reader->data[index] >> (8 - loc - length)) & ((1 << length) - 1);
    }
    reader->offset += length;
    return result;
}

uint32_t readData(DataReader *reader, int length) {
    uint32_t result = 0;
    //    printf("reader->offset: %d, length: %d\n", reader->offset, length);
    while (length > 0) {
        uint8_t step = ((length - 1) % 8) + 1;
        // 처음에만 8 미만의 숫자가 나오고, 이후에는 8
        result = (result << 8) | readSmallData(reader, step);
        //        print_binary_int(result);
        length = length - step;
    }
    
    return result;
}

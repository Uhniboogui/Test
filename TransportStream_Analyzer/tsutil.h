//
//  tsutil.h
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 16..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#ifndef tsutil_h
#define tsutil_h

#include <stdio.h>

#endif /* tsutil_h */

void print_binary(char *c, int count);
void print_binary_int(uint32_t num);
void print_binary_int64(uint64_t num);

typedef struct {
    char *data;
    int offset;
} DataReader;

uint8_t readSmallData(DataReader *reader, int length);
uint32_t readData(DataReader *reader, int length);

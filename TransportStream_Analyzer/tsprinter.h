//
//  tsprinter.h
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 16..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#ifndef tsprinter_h
#define tsprinter_h

#include <stdio.h>
#include "tsmodel.h"

#endif /* tsprinter_h */

void print_pid_info(int pid);
void print_program_association_table(ProgramAssociationTable *info);
void print_stream_type(uint8_t streamType);
void print_descriptor(DescriptionTable *info);
void print_program_element_stream_info(ElementStreamInfo *info);
void print_program_map_table(ProgramMapTable *info);

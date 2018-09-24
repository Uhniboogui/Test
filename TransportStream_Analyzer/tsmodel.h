//
//  tsmodel.h
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 16..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#ifndef tsmodel_h
#define tsmodel_h

#include <stdio.h>

#endif /* tsmodel_h */

typedef struct {
    int programNumber;                    // 16 bits
    char reserved;                        // 3 bits
    int pid;                              // 13 bits
} ProgramNumberPIDInfo;

typedef struct {
    char tableID;                         // 8 bits
    char sectionSyntaxIndicator;        // 1 bit
    char privateBit;                    // 1 bit, must be 0
    char reserved1;                        // 2 bits, must be 0b11
    int sectionLength;                    // 12 bits
    int transportStreamID;                // 16 bits
    char reserved2;                        // 2 bits, must be 0b11
    char versionNumber;                    // 5 bits
    char currentNextIndicator;            // 1 bit
    char sectionNumber;                    // 8 bits
    char lastSectionNumber;                // 8 bits
    int programInfoCount;
    ProgramNumberPIDInfo** infos;
    
    int32_t crc32;                         // 32 bits
} ProgramAssociationTable;

typedef struct {
    
} VideoStreamDescriptorInfo;

typedef struct {
    uint8_t descriptorTag;
    uint8_t descriptorLength;
    void* info;
} DescriptionTable;

typedef struct {
    char streamType;                    // 8 bits
    char reserved1;                        // 3 bits
    int elementaryPID;                    // 13 bits
    char reserved2;                        // 4 bits
    int esInfoLength;                    // 12 bits, # of bytes of the descriptor
    
    int descriptorCount;
    DescriptionTable** descriptions;
} ElementStreamInfo;

typedef struct {
    char tableID;                       // 8 bits
    char sectionSyntaxIndicator;        // 1 bit
    char privateBit;                    // 1 bit, must be 0
    char reserved1;                     // 2 bits, must be 0b11
    int sectionLength;                  // 12 bits
    int programNumber;                  // 16 bits
    char reserved2;                     // 2 bits, must be 0b11
    char versionNumber;                 // 5 bits
    char currentNextIndicator;          // 1 bit
    char sectionNumber;                 // 8 bits
    char lastSectionNumber;             // 8 bits
    char reserved3;                     // 3 bits
    int pcr_pid;                        // 13 bits
    char reserved4;                        // 4 bits
    int programInfoLength;                // 12 bits, 0x00??????????, # of bytes of the descriptor
    
    int programDescriptorCount;
    DescriptionTable** descriptions;
    
    int elementStreamCount;
    ElementStreamInfo** elementStreams;
    
    int crc32;                            // 32 bits
} ProgramMapTable;

typedef struct {
    uint8_t adaptationFieldLength;
    uint8_t discontinuityIndicator;
    uint8_t randomAccessIndicator;
    uint8_t elementaryStreamPriorityIndicator;
    uint8_t pcrFlag;
    uint8_t opcrFlag;
    uint8_t splicingPointFlag;
    uint8_t transportPrivateDataFlag;
    uint8_t adaptationFieldExtensionFlag;
} AdaptationField;

typedef struct {
    uint8_t streamID;
    uint16_t packetLength;
    
} PESPacket;

void dealloc_program_association_table(ProgramAssociationTable *info);
void dealloc_program_map_table(ProgramMapTable *info);

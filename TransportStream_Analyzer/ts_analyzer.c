//
//  ts_analyzer.c
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 9..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#include "ts_analyzer.h"
// https://en.wikipedia.org/wiki/MPEG_transport_stream

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

void dealloc_program_association_table(ProgramAssociationTable *info) {
    if (info == NULL) { return; }
    for (int i = 0; i < info->programInfoCount; i++) {
        // dealloc program number pid info
        free(info->infos[i]);
    }
    free(info->infos);
    free(info);
}

typedef struct {
    
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

int program_map_PID = -1;
int network_PID = -1;
ProgramAssociationTable *current;
ProgramAssociationTable *recent;

// MARK: - Utilities

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

typedef struct {
    char *data;
    int offset;
} DataReader;

// 1 ~ 8
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


// MARK: - Print functions
void print_pid_info(int pid) {
    if (pid == 0x00) {
        printf("\tProgram Association Table (PAT)\n");
    } else if (pid == 0x01) {
        printf("\tConditional Access Table (CAT)\n");
    } else if (pid == 0x02) {
        printf("\tTransport Stream Description Table (TSDT)\n");
    } else if (pid == 0x03) {
        printf("\tIPMP Control Information Table\n");
    } else if (pid >= 0x04 && pid <= 0x0F) {
        printf("\tPID Reserved for future use\n");
    } else if (pid >= 0x10 && pid <= 0x1F) {
        printf("\tDVB metadata - ");
        if (pid == 0x10) {
            printf("NIT, ST\n");
        } else if (pid == 0x11) {
            printf("SDT, BAT, ST\n");
        } else if (pid == 0x12) {
            printf("EIT, ST, CIT\n");
        } else if (pid == 0x13) {
            printf("RST, ST\n");
        } else if (pid == 0x14) {
            printf("TDT, TOT, ST\n");
        } else if (pid == 0x15) {
            printf("network synchronization\n");
        } else if (pid == 0x16) {
            printf("RNT\n");
        } else if (pid >= 0x17 && pid <= 0x1B) {
            printf("reserved for future use\n");
        } else if (pid == 0x1C) {
            printf("inband signalling\n");
        } else if (pid == 0x1D) {
            printf("measurement\n");
        } else if (pid == 0x1E) {
            printf("DIT\n");
        } else if (pid == 0x1F) {
            printf("SIT\n");
        }
    } else if ((pid >= 0x20 && pid <= 0x1FFA) || (pid >= 0x1FFC && pid <= 0x1FFE)) {
        printf("\tProgram Map Tables, elementary streams and other data tables\n");
    } else if (pid == 0x1FFB) {
        printf("\tDigiCipher 2/ATSC MGT metadata\n");
    } else if (pid == 0x1FFF) {
        printf("\tNull Packet\n");
    }
}

// MARK: - Analyzers
ProgramNumberPIDInfo* analyze_program_number_pid_association(char *data) {
    ProgramNumberPIDInfo* info = (ProgramNumberPIDInfo *)malloc(sizeof(ProgramNumberPIDInfo));
    
    DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
    reader->data = data;
    reader->offset = 0;
    
    info->programNumber = readData(reader, 16);
    info->reserved = readData(reader, 3);
    info->pid = readData(reader, 13);
    
    free(reader);
    return info;
}

ProgramAssociationTable* analyze_program_association_table(char *data) {
    ProgramAssociationTable *table = (ProgramAssociationTable *) malloc(sizeof(ProgramAssociationTable));
    int pointerField;
    
    pointerField = data[0] + 1;
    
    DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
    reader->data = data + pointerField;
    reader->offset = 0;
    
    table->tableID = readData(reader, 8);
    table->sectionSyntaxIndicator = readData(reader, 1);
    table->privateBit = readData(reader, 1);
    table->reserved1 = readData(reader, 2);
    table->sectionLength = readData(reader, 12);
    int defaultInfoLen = reader->offset;
    
    table->transportStreamID = readData(reader, 16);
    table->reserved2 = readData(reader, 2);
    table->versionNumber = readData(reader, 5);
    table->currentNextIndicator = readData(reader, 1);
    table->sectionNumber = readData(reader, 8);
    table->lastSectionNumber = readData(reader, 8);
    
    // includes belows
    //   transport_stream_id (16 bits)
    //   reserved (2 bits)
    //   version_number (5 bits)
    //   current_next_indicator (1 bits)
    //   section_number (8 bits)
    //   last_section_number (8 bits)
    int sectionInfoSize = (reader->offset - defaultInfoLen) / 8;
    
    int crcSize = 32 / 8;
//    int offset = reader->offset / 8;
    
    table->programInfoCount = (table->sectionLength - sectionInfoSize - crcSize) / 4;
    table->infos = (ProgramNumberPIDInfo **) malloc(sizeof(ProgramNumberPIDInfo *) * table->programInfoCount);
    for (int i = 0; i < table->programInfoCount; i++) {
        table->infos[i] = analyze_program_number_pid_association(data + pointerField + (reader->offset / 8));
        reader->offset = reader->offset + 32;
    }
    
    table->crc32 = readData(reader, 32);
//    print_binary(data + pointerField + offset, 4);
    
    free(reader);
    
    return table;
}

ElementStreamInfo* analyze_element_stream_info(char *data) {
    ElementStreamInfo *info = (ElementStreamInfo *)malloc(sizeof(ElementStreamInfo));
    
    DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
    reader->data = data;
    reader->offset = 0;
    
    info->streamType = readData(reader, 8);
    info->reserved1 = readData(reader, 3);
    info->elementaryPID = readData(reader, 13);
    info->reserved2 = readData(reader, 4);
    info->esInfoLength = readData(reader, 12);
    
    // TODO: analyze descriptions
    
    free(reader);
    return info;
}


ProgramMapTable* analyze_program_map_table(char *data) {
    ProgramMapTable *table = (ProgramMapTable *)malloc(sizeof(ProgramMapTable));
    int pointerField;
    
    pointerField = data[0] + 1;
    
    table->tableID = data[pointerField];
    table->sectionSyntaxIndicator = (data[pointerField + 1] >> 7) & 0x01;
    table->privateBit = (data[pointerField + 1] >> 6) & 0x01;
    table->reserved1 = (data[pointerField + 1] >> 4) & 0x03;
    table->sectionLength = ((data[pointerField + 1] & 0x0F) << 8) | data[pointerField + 2]; // 0x00??????????
    
    table->programNumber = (data[pointerField + 3] << 8) | data[pointerField + 4];
    table->reserved2 = (data[pointerField + 5] >> 6) & 0x03;
    table->versionNumber = (data[pointerField + 5] >> 1) & 0x1F;
    table->currentNextIndicator = data[pointerField + 5] & 0x01;
    table->sectionNumber = data[pointerField + 6];
    table->lastSectionNumber = data[pointerField + 7];
    table->reserved3 = (data[pointerField + 8] >> 5) & 0x07;
    table->pcr_pid = ((data[pointerField + 8] & 0x1F) << 8) | data[pointerField + 9];
    table->reserved4 = (data[pointerField + 10] >> 4) & 0x0F;
    table->programInfoLength = ((data[pointerField + 10] & 0x0F) << 8) | data[pointerField + 11];
    
    // includes belows
    //   program_number (16 bits)
    //   reserved (2 bits)
    //   version_number (5 bits)
    //   current_next_indicator (1 bits)
    //   section_number (8 bits)
    //   last_section_number (8 bits)
    //   reserved (3 bits)
    //   PCR_PID (13 bits)
    //   reserved (4 bits)
    //   program_info_length (12 bits)
    int sectionInfoSize = (16 + 2 + 5 + 1 + 8 + 8 + 3 + 13 + 4 + 12) / 8;
    
    int crcSize = 32 / 8;
    int offset = sectionInfoSize + (8 + 1 + 1 + 2 + 12) / 8;
    
    table->programDescriptorCount = 0; // table->programInfoLength /
    
    int count = 0;
    while (offset < table->sectionLength) {
        table->elementStreams[count] = analyze_element_stream_info(data + pointerField + offset);
        count++;
        offset = offset + (8 + 3 + 13 + 4 + 12) / 8 + table->elementStreams[count]->esInfoLength;
    }
    table->elementStreamCount = (table->sectionLength - table->programInfoLength - sectionInfoSize - crcSize) / 4;
    table->elementStreams = (ElementStreamInfo **) malloc(sizeof(ElementStreamInfo *) * table->elementStreamCount);
    for (int i = 0; i < table->elementStreamCount; i++) {
        table->elementStreams[i] = analyze_element_stream_info(data + pointerField + offset);
        offset = offset + (8 + 3 + 13 + 4 + 12) / 8 + table->elementStreams[i]->esInfoLength;
    }
    
    table->crc32 = *(int *)(data + pointerField + offset);
//    print_binary(data + pointerField + offset, 4);
    
    return table;
}

int analyzeAdaptationFieldData(char *data) {
    int adaptationFieldLength = data[0];
    if (adaptationFieldLength > 0) {
        DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
        reader->data = data + 1;
        reader->offset = 0;
        
        char discontinuityIndicator = readData(reader, 1);
        char randomAccessIndicator = readData(reader, 1);
        char elementaryStreamPriorityIndicator = readData(reader, 1);
        char PCRFlag = readData(reader, 1);
        char OPCRFlage = readData(reader, 1);
        char splicingPointFlag = readData(reader, 1);
        char transportPrivateDataFlag = readData(reader, 1);
        char adaptationFieldExtensionFlag = readData(reader, 1);
        
        free(reader);
        
        printf("Adaptaion Field :\n");
        printf("\tDiscontinuity indicator : %d\n", discontinuityIndicator);
        printf("\tRandom access indicator : %d\n", randomAccessIndicator);
        printf("\tElementary stream priority indicator : %d\n", elementaryStreamPriorityIndicator);
        printf("\tPCR flag : %d\n", PCRFlag);
        printf("\tOPCR flag : %d\n", OPCRFlage);
        printf("\tSplicing point flag : %d\n", splicingPointFlag);
        printf("\tTransport private data flag : %d\n", transportPrivateDataFlag);
        printf("\tAdaptation field extension flag : %d\n", adaptationFieldExtensionFlag);
        
    }
    return adaptationFieldLength;
}

void print_program_association_table(ProgramAssociationTable *info) {
    printf("\ttableID: %X\n", info->tableID);
    printf("\tsection_syntax_indicator: %d\n", info->sectionSyntaxIndicator);
    printf("\tprivateBit (must be 0): %d\n", info->privateBit);
    printf("\treserved: %d\n", info->reserved1);
    printf("\tsection_length: %d\n", info->sectionLength);
    printf("\n");
    printf("\ttransport_stream_id: %X\n", info->transportStreamID);
    printf("\treserved: %d\n", info->reserved2);
    printf("\tversion_number: %d\n", info->versionNumber);
    printf("\tcurrent_next_indicator: %d\n", info->currentNextIndicator);
    printf("\tsection_number: %d\n", info->sectionNumber);
    printf("\tlast_section_number: %d\n", info->lastSectionNumber);
    
    printf("\tprogram info count: %d\n", info->programInfoCount);
    
    // each program info data length - 16 + 3 + 13 bits => 32 bits => 4 bytes
    for (int i = 0; i < info->programInfoCount; i++) {
        printf("\tprogram # %d\n", i+1);
        ProgramNumberPIDInfo *pidinfo = info->infos[i];
        printf("\t\tprogram_number: %d\n", pidinfo->programNumber);
        printf("\t\treserved: %d\n", pidinfo->reserved);
        printf("\t\tpid: %04X\n", pidinfo->pid);
    }
    
    printf("\tCRC_32: ");
    print_binary_int(info->crc32);
    printf("\n");
}

void analyzeData(char *data, int pid) {
    print_binary(data, 3);
    
    printf("PID Info - \n");
    if (pid == 0x0000) {
        // Program Association Table
        printf("\tProgram Association Table\n");
        ProgramAssociationTable *info = analyze_program_association_table(data);
        print_program_association_table(info);
        
        if (recent != current) {
            dealloc_program_association_table(recent);
        }
        recent = info;
        if (info->currentNextIndicator == 1) {
            dealloc_program_association_table(current);
            current = info;
        }
        
    } else if (pid == 0x0001) {
        // Control Access Table
        printf("\tControl Access Table\n");
    } else if (pid == 0x0002) {
        // Transport Stream Description Table
        printf("\tTransport Stream Description Table\n");
    } else if (pid >= 0x0003 && pid <= 0x000F) {
        // Reserved
        printf("\tReserved\n");
    } else if (pid >= 0x0010 && pid <= 0x1FFE) {
        // May be assigned as network_PID, Program_map_PID, or for other purposes
        printf("\tMay be assigned as network_PID, Program_map_PID, or for other purposes\n");
        
        if (pid == program_map_PID) {
//            print_pat_info(data);
            ProgramMapTable *table = analyze_program_map_table(data);
        }
    } else if (pid == 0x1FFF) {
        // Null packet
        printf("\tNull packet\n");
    } else {
        printf("\nOther (%X)\n", pid);
    }
}

void analyzeTransportPacket(char *packet) {
    DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
    reader->data = packet;
    reader->offset = 0;
    
    char syncByte = readData(reader, 8);
    char tei = readData(reader, 1);
    char pusi = readData(reader, 1);
    char transportPriority = readData(reader, 1);
    int pid = readData(reader, 13);
    
    char tsc = readData(reader, 2); // transport_scrambling_control
    char adaptationFieldControl = readData(reader, 2);
    char continuityCounter = readData(reader, 4);
    
    free(reader);
    
    printf("tei : %d, pusi : %d, transportPriority: %d, pid : %04X\n", tei, pusi, transportPriority, pid);
    print_pid_info(pid);
    printf("tsc : %d, continuityCounter : %X\n", tsc, continuityCounter);
    
    printf("Adaptation field control value : ");
    if (adaptationFieldControl == 1) {
        printf("No adaptation_field, payload only\n");
    } else if (adaptationFieldControl == 2) {
        printf("Adaptation_field only, no payload\n");
    } else if (adaptationFieldControl == 3) {
        printf("Adaptation_field followed by payload\n");
    } else {
        printf("Reserved for future use by ISO/IEC\n");
    }
    
    int dataByteOffset = 4;
    if (adaptationFieldControl == 0x10 || adaptationFieldControl == 0x11) {
        // handle adaptation field
        dataByteOffset = analyzeAdaptationFieldData(packet + 4);
    }
    
    if (adaptationFieldControl == 0x01 || adaptationFieldControl == 0x11) {
        // handle data
        analyzeData(packet + dataByteOffset, pid);
    }
    
}

int main(int argc, char* argv[]) {
    FILE *fp;
    char packet[188];
    char* filename;
    
    if(argc < 2)
    {
        printf("usage : %s <input>\n", argv[0]);
        return 0;
    }
    
    filename = argv[1];
    
    fp = fopen(filename, "r");
    size_t result;
    int count = 0;
    while ((result = fread(&packet, 1, 188, fp)) > 0) {
        count++;
        printf("====== Transport Packet # %d ======\n", count);
        
        analyzeTransportPacket(packet);
        
        print_binary(packet, 188);
        
        if (count > 4) {
            break;
        }
    }
    
    fclose(fp);
    
    return 0;
}


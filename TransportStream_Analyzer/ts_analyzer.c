//
//  ts_analyzer.c
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 9..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#include "ts_analyzer.h"
#include "tsprinter.h"
//#include "tsmodel.h"
#include "tsutil.h"
// https://en.wikipedia.org/wiki/MPEG_transport_stream

         
int program_map_PID = -1;
int network_PID = -1;
ProgramAssociationTable *current;
ProgramAssociationTable *recent;

ProgramMapTable *currentPMT;
ProgramMapTable *recentPMT;

// 1 ~ 8

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

DescriptionTable* analyze_descripor(char *data) {
    DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
    reader->data = data;
    reader->offset = 0;
    
    DescriptionTable *descriptor = (DescriptionTable *)malloc(sizeof(DescriptionTable));
    descriptor->descriptorTag = readData(reader, 8);
    descriptor->descriptorLength = readData(reader, 8);
    
    free(reader);
    return descriptor;
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
    
    info->descriptions = (DescriptionTable **)malloc(sizeof(DescriptionTable *) * (info->esInfoLength / 2));
    int streamInfoSize = reader->offset;
    int count = 0;
    // TODO: analyze descriptions
    if (reader->offset - streamInfoSize < info->esInfoLength) {
        DescriptionTable *descriptor = analyze_descripor(data + (reader->offset / 8));
        info->descriptions[count] = descriptor;
        reader->offset += ((descriptor->descriptorLength + 2) * 8);
        count++;
    }
    info->descriptorCount = count;
    
    free(reader);
    return info;
}

ProgramMapTable* analyze_program_map_table(char *data) {
    ProgramMapTable *table = (ProgramMapTable *)malloc(sizeof(ProgramMapTable));
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
    int defaultInfoBits = reader->offset;
    
    table->programNumber = readData(reader, 16);
    table->reserved2 = readData(reader, 2);
    table->versionNumber = readData(reader, 5);
    table->currentNextIndicator = readData(reader, 1);
    table->sectionNumber = readData(reader, 8);
    table->lastSectionNumber = readData(reader, 8);
    table->reserved3 = readData(reader, 3);
    table->pcr_pid = readData(reader, 13);
    table->reserved4 = readData(reader, 4);
    table->programInfoLength = readData(reader, 12);
    
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
    int sectionInfoSize = (reader->offset - defaultInfoBits) / 8; // Bytes
    
    int crcSize = 32 / 8;
    
    table->programDescriptorCount = 0; // table->programInfoLength /
    int programInfoOffset = 0;
    while (programInfoOffset < table->programInfoLength) {
        // TODO: Analyze Program Descriptor
        char descriptorTag = readData(reader, 8);
        char descriptorLength = readData(reader, 8);
        
        reader->offset += (descriptorLength * 8);
        programInfoOffset += (2 + descriptorLength);
    }
    int offset = reader->offset / 8;
    
    int maximumInfoCount = (table->sectionLength - 4 - sectionInfoSize) / 5;
    table->elementStreams = (ElementStreamInfo **)malloc(sizeof(ElementStreamInfo *) * maximumInfoCount);
    int count = 0;
    while (offset < (table->sectionLength - 4)) {
        ElementStreamInfo *esInfo = analyze_element_stream_info(data + pointerField + offset);
        table->elementStreams[count] = esInfo;
        count++;
//        offset = offset + (8 + 3 + 13 + 4 + 12) / 8 + table->elementStreams[count]->esInfoLength;
        offset = offset + (8 + 3 + 13 + 4 + 12) / 8 + esInfo->esInfoLength;
        reader->offset += (8 + 3 + 13 + 4 + 12) + (esInfo->esInfoLength * 8);
    }
    
    table->elementStreamCount = count;
    
    table->crc32 = readData(reader, 32);
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
        
        printf("Adaptaion Field -\n");
        printf("\tadaptation_field_length : %d\n", adaptationFieldLength);
        printf("\tDiscontinuity indicator : %d\n", discontinuityIndicator);
        printf("\tRandom access indicator : %d\n", randomAccessIndicator);
        printf("\tElementary stream priority indicator : %d\n", elementaryStreamPriorityIndicator);
        printf("\tPCR flag : %d\n", PCRFlag);
        printf("\tOPCR flag : %d\n", OPCRFlage);
        printf("\tSplicing point flag : %d\n", splicingPointFlag);
        printf("\tTransport private data flag : %d\n", transportPrivateDataFlag);
        printf("\tAdaptation field extension flag : %d\n", adaptationFieldExtensionFlag);
        
    }
    return adaptationFieldLength + 1;
}

void analyze_pes_packet(char *data) {
    DataReader *reader = (DataReader *)malloc(sizeof(DataReader));
    reader->data = data;
    reader->offset = 0;
    
    uint32_t packetStartCodePrefix = readData(reader, 24);
    uint8_t streamID = readData(reader, 8);
    uint16_t pesPacketLength = readData(reader, 16);
    
    printf("packet_start_code_prefix : %03X\n", packetStartCodePrefix);
    printf("stream_id : ");
    print_binary((char *)&streamID, 1);
    printf("PES_packet_length : %d\n", pesPacketLength);
    printf("\n");
    \
    if (streamID != 0xBC && streamID != 0xBE
        && streamID != 0xBF && streamID != 0xF0
        && streamID != 0xF1 && streamID != 0xFF
        && streamID != 0xF2 && streamID != 0xF8) {
        uint8_t privateBits = readData(reader, 2); // must be `10`
        uint8_t pesScramblingControl = readData(reader, 2);
        uint8_t pesPriority = readData(reader, 1);
        uint8_t dataAlignmentIndicator = readData(reader, 1);
        uint8_t copyright = readData(reader, 1);
        uint8_t originalOrCopy = readData(reader, 1);
        uint8_t ptsDtsFlags = readData(reader, 2);
        uint8_t escrFlag = readData(reader, 1);
        uint8_t esRateFlag = readData(reader, 1);
        uint8_t dsmTrickModeFlag = readData(reader, 1);
        uint8_t additionalCopyInfoFlag = readData(reader, 1);
        uint8_t pesCrcFlag = readData(reader, 1);
        uint8_t pesExtensionFlag = readData(reader, 1);
        uint8_t pesHeaderDataLength = readData(reader, 8);
        
        printf("privateBits : %d\n", privateBits);
        printf("PES_scrambling_control : %d\n", pesScramblingControl);
        printf("PES_priority : %d\n", pesPriority);
        printf("data_alignment_indicator : %d\n", dataAlignmentIndicator);
        printf("copyright : %d\n", copyright);
        printf("original_or_copy : %d\n", originalOrCopy);
        printf("PTS_DTS_flags : %d\n", ptsDtsFlags);
        printf("ESCR_flag : %d\n", escrFlag);
        printf("ES_rate_flag : %d\n", esRateFlag);
        printf("DSM_trick_mode_flag : %d\n", dsmTrickModeFlag);
        printf("additional_copy_info_flag : %d\n", additionalCopyInfoFlag);
        printf("PES_CRC_flag : %d\n", pesCrcFlag);
        printf("PES_extension_flag : %d\n", pesExtensionFlag);
        printf("PES_header_data_length : %d\n", pesHeaderDataLength);
        
        printf("\n");
        if (ptsDtsFlags == 0b10) {
            uint8_t privateBits2 = readData(reader, 4);
            uint64_t pts = ((uint64_t)readData(reader, 4) & 0x0e) << 29 | (readData(reader, 16) >> 1) << 15 | (readData(reader, 16) >> 1);
            
            printf("\tprivateBits2 : %d\n", privateBits2);
            printf("\tpts: ");
            print_binary_int64(pts);
        }
        if (ptsDtsFlags == 0b11) {
            uint8_t privateBits2 = readData(reader, 4);
            uint64_t pts = ((uint64_t)readData(reader, 4) & 0x0e) << 29 | (readData(reader, 16) >> 1) << 15 | (readData(reader, 16) >> 1);
            
            printf("\tprivateBits2 : %d\n", privateBits2);
            printf("\tpts: ");
            print_binary_int64(pts);
            
            uint8_t privateBits3 = readData(reader, 4);
            uint64_t dts = ((uint64_t)readData(reader, 4) & 0x0e) << 29 | (readData(reader, 16) >> 1) << 15 | (readData(reader, 16) >> 1);
            printf("\tdts: ");
            print_binary_int64(dts);
        }
        
    }
    
    free(reader);
}

void analyzeData(char *data, int pid) {
//    print_binary(data, 3);
    
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
            
            for (int i = 0; i < info->programInfoCount; i++) {
                ProgramNumberPIDInfo *pidInfo = info->infos[i];
                if (pidInfo->programNumber == 0) {
                    network_PID = pidInfo->pid;
                } else {
                    program_map_PID = pidInfo->pid;
                }
            }
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
            print_program_map_table(table);
            
            if (recentPMT != currentPMT) {
                dealloc_program_map_table(recentPMT);
            }
            recentPMT = table;
            
            if (table->currentNextIndicator == 1) {
                dealloc_program_map_table(currentPMT);
                currentPMT = table;
            }
        } else {
            analyze_pes_packet(data);
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
    if (adaptationFieldControl == 0b10 || adaptationFieldControl == 0b11) {
        // handle adaptation field
        dataByteOffset += analyzeAdaptationFieldData(packet + 4);
    }
    
    if (adaptationFieldControl == 0b01 || adaptationFieldControl == 0b11) {
        // handle data
        analyzeData(packet + dataByteOffset, pid);
        print_binary(packet + dataByteOffset, 188 - dataByteOffset);
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
        
//        print_binary(packet, 188);
        printf("\n");
        if (count > 20) {
            break;
        }
    }
    
    fclose(fp);
    
    return 0;
}


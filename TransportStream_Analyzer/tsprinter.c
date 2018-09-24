//
//  tsprinter.c
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 16..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#include "tsprinter.h"
#include "tsutil.h"

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
}

void print_stream_type(uint8_t streamType) {
    if (streamType == 0x00) {
        printf("ITU-T | ISO/IEC Reserved");
    } else if (streamType == 0x01) {
        printf("ISO/IEC 11172 Video");
    } else if (streamType == 0x02) {
        printf("ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream");
    } else if (streamType == 0x03) {
        printf("ISO/IEC 11172 Audio");
    } else if (streamType == 0x04) {
        printf("ISO/IEC 13818-3 Audio");
    } else if (streamType == 0x05) {
        printf("ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections");
    } else if (streamType == 0x06) {
        printf("ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data");
    } else if (streamType == 0x07) {
        printf("ISO/IEC 13522 MHEG");
    } else if (streamType == 0x08) {
        printf("ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC");
    } else if (streamType == 0x09) {
        printf("ITU-T Rec. H.222.1");
    } else if (streamType == 0x0A) {
        printf("ISO/IEC 13818-6 type A");
    } else if (streamType == 0x0B) {
        printf("ISO/IEC 13818-6 type B");
    } else if (streamType == 0x0C) {
        printf("ISO/IEC 13818-6 type C");
    } else if (streamType == 0x0D) {
        printf("ISO/IEC 13818-6 type D");
    } else if (streamType == 0x0E) {
        printf("ITU-T Rec. H.222.0 | ISO/IEC 13818-1 auxiliary");
    } else if (streamType == 0x0F) {
        printf("ISO/IEC 13818-7 Audio with ADTS transport syntax");
    } else if (streamType == 0x10) {
        printf("ISO/IEC 14496-2 Visual");
    } else if (streamType == 0x11) {
        printf("ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3 / AMD 1");
    } else if (streamType == 0x12) {
        printf("ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets");
    } else if (streamType == 0x13) {
        printf("ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC14496_sections.");
    } else if (streamType == 0x14) {
        printf("ISO/IEC 13818-6 Synchronized Download Protocol");
    } else if (streamType >= 0x15 && streamType <= 0x7F) {
        printf("ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Reserved");
    } else if (streamType >= 0x80 && streamType <= 0xFF) {
        printf("User Private");
    }
}

void print_descriptor(DescriptionTable *info) {
    printf("\t\t\tdescriptor_tag: %X\n", info->descriptorTag);
    printf("\t\t\tdescriptor_length: %d\n", info->descriptorLength);
}

void print_program_element_stream_info(ElementStreamInfo *info) {
    printf("\t\tstream_type: %02X - ", info->streamType);
    print_stream_type(info->streamType);
    printf("\n");
    
    printf("\t\treserved: %d\n", info->reserved1);
    printf("\t\telementary_PID: %04X\n", info->elementaryPID);
    printf("\t\treserved: %d\n", info->reserved2);
    printf("\t\tES_info_length: %d\n", info->esInfoLength);
    
    for (int i = 0; i < info->descriptorCount; i++) {
        printf("\t\t- Descriptor # %d\n", i + 1);
        print_descriptor(info->descriptions[i]);
        printf("\n");
    }
}

void print_program_map_table(ProgramMapTable *info) {
    printf("\ttableID: %X\n", info->tableID);
    printf("\tsection_syntax_indicator: %d\n", info->sectionSyntaxIndicator);
    printf("\tprivateBit (must be 0): %d\n", info->privateBit);
    printf("\treserved: %d\n", info->reserved1);
    printf("\tsection_length: %d\n", info->sectionLength);
    printf("\n");
    printf("\tprogram_number: %X\n", info->programNumber);
    printf("\treserved: %d\n", info->reserved2);
    printf("\tversion_number: %d\n", info->versionNumber);
    printf("\tcurrent_next_indicator: %d\n", info->currentNextIndicator);
    printf("\tsection_number: %d\n", info->sectionNumber);
    printf("\tlast_section_number: %d\n", info->lastSectionNumber);
    
    printf("\treserved: %d\n", info->reserved3);
    printf("\tPCR_PID: %X\n", info->pcr_pid);
    printf("\treserved: %d\n", info->reserved4);
    printf("\tprogram_info_length: %d\n", info->programInfoLength);
    
    // each program info data length - 16 + 3 + 13 bits => 32 bits => 4 bytes
    for (int i = 0; i < info->programDescriptorCount; i++) {
        
    }
    
    for (int i = 0; i < info->elementStreamCount; i++) {
        printf("\t- Element Stream PID info # %d\n", i + 1);
        print_program_element_stream_info(info->elementStreams[i]);
        printf("\n");
    }
    
    printf("\tCRC_32: ");
    print_binary_int(info->crc32);
}

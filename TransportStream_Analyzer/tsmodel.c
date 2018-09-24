//
//  tsmodel.c
//  TransportStream_Analyzer
//
//  Created by uhniboogui on 2018. 9. 16..
//  Copyright © 2018년 uhniboogui. All rights reserved.
//

#include "tsmodel.h"
#include <stdlib.h>

void dealloc_program_association_table(ProgramAssociationTable *info) {
    if (info == NULL) { return; }
    for (int i = 0; i < info->programInfoCount; i++) {
        // dealloc program number pid info
        free(info->infos[i]);
    }
    free(info->infos);
    free(info);
}



void dealloc_program_map_table(ProgramMapTable *info) {
    if (info == NULL) { return; }
    if (info->programDescriptorCount > 0) {
        for (int i = 0; i < info->programDescriptorCount; i++) {
            free(info->descriptions[i]);
        }
        free(info->descriptions);
    }
    
    for(int i = 0; i < info->elementStreamCount; i++) {
        free(info->elementStreams[i]);
    }
    free(info->elementStreams);
    
    free(info);
}

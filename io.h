/*
 *  io.h
 *  HFSPlusRecovery
 *
 *  Created by Adrian Moser on 16.08.08.
 *  Copyright (c) 2008, Adrian Moser
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *  * Neither the name of the author nor the
 *  names of its contributors may be used to endorse or promote products
 *  derived from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <CoreServices/CoreServices.h>
#include <sys/attr.h>
#include "definitions.h"

#define VOL_HEADER_OFFSET 1024L
#define RSRC_FORK_NAME "/..namedfork/rsrc"
#define RSRC_FORK_NAME_LEN 17
#define FIRST_KEY_OFFSET 14

typedef struct {
    u_int64_t volOffset;
    FILE *device;
    HFSPlusVolumeHeader volHeader;
    BTHeaderRec *catalogHeader;
    BTHeaderRec *extentsHeader;
} HFSPlusVolume;

typedef struct {
    unsigned long length;
    fsobj_type_t objType;
    char finderInfo[32];
} FInfoAttrBuf;

typedef struct attrlist attrlist_t;

u_int64_t 
calculateCatalogOffset(const u_int32_t nodeNum);

void 
openVolume(const char *const dev, u_int64_t volOffset);

void 
readVolumeHeader();

void 
readNodeDescriptor(const u_int32_t offset, BTNodeDescriptor *const desc);

void 
readHeaderRecord(const u_int32_t offset, BTHeaderRec *const headerRec);

void 
readHeaderNode(u_int32_t offset, BTHeaderRec **header);

void 
readNode(const u_int64_t offset, char *const node, u_int32_t nodeSize);

void 
readCatalogNode(const u_int32_t nodeNum, char *const node);

void 
copyFile(const file *const f, const char *const dstFileName);

int 
copyFork(const HFSPlusForkData *const fork, const orderedlist *const extents, 
      FILE *const dst);

int 
copyExtent(char *const buf, const HFSPlusExtentDescriptor *const desc, 
      FILE *const dst, int isLastExtent, u_int64_t partialBlockSize);

void 
setFinderInfo(const char *const fileName, const FndrFileInfo *const info);

void 
sequentiallyReadCatalog(int(*filter)(HFSPlusCatalogKey*, sint16), 
      void(*handler)(HFSPlusCatalogKey*, sint16, void*));

void 
iterateOverCatalogRecords(char *node, BTNodeDescriptor *desc, 
      int(*filter)(HFSPlusCatalogKey*, sint16), 
      void(*handler)(HFSPlusCatalogKey*, sint16, void*) );
    
void 
iterateOverCatalog( int(*filter)(HFSPlusCatalogKey*, sint16), 
      void(*handler)(HFSPlusCatalogKey*, sint16, void*) );
    
void 
iterateOverExtentsRecords(char *node, BTNodeDescriptor *desc, 
      void(*handler)(HFSPlusExtentKey*, HFSPlusExtentRecord*));

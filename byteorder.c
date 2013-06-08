/*
 *  byteorder.c
 *  HFSPlusRecovery
 *
 *  Created by Adrian Moser on 15.08.08.
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
#include "byteorder.h"

void
convertNodeDescriptorToHostByteOrder(BTNodeDescriptor *const desc) {
   desc->fLink = CFSwapInt32BigToHost(desc->fLink);
   desc->bLink = CFSwapInt32BigToHost(desc->bLink);
   desc->numRecords = CFSwapInt16BigToHost(desc->numRecords);
}

void 
convertHeaderRecordToHostByteOrder(BTHeaderRec *const hdr) {
   hdr->treeDepth = CFSwapInt16BigToHost(hdr->treeDepth);
   hdr->rootNode = CFSwapInt32BigToHost(hdr->rootNode);
   hdr->leafRecords = CFSwapInt32BigToHost(hdr->leafRecords);
   hdr->firstLeafNode = CFSwapInt32BigToHost(hdr->firstLeafNode);
   hdr->lastLeafNode = CFSwapInt32BigToHost(hdr->lastLeafNode);
   hdr->nodeSize = CFSwapInt16BigToHost(hdr->nodeSize);
   hdr->maxKeyLength = CFSwapInt16BigToHost(hdr->maxKeyLength);
   hdr->totalNodes = CFSwapInt32BigToHost(hdr->totalNodes);
   hdr->freeNodes = CFSwapInt32BigToHost(hdr->freeNodes);
   hdr->clumpSize = CFSwapInt32BigToHost(hdr->clumpSize);
   hdr->attributes = CFSwapInt32BigToHost(hdr->attributes);
}

void 
convertFileToHostByteOrder(HFSPlusForkData *const file) {
   int i;
   
   file->logicalSize = CFSwapInt64BigToHost(file->logicalSize);
   file->clumpSize = CFSwapInt32BigToHost(file->clumpSize);
   file->totalBlocks = CFSwapInt32BigToHost(file->totalBlocks);
   
   for (i = 0; i < 8; i++) {
      if (file->extents[i].startBlock != 0 
            || file->extents[i].blockCount != 0) {
         file->extents[i].startBlock = 
            CFSwapInt32BigToHost(file->extents[i].startBlock);
         file->extents[i].blockCount = 
            CFSwapInt32BigToHost(file->extents[i].blockCount);
      }
   }
}

void 
convertHFSPlusCatalogFileToHostByteOrder(HFSPlusCatalogFile *const cat) {
   cat->recordType = CFSwapInt16BigToHost(cat->recordType);
   cat->flags = CFSwapInt16BigToHost(cat->flags);
   cat->fileID = CFSwapInt32BigToHost(cat->fileID);
   cat->createDate = CFSwapInt32BigToHost(cat->createDate);
   cat->contentModDate = CFSwapInt32BigToHost(cat->contentModDate);
   cat->attributeModDate = CFSwapInt32BigToHost(cat->attributeModDate);
   cat->accessDate = CFSwapInt32BigToHost(cat->accessDate);
   cat->backupDate = CFSwapInt32BigToHost(cat->backupDate);
   cat->textEncoding = CFSwapInt32BigToHost(cat->textEncoding);
   cat->userInfo.fdFlags = CFSwapInt16BigToHost(cat->userInfo.fdFlags);   
   
   convertFileToHostByteOrder(&cat->dataFork);
   convertFileToHostByteOrder(&cat->resourceFork);
}

void 
convertHFSPlusCatalogFolderToHostByteOrder(HFSPlusCatalogFolder *const cat) {
   cat->recordType = CFSwapInt16BigToHost(cat->recordType);
   cat->flags = CFSwapInt16BigToHost(cat->flags);
   cat->valence = CFSwapInt32BigToHost(cat->valence);
   cat->folderID = CFSwapInt32BigToHost(cat->folderID);
   cat->createDate = CFSwapInt32BigToHost(cat->createDate);
   cat->contentModDate = CFSwapInt32BigToHost(cat->contentModDate);
   cat->attributeModDate = CFSwapInt32BigToHost(cat->attributeModDate);
   cat->accessDate = CFSwapInt32BigToHost(cat->accessDate);
   cat->backupDate = CFSwapInt32BigToHost(cat->backupDate);
   cat->textEncoding = CFSwapInt32BigToHost(cat->textEncoding);
}

void 
convertHFSPlusCatalogThreadToHostByteOrder(HFSPlusCatalogThread *const cat) {
   cat->recordType = CFSwapInt16BigToHost(cat->recordType);
   cat->recordType = CFSwapInt16BigToHost(cat->recordType);
   cat->parentID = CFSwapInt32BigToHost(cat->parentID);
   cat->nodeName.length = CFSwapInt16BigToHost(cat->nodeName.length);
}


void 
convertHFSPlusExtentRecordToHostByteOrder(HFSPlusExtentRecord *const ext) {
   int i;
   for (i = 0; i < 8; i++) {
      HFSPlusExtentDescriptor *desc = ((HFSPlusExtentDescriptor*)ext)+i;
      desc->startBlock = CFSwapInt32BigToHost(desc->startBlock);
      desc->blockCount = CFSwapInt32BigToHost(desc->blockCount);
   }
}

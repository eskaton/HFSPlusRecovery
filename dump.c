/*
 *  dump.c
 *  HFSPlusRecovery
 *
 *  Created by Adrian Moser on 20.08.08.
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

#include "dump.h"
#include "io.h"
#include "util.h"

extern HFSPlusVolume volume;

void 
dumpForkData(const HFSPlusForkData *const file) {
   int i;
   
   printf("  logical size: %d\n", file->logicalSize);
   printf("  clump size:   %d\n", file->clumpSize);
   printf("  total blocks: %d\n", file->totalBlocks);
   printf("  Extents:   #     start block      block count\n");
   
   for (i = 0; i < 8; i++) {
      if (file->extents[i].startBlock != 0 || 
            file->extents[i].blockCount != 0) {
         printf("             %d     %-11d      %-11d\n", i, 
               file->extents[i].startBlock, 
               file->extents[i].blockCount);
      } else {
         break;
      }
   }
}

void 
dumpVolumeHeader() {
   printf("Volume header\n");
   printf(" signature:    %c%c\n", volume.volHeader.signature>>8, 
         volume.volHeader.signature);
   printf(" version:      %d\n", volume.volHeader.version);
   printf(" file count:   %d\n", volume.volHeader.fileCount);
   printf(" folder count:  %d\n", volume.volHeader.folderCount);
   printf(" block size:   %d\n", volume.volHeader.blockSize);
   printf(" total blocks:  %d\n", volume.volHeader.totalBlocks);
   printf(" free blocks:   %d\n", volume.volHeader.freeBlocks);
   printf(" Allocation file:\n");
   dumpForkData(&volume.volHeader.allocationFile);
   printf(" Extents overflow file:\n");
   dumpForkData(&volume.volHeader.extentsFile);
   printf(" Catalog file:\n");
   dumpForkData(&volume.volHeader.catalogFile);
}

void 
dumpNodeDescriptor(const BTNodeDescriptor const* desc) {
   char *kind;
   
   switch (desc->kind) {
      case kBTLeafNode:
         kind = "kBTLeafNode";
         break;
      case kBTIndexNode:
         kind = "kBTIndexNode";
         break;
      case kBTHeaderNode:
         kind = "kBTHeaderNode";
         break;
      case kBTMapNode:
         kind = "kBTMapNode";
         break;
      default:
         kind = "undefined";
   }
   
   printf("BTNodeDescriptor\n");
   printf(" flink:      %d\n", desc->fLink);
   printf(" blink:      %d\n", desc->bLink);
   printf(" kind:      %s\n", kind);
   printf(" height:     %d\n", desc->height);
   printf(" num records: %d\n", desc->numRecords);
}


void dumpHeaderRecord(const BTHeaderRec const* hdr) {
   printf("BTHeaderRec\n");
   printf(" tree depth:      %d\n", hdr->treeDepth);
   printf(" root node:      %d\n", hdr->rootNode);
   printf(" leaf records:    %d\n", hdr->leafRecords);
   printf(" first leaf node:  %d\n", hdr->firstLeafNode);
   printf(" last leaf node:   %d\n", hdr->lastLeafNode);
   printf(" node size:      %d\n", hdr->nodeSize);
   printf(" max key length:   %d\n", hdr->maxKeyLength);
   printf(" total nodes:     %d\n", hdr->totalNodes);
   printf(" free nodes:      %d\n", hdr->freeNodes);
   printf(" clump size:      %d\n", hdr->clumpSize);
   printf(" btree type:      %d\n", hdr->btreeType);
   printf(" key compare type: %d\n", hdr->keyCompareType);
   printf(" attributes:      %d\n", hdr->attributes);
}

void 
dumpFile(HFSPlusCatalogFile *file) {
   printf(" file id:      %d\n", file->fileID);
   printf(" data fork: \n");
   dumpForkData(&file->dataFork);
   printf(" resource fork: \n");
   dumpForkData(&file->resourceFork);
}

void 
dumpFolder(HFSPlusCatalogFolder *folder) {
   printf(" folder id:      %d\n", folder->folderID);
}

void 
dumpThread(HFSPlusCatalogThread *thread) {
   char *nodeName = HFSUniStr255ToCString(&thread->nodeName);
   printf(" parent id: %d\n", thread->parentID);
   printf(" node name: %s\n", nodeName);
}


void 
dumpCatalogRecords(char *node, BTNodeDescriptor *desc) {
   int i = 0;
   int firstOffset = volume.catalogHeader->nodeSize-2;
   
   for (i = 0; i < desc->numRecords; i++) {
      u_int16_t offset = CFSwapInt16BigToHost(*(u_int16_t*)(node+firstOffset));
      char *kind;
      HFSPlusCatalogKey key;

      key.keyLength = CFSwapInt16BigToHost(*(u_int16_t*)(node+offset));
      key.parentID = CFSwapInt16BigToHost(*(u_int32_t*)(node+offset+2));
      key.nodeName.length = CFSwapInt16BigToHost(*(u_int16_t*)(node+offset+6));
      memcpy(key.nodeName.unicode, node+offset+8, key.nodeName.length*2);

      char *objName = HFSUniStr255ToCString(&key.nodeName);
      sint16 recType =
         CFSwapInt16BigToHost(*(sint16*)(node+offset+key.keyLength+2));
   
      switch (recType) {
         case kHFSPlusFolderRecord:
            kind = "kHFSFolderRecord";
            break;
         case kHFSPlusFileRecord:
            kind = "kHFSFileRecord";
            break;
         case kHFSPlusFolderThreadRecord:
            kind = "kHFSFolderThreadRecord";
            break;
         case kHFSPlusFileThreadRecord:
            kind = "kHFSFileThreadRecord";
            break;
         default:
            kind = "undefined";
      }
      
      if (recType == kHFSPlusFileRecord) {
         printf("%s\n", objName);
         HFSPlusCatalogFile *file = 
            (HFSPlusCatalogFile*)(node+offset+key.keyLength+2);
         convertHFSPlusCatalogFileToHostByteOrder(file);
         dumpFile(file);
      }
      
      firstOffset-=2;
   }

}

void 
dumpCatalogFile() {
   u_int32_t blockSize = volume.volHeader.blockSize;
   u_int32_t startBlock = volume.volHeader.catalogFile.extents[0].startBlock;
   u_int32_t descOffset = blockSize *startBlock;
   char *node = (char*)malloc(142);

   readNode(descOffset, node, 142);
   
   BTNodeDescriptor *desc = (BTNodeDescriptor*)&node[0];
   volume.catalogHeader = (BTHeaderRec*)&node[14];
   convertNodeDescriptorToHostByteOrder(desc);
   convertHeaderRecordToHostByteOrder(volume.catalogHeader);

   dumpNodeDescriptor(desc);
   dumpHeaderRecord(volume.catalogHeader);
   
   node = (char*)malloc(volume.catalogHeader->nodeSize);
   
   u_int32_t nextLeafNode = volume.catalogHeader->firstLeafNode;
   
   while (nextLeafNode) {
      readCatalogNode(nextLeafNode, node);
      desc = (BTNodeDescriptor*)&node[0];
      convertNodeDescriptorToHostByteOrder(desc);
      dumpNodeDescriptor(desc);
      dumpCatalogRecords(node, desc);
      nextLeafNode = desc->fLink;
   }
}

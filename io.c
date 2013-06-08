/*
 *  io.c
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

#include "io.h"

HFSPlusVolume volume;


void 
openVolume(const char *const dev, u_int64_t volOffset) {
   if ((volume.device = fopen(dev, "r")) == NULL) {
      perror("fopen");
      exit(1);
   }
   
   volume.volOffset = volOffset;
   
   readVolumeHeader();
	
   if (volume.volHeader.signature != kHFSPlusSigWord 
         || volume.volHeader.version != kHFSPlusVersion) {
      fprintf(stderr, "invalid volume header");
      exit(1);
   }
}

void 
readVolumeHeader() {
   if (fseek(volume.device, 
            volume.volOffset + VOL_HEADER_OFFSET, SEEK_SET) == -1) {
      perror("unable to seek for volume header");
      exit(1);
   }

   if (fread(&volume.volHeader, sizeof(HFSPlusVolumeHeader), 1, 
            volume.device) != 1) {
      perror("unable to read volume header");
      exit(1);
   }
   
   volume.volHeader.signature = 
      CFSwapInt16BigToHost(volume.volHeader.signature);
   volume.volHeader.version = 
      CFSwapInt16BigToHost(volume.volHeader.version);
   volume.volHeader.fileCount =
      CFSwapInt32BigToHost(volume.volHeader.fileCount);
   volume.volHeader.folderCount = 
      CFSwapInt32BigToHost(volume.volHeader.folderCount);
   volume.volHeader.blockSize = 
      CFSwapInt32BigToHost(volume.volHeader.blockSize);
   volume.volHeader.totalBlocks =
      CFSwapInt32BigToHost(volume.volHeader.totalBlocks);
   volume.volHeader.freeBlocks = 
      CFSwapInt32BigToHost(volume.volHeader.freeBlocks);
   
   convertFileToHostByteOrder(&volume.volHeader.allocationFile);
   convertFileToHostByteOrder(&volume.volHeader.extentsFile);
   convertFileToHostByteOrder(&volume.volHeader.catalogFile);
}

void 
readNodeDescriptor(const u_int32_t offset, BTNodeDescriptor *const desc) {
   if (fseek(volume.device, volume.volOffset+offset, SEEK_SET) == -1) {
      perror("unable to seek for node descriptor");
      exit(1);
   }
   
   if (fread(desc, sizeof(BTNodeDescriptor), 1, volume.device) != 1) {
      perror("unable to read  node descriptor");
      exit(1);
   }
   
   desc->fLink = CFSwapInt32BigToHost(desc->fLink);
   desc->bLink = CFSwapInt32BigToHost(desc->bLink);
   desc->numRecords = CFSwapInt16BigToHost(desc->numRecords);
}

void 
readHeaderRecord(const u_int32_t offset, BTHeaderRec *const headerRec) {
   if (fseek(volume.device, volume.volOffset+offset, SEEK_SET) == -1) {
      perror("unable to seek for header record");
      exit(1);
   }

   if (fread(headerRec, sizeof(BTHeaderRec), 1, volume.device) != 1) {
      perror("unable to read header record");
      exit(1);
   }
}

void 
readNode(const u_int64_t offset, char *const node, u_int32_t nodeSize) {
   u_int64_t off = offset + volume.volOffset;
   
   if (fsetpos(volume.device, (fpos_t*)&off) == -1) {
      perror("unable to seek for node");
      exit(1);
   }
   
   if (fread(node, nodeSize, 1, volume.device) != 1) {
      perror("unable to read node");
      exit(1);
   }
}

u_int64_t 
calculateCatalogOffset(const u_int32_t nodeNum) {
   u_int32_t nodeOffset = nodeNum * volume.catalogHeader->nodeSize;
   u_int32_t block = nodeOffset / volume.volHeader.blockSize;
   u_int32_t offsetInBlock = nodeOffset % volume.volHeader.blockSize;
   int i = 0, b = block;
   
   while (volume.volHeader.catalogFile.extents[i].blockCount != 0) {
      if (b <= volume.volHeader.catalogFile.extents[i].blockCount) {
         u_int64_t offset = 
            (u_int64_t)(volume.volHeader.catalogFile.extents[i].startBlock+b) 
            * volume.volHeader.blockSize + offsetInBlock;
         return offset;
      }
      b -= volume.volHeader.catalogFile.extents[i].blockCount;
      i++;
   }
   
   printf("block not found. searching in overflow not yet supported\n");
   exit(1);
}

void 
readCatalogNode(const u_int32_t nodeNum, char *const node) {
   u_int64_t offset = calculateCatalogOffset(nodeNum);
   readNode(offset, node, volume.catalogHeader->nodeSize);
}

void 
copyFile(const file *const f, const char *const dstFileName) {
   HFSPlusCatalogFile *hfsFile = (HFSPlusCatalogFile*)f->hfsFile;
   FILE *dstData, *dstRsrc;
   int dataBlocks = 0, rsrcBlocks = 0, e;
   
   for (e = 0; e < 8; e++) {
      dataBlocks += hfsFile->dataFork.extents[e].blockCount;
      rsrcBlocks += hfsFile->resourceFork.extents[e].blockCount;
		if (dataBlocks == hfsFile->dataFork.totalBlocks 
            && rsrcBlocks == hfsFile->resourceFork.totalBlocks) {
			break;
		}
   }
   
   if (dataBlocks != hfsFile->dataFork.totalBlocks && f->dataExtents == NULL 
         || rsrcBlocks != hfsFile->resourceFork.totalBlocks 
         && f->rsrcExtents == NULL) {
      fprintf(stderr, "inconsistency in file extents: %s\n", dstFileName);
      return;
   }
   
	printf("restoring file: %d - %s\n", f->fileID, dstFileName);
	
   /* open the file in any case. either there is a data fork
    * or an empty file must be present to write its resource fork
    */
   if ((dstData = fopen(dstFileName, "w") ) == NULL) {
      perror("fopen");
      fprintf(stderr, "failed to restore: %s\n", dstFileName);
      return;
   }
   
   if (hfsFile->dataFork.totalBlocks > 0) {
      if (copyFork(&hfsFile->dataFork, f->dataExtents, dstData) == -1) {
			fprintf(stderr, "failed to restore: %s\n", dstFileName);
			fclose(dstData);
			return;
		}
   }
   
   fclose(dstData);
   
   if (hfsFile->resourceFork.totalBlocks > 0) {
      int fLen = strlen(dstFileName);
      char *rsrcFileName = (char*)calloc(RSRC_FORK_NAME_LEN+fLen+1, 1);
      memcpy(rsrcFileName, dstFileName, fLen);
      memcpy(rsrcFileName+fLen, RSRC_FORK_NAME, RSRC_FORK_NAME_LEN);
   
      if ((dstRsrc = fopen(rsrcFileName, "w") ) == NULL) {
         perror("fopen");
         fprintf(stderr, "failed to restore resource fork of: %s\n", 
               dstFileName);
         return;
      }

      if (copyFork(&hfsFile->resourceFork, f->rsrcExtents, dstRsrc) == -1) {
			fprintf(stderr, "failed to restore: %s\n", dstFileName);
			fclose(dstRsrc);
			free(rsrcFileName);
			return;
		}
		
      fclose(dstRsrc);
      free(rsrcFileName);
   }
   
   setFinderInfo(dstFileName, hfsFile);
	HFSPlusBSDInfo bsdInfo = hfsFile->bsdInfo;
	chmod(dstFileName, bsdInfo.fileMode);
}

int 
copyExtent(char *const buf, const HFSPlusExtentDescriptor *const desc, 
      FILE *const dst, int isLastExtent, u_int64_t partialBlockSize) {
   u_int32_t blockSize = volume.volHeader.blockSize;
   u_int32_t block = desc->startBlock;
   u_int32_t count = desc->blockCount;
   u_int64_t offset = (u_int64_t)block*blockSize + volume.volOffset;
      
   while (count-- > 0) {
      if (fsetpos(volume.device, (fpos_t*)&offset) == -1) {
			fprintf(stderr, "%s:%d unable to seek for file\n", __FILE__, __LINE__);
         return -1;
      }
         
      if (fread(buf, blockSize, 1, volume.device) != 1) {
			fprintf(stderr, "%s:%d unable to read file\n", __FILE__, __LINE__);
			int err = ferror(volume.device);

			if (err) {
				fprintf(stderr, "%s:%d error=%d\n", __FILE__, __LINE__, err);
				clearerr(volume.device);
			} else if (feof(volume.device)) {
				fprintf(stderr, "%s:%d premature end-of-file\n", __FILE__, 
                  __LINE__);
			}

         return -1;
      }
         
      fwrite(buf, count == 0 && isLastExtent ? partialBlockSize : blockSize, 1, dst);
      block++;
      offset += blockSize;
   }
	
	return 0;
}

int 
copyFork(const HFSPlusForkData *const fork, const orderedlist *extList, 
      FILE *const dst) {
   u_int32_t blockSize = volume.volHeader.blockSize;
   u_int64_t partialBlockSize = fork->logicalSize % blockSize;
	u_int32_t remainingBlocks = fork->totalBlocks;
   ol_node *extents = extList != NULL ? extList->head : NULL;
   int isLastExtent;
   int i;
   char *buf = (char*)malloc(blockSize);
   
   for (i = 0; i < 8; i++) {
      if (fork->extents[i].blockCount == 0) {
         break;
      }

      isLastExtent = (i == 7 && extents == NULL 
            || fork->extents[i+1].blockCount == 0);

      if (copyExtent(buf, (HFSPlusExtentDescriptor*)(&fork->extents[i]), dst, 
               isLastExtent, partialBlockSize) == -1) {
			fprintf(stderr, "%s:%d copyExtent failed\n", __FILE__, __LINE__);
			free(buf);
			return -1;
		}
		
		remainingBlocks -= fork->extents[i].blockCount;
   }
   
   while (extents != NULL) {
      HFSPlusExtentRecord *rec = extents->value;
      for( i = 0; i < 8; i++ ) {
         if( rec[i]->blockCount == 0 || remainingBlocks == 0 ) {
            break;
         }

         isLastExtent = i == 7 && extents->next == NULL 
            || rec[i+1]->blockCount == 0 
            || remainingBlocks - rec[i]->blockCount == 0;
			
			if (((HFSPlusExtentDescriptor*)(rec[i]))->blockCount > remainingBlocks) {
				fprintf(stderr, "%s:%d something's wrong here: we expect %d blocks,"
                  " the extent apparently contains %d blocks\n", 
                  __FILE__, __LINE__, remainingBlocks, 
                  ((HFSPlusExtentDescriptor*)(rec[i]))->blockCount);
				free(buf);
				return -1;
			}
			
         if (copyExtent(buf, (HFSPlusExtentDescriptor*)(rec[i]), dst, 
                  isLastExtent, partialBlockSize) == -1) {
				fprintf(stderr, "%s:%d copyExtent failed\n", __FILE__, __LINE__);
				free(buf);
				return -1;
			}

			remainingBlocks -=  rec[i]->blockCount;
      }

      extents = extents->next;
   }
   
   free(buf);
	return 0;
}

void 
setFinderInfo(const char *const fileName, const FndrFileInfo *const info) {
   FInfoAttrBuf attrBuf;
   attrlist_t *attrList = (attrlist_t*)calloc(sizeof(attrlist_t), 1);

   attrList->bitmapcount = ATTR_BIT_MAP_COUNT;
   attrList->commonattr = ATTR_CMN_FNDRINFO;
   
   if (getattrlist(fileName, attrList, &attrBuf, sizeof(attrBuf), 0) ) {
      perror("getattrlist");
      fprintf(stderr, "couldn't get attributes of: %s\n", fileName);
   } else {
      memcpy(&attrBuf.finderInfo[0], &info->fdType, 4);
      memcpy(&attrBuf.finderInfo[4], &info->fdCreator, 4);
		memcpy(&attrBuf.finderInfo[8], &info->fdFlags, 2);
		
      attrList->commonattr = ATTR_CMN_FNDRINFO;
      
      if (setattrlist(fileName, attrList, attrBuf.finderInfo, 
               sizeof(attrBuf.finderInfo), 0)) {
         perror("setattrlist");
         fprintf(stderr, "couldn't set attributes of: %s\n", fileName);
      }
   }
	
   free(attrList);
}

void 
sequentiallyReadCatalog(int(*filter)(HFSPlusCatalogKey*, sint16), 
                   void(*handler)(HFSPlusCatalogKey*, sint16, void*)) {
   u_int32_t blockSize = volume.volHeader.blockSize;
   u_int32_t startBlock = volume.volHeader.catalogFile.extents[0].startBlock;
   u_int32_t descOffset = blockSize * startBlock;

   char *headerNode = (char*)malloc(142);
   readNode(descOffset, headerNode, 142);
   
   BTNodeDescriptor *desc = (BTNodeDescriptor*)&headerNode[0];
   volume.catalogHeader = (BTHeaderRec*)&headerNode[14];
   convertNodeDescriptorToHostByteOrder(desc);
   convertHeaderRecordToHostByteOrder(volume.catalogHeader);
   
   u_int32_t nodeBlockRatio = volume.catalogHeader->nodeSize / blockSize;
   
   char *node = (char*)malloc(volume.catalogHeader->nodeSize);
   BTNodeDescriptor *nodeDesc;
   int i;
   
   for (i = 0; i < 8; i++) {
      u_int32_t blockCount = volume.volHeader.catalogFile.extents[i].blockCount;
      u_int32_t block = volume.volHeader.catalogFile.extents[i].startBlock;
      
      while (blockCount > 0) {
         u_int64_t offset = (u_int64_t)block*blockSize + volume.volOffset;
         
         if (fsetpos(volume.device, (fpos_t*)&offset) == -1) {
            perror("unable to seek");
            exit(1);
         }
         
         if (fread(node, nodeBlockRatio*blockSize, 1, volume.device) != 1) {
            perror("unable to read");
            exit(1);
         }
         
         nodeDesc = (BTNodeDescriptor*)&node[0];
         convertNodeDescriptorToHostByteOrder(nodeDesc);
         
         if (nodeDesc->kind == kBTLeafNode) {
            iterateOverCatalogRecords(node, nodeDesc, filter, handler);
         }
         
         blockCount -= nodeBlockRatio;
         block += nodeBlockRatio;
      }
   }
}

void 
readHeaderNode(u_int32_t offset, BTHeaderRec **header) {
   char *headerNode = (char*)malloc(142);
   readNode(offset, headerNode, 142);
   
   BTNodeDescriptor *desc = (BTNodeDescriptor*)&headerNode[0];
   *header = (BTHeaderRec*)&headerNode[14];
   convertNodeDescriptorToHostByteOrder(desc);
   convertHeaderRecordToHostByteOrder(*header);
}

void 
sequentiallyReadExtents(
      void(*handler)(HFSPlusExtentKey*, HFSPlusExtentRecord*)) {
   u_int32_t blockSize = volume.volHeader.blockSize;
   u_int32_t startBlock = volume.volHeader.extentsFile.extents[0].startBlock;
   u_int32_t descOffset = blockSize * startBlock;

   readHeaderNode(descOffset, &volume.extentsHeader);
   
   u_int32_t nodeBlockRatio = volume.extentsHeader->nodeSize / blockSize;
   
   char *node = (char*)malloc(volume.extentsHeader->nodeSize);
   BTNodeDescriptor *nodeDesc;
   int i;
   
   for (i = 0; i < 8; i++) {
      u_int32_t blockCount = volume.volHeader.extentsFile.extents[i].blockCount;
      u_int32_t block = volume.volHeader.extentsFile.extents[i].startBlock;
      
      while (blockCount > 0) {
         u_int64_t offset = (u_int64_t)block*blockSize + volume.volOffset;
         
         if (fsetpos(volume.device, (fpos_t*)&offset) == -1) {
            perror("unable to seek");
            exit(1);
         }
         
         if (fread(node, nodeBlockRatio*blockSize, 1, volume.device) != 1) {
            perror("unable to read");
            exit(1);
         }
         
         nodeDesc = (BTNodeDescriptor*)&node[0];
         convertNodeDescriptorToHostByteOrder(nodeDesc);
         
         if (nodeDesc->kind == kBTLeafNode) {
            iterateOverExtentsRecords(node, nodeDesc, handler);
         }
         
         blockCount -= nodeBlockRatio;
         block += nodeBlockRatio;
      }
   }
   
}


#pragma mark === iterators ===

void 
iterateOverCatalogRecords(char *node, BTNodeDescriptor *desc, 
      int(*filter)(HFSPlusCatalogKey*, sint16), 
      void(*handler)(HFSPlusCatalogKey*, sint16, void*) ) {
   int i = 0;
   int firstOffset = volume.catalogHeader->nodeSize-2;
   
   for (i = 0; i < desc->numRecords; i++) {
      u_int16_t keyOffset = CFSwapInt16BigToHost(*(u_int16_t*)(node+firstOffset));
      
      if (i == 0 && keyOffset != FIRST_KEY_OFFSET) {
         fprintf(stderr, "invalid offset to first record in catalog node: "
               "expected 14, found %d\n", keyOffset);
         return;
      }
      
      HFSPlusCatalogKey key;
      key.keyLength = CFSwapInt16BigToHost(*(u_int16_t*)(node+keyOffset));
      key.parentID = CFSwapInt32BigToHost(*(u_int32_t*)(node+keyOffset+2));
      key.nodeName.length = CFSwapInt16BigToHost(*(u_int16_t*)(node+keyOffset+6));
      
      if (key.keyLength > volume.catalogHeader->maxKeyLength) {
         fprintf(stderr, "invalid key length in catalog record: %d\n", 
               key.keyLength);
         continue;
      }
      
      if (key.nodeName.length * 2 + 6 != key.keyLength) {
         fprintf(stderr, "inconsistency in catalog record key found: "
               "key length %d vs. %d\n", key.nodeName.length * 2 + 6, 
               key.keyLength);
         char *keyPart = (char*)calloc(21, 1);
         memcpy(keyPart, node+keyOffset+8, 20);
         fprintf(stderr, "possible key starts with: %s\n", keyPart);
         free(keyPart);
         continue;
      }
      
      memcpy(key.nodeName.unicode, node+keyOffset+8, key.nodeName.length*2);
      u_int16_t recOffset = keyOffset+key.keyLength+2;
      sint16 recType = CFSwapInt16BigToHost(*(sint16*)(node+recOffset));
   
      if ((*filter)(&key, recType)) {
         void *record = node+recOffset;

         switch (recType) {
            case kHFSPlusFileRecord:
               convertHFSPlusCatalogFileToHostByteOrder(
                     (HFSPlusCatalogFile*)record);
               break;
            case kHFSPlusFolderRecord:
               convertHFSPlusCatalogFolderToHostByteOrder(
                     (HFSPlusCatalogFolder*)record);
               break;
            case kHFSPlusFileThreadRecord:
               convertHFSPlusCatalogThreadToHostByteOrder(
                     (HFSPlusCatalogThread*)record);
               break;
            case kHFSPlusFolderThreadRecord:
               convertHFSPlusCatalogThreadToHostByteOrder(
                     (HFSPlusCatalogThread*)record);
               break;
            default:
               fprintf(stderr, "unknown record type\n");
         }
         (*handler)(&key, recType, record);
      }
      
      firstOffset-=2;
   }
}

void 
iterateOverCatalog(int(*filter)(HFSPlusCatalogKey*, sint16), 
      void(*handler)(HFSPlusCatalogKey*, sint16, void*)) {
   u_int32_t blockSize = volume.volHeader.blockSize;
   u_int32_t startBlock = volume.volHeader.catalogFile.extents[0].startBlock;
   u_int32_t descOffset = blockSize * startBlock;

   char *headerNode = (char*)malloc(142);
   readNode(descOffset, headerNode, 142);
   
   BTNodeDescriptor *desc = (BTNodeDescriptor*)&headerNode[0];
   volume.catalogHeader = (BTHeaderRec*)&headerNode[14];
   convertNodeDescriptorToHostByteOrder(desc);
   convertHeaderRecordToHostByteOrder(volume.catalogHeader);
   
   char *node = (char*)malloc(volume.catalogHeader->nodeSize);
   
   u_int32_t nextLeafNode = volume.catalogHeader->firstLeafNode;
   
   while (nextLeafNode) {
      readCatalogNode(nextLeafNode, node);
      desc = (BTNodeDescriptor*)&node[0];
      convertNodeDescriptorToHostByteOrder(desc);
      iterateOverCatalogRecords(node, desc, filter, handler);
      nextLeafNode = desc->fLink;
   }
   
   free(headerNode);
   free(node);
}


void 
iterateOverExtentsRecords(char *node, BTNodeDescriptor *desc, 
      void(*handler)(HFSPlusExtentKey*, HFSPlusExtentRecord*)) {
   int i = 0;
   int firstOffset = volume.extentsHeader->nodeSize-2;
   
   for (i = 0; i < desc->numRecords; i++) {
      u_int16_t keyOffset = 
         CFSwapInt16BigToHost(*(u_int16_t*)(node+firstOffset));
      
      if (i == 0 && keyOffset != FIRST_KEY_OFFSET) {
         fprintf(stderr, "invalid offset to first record in extent overflow "
               "node: expected 14, found %d\n", keyOffset);
         return;
      }
      
      HFSPlusExtentKey key;
      key.keyLength = CFSwapInt16BigToHost(*(u_int16_t*)(node+keyOffset));
      key.forkType = *(u_int8_t*)(node+keyOffset+2);
      key.fileID = CFSwapInt32BigToHost(*(u_int32_t*)(node+keyOffset+4));
      key.startBlock = CFSwapInt32BigToHost(*(u_int32_t*)(node+keyOffset+8));
      
		if (key.keyLength != kHFSPlusExtentKeyMaximumLength) {
         fprintf(stderr, "invalid key length in extent overflow record: %d\n", 
               key.keyLength);
      } else {
         u_int16_t recOffset = keyOffset+key.keyLength+2;
         HFSPlusExtentRecord *record = (HFSPlusExtentRecord*)(node+recOffset);
         convertHFSPlusExtentRecordToHostByteOrder(record);
			(*handler)(&key, record);
		}
      
      firstOffset -= 2;
   }
}

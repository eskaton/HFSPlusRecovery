/*
 *  rec_handler.c
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

#include "rec_handler.h"
#include "util.h"


void 
dumpRecordType(HFSPlusCatalogKey *key, sint16 recType, void *record) {
   char *kind;
      
   switch (recType) {
      case kHFSPlusFolderRecord:
         kind = "kHFSPlusFolderRecord";
         break;
      case kHFSPlusFileRecord:
         kind = "kHFSPlusFileRecord";
         break;
      case kHFSPlusFolderThreadRecord:
         kind = "kHFSPlusFolderThreadRecord";
         break;
      case kHFSPlusFileThreadRecord:
         kind = "kHFSPlusFileThreadRecord";
         break;
      default:
         kind = "undefined";
   }
   
   printf("record type: %s\n", kind);
}

void 
dumpRecord(HFSPlusCatalogKey *key, sint16 recType, void *record) {
   char *name = HFSUniStr255ToCString(&key->nodeName);
         
   printf("%d %s:\n", key->parentID, name);
            
   switch (recType) {
      case kHFSPlusFolderRecord:
         dumpFolder(record);
         break;
      case kHFSPlusFileRecord:
         dumpFile(record);
         break;
      case kHFSPlusFolderThreadRecord:
         dumpThread(record);
         break;
      case kHFSPlusFileThreadRecord:
         dumpThread(record);
         break;
      default:
         /* shouldn't happen */;
   }
}

void 
fileRecordDump(HFSPlusCatalogKey *key, sint16 recType, void *file) {
   char *fileName = HFSUniStr255ToCString(&key->nodeName);
   file = (HFSPlusCatalogFile*)file;
   printf("%s:\n", fileName);
   dumpFile(file);
}

void 
fileRecordHandler(HFSPlusCatalogKey *key, sint16 recType, void *file) {
   char *fileName = HFSUniStr255ToCString(&key->nodeName);
   printf("%s:\n", fileName);
   dumpFile(file);
   char *dstFileName = (char*)calloc(1, 5 + strlen(fileName) + 1);

   memcpy(dstFileName, "/tmp/", 5);
   memcpy(dstFileName+5, fileName, strlen(fileName));
   
   HFSCatalogNodeID id = ((HFSPlusCatalogFile*)file)->fileID;
   printf("%lx: %s\n", id, dstFileName);
   
   copyFile(file, dstFileName);
   free(fileName);
   free(dstFileName);
}

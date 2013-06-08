/*
 *  main.c
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
#include "util.h"
#include "byteorder.h"
#include "io.h"
#include "rec_filter.h"
#include "rec_handler.h"
#include "btree.h"
#include "orderedlist.h"
#include "definitions.h"
#include "memory.h"

extern HFSPlusVolume volume;

static char *recoveryPath;
static int recoveryPathLen;
static const char *const lostPath = "/lost+found";
static const int lostPathLen = 11;
static const short maxCnidLen = 10;

static btree *folders;
static btree *files;
static btree *extents;


int 
folderThreadKeyComparator(void *key1, void *key2) {
   return strcmp((char*)key1, (char*)key2);
}

int 
CNIDComparator(void *key1, void *key2) {
   long k1 = *(long*)key1;
   long k2 = *(long*)key2;

   if( k1 < k2 ) return -1;
   if( k1 > k2 ) return  1;
   return 0;
}

int 
startBlockComparator(void *key1, void *key2) {
   u_int32_t k1 = *(u_int32_t*)key1;
   u_int32_t k2 = *(u_int32_t*)key2;

   if( k1 < k2 ) return -1;
   if( k1 > k2 ) return  1;
   return 0;
}

void 
addFileRecord(HFSPlusCatalogKey *key, sint16 recType, void *fileRec) {
   file *f = (file*)calloc(1, sizeof(file));
   f->parentID = key->parentID;
   f->fileID = ((HFSPlusCatalogFile*)fileRec)->fileID;
   f->name = HFSUniStr255ToCString(&key->nodeName);
   f->hfsFile = (HFSPlusCatalogFile*)malloc(sizeof(HFSPlusCatalogFile));
   memcpy(f->hfsFile, fileRec, sizeof(HFSPlusCatalogFile));
   btree_insert(files, &f->fileID, f);
}

void 
addFolderRecord(HFSPlusCatalogKey *key, sint16 recType, void *folderRec) {
   folder *fldr = (folder*)calloc(1, sizeof(folder));
   fldr->parentID = key->parentID;
   fldr->folderID = ((HFSPlusCatalogFolder*)folderRec)->folderID;
   fldr->name = HFSUniStr255ToCString(&key->nodeName);
   btree_insert(folders, &fldr->folderID, fldr);
}

void 
addFolderAndFileRecord(HFSPlusCatalogKey *key, sint16 recType, void *fileRec) {
   if (recType == kHFSPlusFileRecord) {
      addFileRecord(key, recType, fileRec);
   } else {
      addFolderRecord(key, recType, fileRec);
   }
}

void 
dumpExtentRecord(HFSPlusExtentKey *key, HFSPlusExtentRecord *record) {
   int i;
   HFSPlusExtentDescriptor *desc = (HFSPlusExtentDescriptor*)record;
   
   printf("file-id:    %d\n", key->fileID);
   printf("fork type:   %lx\n", key->forkType);
   printf("start block: %d\n", key->startBlock);
   
   for (i = 0; i < 8; i++) {
      if (desc->startBlock != 0 || desc->blockCount != 0) {
         printf("          %d     %-11d      %-11d\n", i, desc->startBlock, 
               desc->blockCount);
      } else {
         break;
      }
      desc++;
   }
}

void 
addExtentRecord(HFSPlusExtentKey *key, HFSPlusExtentRecord *record) {
   u_int32_t *listKey = (u_int32_t*)malloc(sizeof(u_int32_t));
   extentKey *treeKey = (extentKey*)malloc(sizeof(extentKey));
   HFSPlusExtentRecord *r = 
      (HFSPlusExtentRecord*)malloc(sizeof(HFSPlusExtentRecord));
   orderedlist *list;
   
   *listKey = key->startBlock;
   treeKey->fileID = key->fileID;
   treeKey->forkType = key->forkType;
   memcpy(r, record, sizeof(HFSPlusExtentRecord));   
   btree_node *listNode = (btree_node*)btree_find(extents, treeKey);

   if (listNode == NULL) {
      list = ol_create(&startBlockComparator);
      btree_insert(extents, treeKey, list);
   } else {
      list = (orderedlist*)listNode->value;
   }
   
   ol_insert(list, listKey, r);
}

void 
dumpFolderNode(btree_node *node) {
   folder *fldr = (folder*)node->value;
   printf("%d > %d: %s\n", fldr->parentID, *(long*)node->key, fldr->name);
}

void 
dumpFileNode(btree_node *node) {
   file *f = (file*)node->value;
   printf("%d > %d: %s/%s\n", f->parentID, *(long*)node->key, 
         f->path != NULL ? f->path : "", f->name);
}


void 
dumpFolderThreadNode(btree_node *node) {
   u_int32_t parentID = *(u_int32_t*)node->value;
   char *folder = (char*)node->key;
   printf("%s: %d\n", folder, parentID);
}

void 
linkFolderToParent(btree_node *node) {
   long key = ((folder*)node->value)->parentID;
   btree_node *parentNode = btree_find(folders, &key);

   if (parentNode != NULL) {
      ((folder*)(node->value))->parent = parentNode->value;
   } else if (((folder*)node->value)->parentID != kHFSRootParentID) {
      fprintf(stderr, 
            "orphan folder found: Parent-CNID=%d / CNID=%d / Name=%s\n", 
            ((folder*)node->value)->parentID, ((folder*)node->value)->folderID, 
            ((folder*)node->value)->name);
   }
}


void 
linkFilesToParent(btree_node *node) {
   long key = ((file*)node->value)->parentID;
   btree_node *parentNode = btree_find(folders, &key);

   if (parentNode != NULL) {
      ((file*)(node->value))->parent = parentNode->value;
   } else {
      fprintf(stderr, "orphan file found: Parent-CNID=%d / CNID=%d / Name=%s\n", 
            ((file*)(node->value))->parentID, ((file*)(node->value))->fileID, 
            ((file*)(node->value))->name);
   }
}

void 
linkExtentsToFile(btree_node *node) {
   extentKey *key = (extentKey*)node->key;
   orderedlist *value = (orderedlist*)node->value;
   btree_node *fileNode = btree_find(files, &key->fileID);
   file* f;
   
   if (fileNode == NULL) {
      fprintf(stderr, "file %d has extents but no catalog entry\n", 
            key->fileID);
      return;
   }
   
   f = (file*)fileNode->value;
   
   if (key->forkType == 0x00) {
      f->dataExtents = value;
   } else {
      f->rsrcExtents = value;
   }
}

void 
buildPath(btree_node *node) {
   int initialSize = 8;
   int growthFactor = 2;
   int currentSize = initialSize;
   int top = 0;
   long key = ((file*)node->value)->parentID;
   btree_node *fldrNode = btree_find(folders, &key);
   folder *fldr;
   char *pathName;
   int i, pathLen = 0;
   
   if (fldrNode != NULL) {
      char** folderStack = (char**)calloc(initialSize, sizeof(char*));
      fldr = (folder*)fldrNode->value;
      folderStack[top++] = fldr->name;
   
      while ((fldr = (folder*)fldr->parent) != NULL) {
         if (top == currentSize) {
            if ((folderStack = (char**)realloc(folderStack, 
                        (currentSize *= growthFactor)*sizeof(char*))) == NULL) {
               perror("realloc");
               exit(1);
            }
         }

         folderStack[top++] = fldr->name;
      }
      
      for (i = 0; i < top; i++) {
         pathLen += strlen(folderStack[i]);
      }
      
      pathLen += top + 1;
      pathName = (char*)malloc(pathLen);
      pathName[0] = '/';
      pathName[pathLen-1] = '\0';
      strrjoin(folderStack, top-1, '/', pathName+1);
      free(folderStack);
      
      ((file*)node->value)->path = pathName;
   }
}

void 
listFilesWithExtends(btree_node *node) {
   file *f = (file*)node->value;
   HFSPlusCatalogFile *hfsFile = (HFSPlusCatalogFile*)f->hfsFile;
   int dataBlocks = 0, rsrcBlocks = 0, e;
   
   for (e = 0; e < 8; e++) {
      dataBlocks += hfsFile->dataFork.extents[e].blockCount;
      rsrcBlocks += hfsFile->resourceFork.extents[e].blockCount;

      if (dataBlocks == hfsFile->dataFork.totalBlocks && rsrcBlocks == hfsFile->resourceFork.totalBlocks) {
         break;
      }
   }
   
   if (dataBlocks != hfsFile->dataFork.totalBlocks && f->dataExtents == NULL 
         || rsrcBlocks != hfsFile->resourceFork.totalBlocks 
         && f->rsrcExtents == NULL) {
      fprintf(stderr, "inconsistency in file extents: %s\n", f->path);
      return;
   }
   
   if (dataBlocks == hfsFile->dataFork.totalBlocks 
         && rsrcBlocks == hfsFile->resourceFork.totalBlocks) {
      /* ignore files without extents */
      return;
   }
   
   printf("%s\n", f->name);
}

char *
concatPath(const char *const p1, const char *const p2) {
   int len1 = strlen(p1);
   int len2 = strlen(p2);
   char *path = (char*)malloc(len1+len2+2);
   memcpy(path, p1, len1);
   path[len1] = '/';
   memcpy(path+len1+1, p2, len2);
   path[len1+len2+1] = '\0';
   return path;
}

char *
concat(const char *const s1, const char *const s2) {
   int len1 = strlen(s1);
   int len2 = strlen(s2);
   char *str = (char*)malloc(len1+len2+1);
   memcpy(str, s1, len1);
   memcpy(str+len1, s2, len2);
   str[len1+len2] = '\0';
   return str;
}

int 
restoreFile(file *f, char *path) {
   char *dstFile;
   /*TODO: apply the original mask*/
   mode_t mask = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

   if (mkdirr(path, mask) != 0 && errno != EEXIST) {
      fprintf(stderr, "couldn't create path (errno=%d): %s\n", errno, path);
      return -1;
   } else {
      dstFile = concatPath(path, f->name);
      copyFile(f, dstFile);
      free(dstFile);
   }
   
   return 0;
}

void 
restore(btree_node *node) {
   file *f = (file*)node->value;
   char *path, *tmpPath, *cnidStr;
   int error = 0;

   if (f->path != NULL) {
      path = concat(recoveryPath, f->path);
      
      if (restoreFile(f, path) == -1) {
         error = 1;
      }
      
      free(path);
   }
   
   if (f->path == NULL || error) {
      cnidStr = (char*)malloc(maxCnidLen+1);
      sprintf(cnidStr, "%d", f->parentID);
      tmpPath = concat(recoveryPath, lostPath);
      path = concatPath(tmpPath, cnidStr);
      free(tmpPath);
      free(cnidStr);
   
      if (restoreFile(f, path) == -1) {
         fprintf(stderr, "unable to restore file: %s\n", f->name);
      }
      
      free(path);
   }
}

void 
recovery() {
   printf("building folder and file tree from catalog\n");
   sequentiallyReadCatalog(&folderAndFileRecordFilter, &addFolderAndFileRecord);
   printf("node count in folder tree: %d\n", folders->nodeCount);
   printf("node count in file tree: %d\n", files->nodeCount);
   printf("link all folders to their parents\n");
   btree_inorderTraverse(folders, &linkFolderToParent);
   printf("link all files to their parent folders\n");
   btree_inorderTraverse(files, &linkFilesToParent);
   printf("determine path of files\n");
   btree_inorderTraverse(files, &buildPath);
   printf("building tree from extent overflow file\n");
   sequentiallyReadExtents(&addExtentRecord);
   printf("node count in extent overflow tree: %d\n", extents->nodeCount);
   printf("linking overflow extents to files\n");
   btree_inorderTraverse(extents, &linkExtentsToFile);
   printf("restoring files...\n");
   btree_inorderTraverse(files, &restore);
   printf("finished\n");

}

int 
main (int argc, const char * argv[]) {

   if (argc < 3 || argc > 4) {
      fprintf(stderr, "usage: %s <device> <recovery-path> [<offset>]\n", argv[0]);
      exit(1);
   }
   
   char *device = (char *)argv[1];

   if (*(char *)argv[2] != '/') {
      char *cwd = getcwd(NULL);
      recoveryPath = concatPath(cwd, (char *)argv[2]);
      free(cwd);
   } else {
      recoveryPath = (char *)argv[2];
   }

   recoveryPathLen = strlen(recoveryPath);

   u_int64_t offset = argc == 4 ? atoll(argv[3]) : 0;

   folders = btree_create(&CNIDComparator);
   files = btree_create(&CNIDComparator);
   extents = btree_create(&CNIDComparator);
   
   openVolume(device, offset);
   dumpVolumeHeader();
   
   recovery();

   return 0;
}

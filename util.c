/*
 *  util.c
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
#include "memory.h"

char * 
HFSUniStr255ToCString(HFSUniStr255 *uniString) {
    char *cStr = (char*)malloc(uniString->length+1);
    int len = uniString->length;
    u_int16_t *p = (u_int16_t*)&uniString->unicode;
    int i;
    
    for (i = 0; i < len; i++) {
        u_int16_t c = CFSwapInt16BigToHost(*p);
        *(cStr+i) = (char)(c & 0xFF);
        p++;
    }

    cStr[uniString->length] = 0;

    return cStr;
}

int 
endsWith(char *str, char *pattern) {
    int patternLen = strlen(pattern);
    int strLen = strlen(str);
    int i, flag = 1;

    for (i = 0; i < patternLen; i++) {
        if (*(str+strLen-1-i) != *(pattern+patternLen-1-i)) {
            flag = 0;
            break;
        }
    }

    return flag;
}

int 
startsWith(char *str, char *pattern) {
    int patternLen = strlen(pattern);
    int i, flag = 1;

    for (i = 0; i < patternLen; i++) {
        if (*(str+i) != *(pattern+i)) {
            flag = 0;
            break;
        }
    }

    return flag;
}

void 
strjoin(char **stack, int stackSize, char glue, char *buf) {
    int i, pos = 0;
    
    for (i = 0; i <= stackSize; i++) {
        int len = strlen(stack[i]);
        memcpy(buf+pos, stack[i], len);
        pos += len;

        if (i < stackSize) {
            buf[pos++] = glue;
        }
    }
}

void 
strrjoin(char **stack, int stackSize, char glue, char *buf) {
    int i, pos = 0;
    
    for (i = stackSize; i >= 0; i--) {
        int len = strlen(stack[i]);
        memcpy(buf+pos, stack[i], len);
        pos += len;

        if (i > 0) {
            buf[pos++] = glue;
        }
    }
}

char 
**strsplit(const char *const str, char delim) {
    int initialSize = 4;
    int growthFactor = 2;
    int currentSize = initialSize;
    int top = 0;
    char **stack = (char**)malloc(initialSize*sizeof(char*));
    int i = 0;

    do {
        int j = 0;
        while (str[i] != delim && str[i] != '\0') {
            i++, j++;
        }
        
        if (j > 0) {
            stack[top] = (char*)malloc(j+1);
            stack[top][j] = '\0';
            memcpy(stack[top], str+i-j, j);
            top++;

            if (top == currentSize) {
                if ((stack = (char**)realloc(stack, (currentSize *= growthFactor) * 
                            sizeof(char*))) == NULL) {
                    perror("realloc");
                    exit(1);
                }
            }
        }
        
    } while(str[i] != '\0' && ++i);
    
    stack[top] = 0;
    
    return stack;
}

int 
strisascii(const char *str) { 
    while (*str != '\0' && isascii(*str++));
    return *str == 0 ? 1 : 0;
}

int 
mkdirr(char *path, mode_t mask) {
   char **dirParts = strsplit(path, '/');
   char *str = (char*)malloc(strlen(path)+2);
   int i = 0;
   int pos = 1;
   int len;

   *str = '/';
    
   while( dirParts[i] != 0 ) {
      len = strlen(dirParts[i]);
      memcpy(str+pos, dirParts[i], len);
      pos+=len;
      str[pos] = '\0';
      free(dirParts[i]);

      if (mkdir(str, mask) != 0 && errno != EEXIST) {
         free(str);
         while (dirParts[++i] != 0) {
            free(dirParts[i]);
         }
         free(str);
         free(dirParts);
         return -1;
      }
      str[pos++] = '/';
      str[pos] = '\0';
      i++;
   }
    
   free(str);
   free(dirParts);
   return 0;
}

/*
 *  orderedlist.h
 *  HFSPlusRecovery
 *
 *  Created by Adrian Moser on 01.09.08.
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

#ifndef __ORDEREDLIST_H_
#define __ORDEREDLIST_H_
 
typedef struct _ol_node {
    struct _ol_node *next;
    void *key;
    void *value;
} ol_node;


typedef struct _orderedlist {
    ol_node *head;
    long nodeCount;
    int(*keyComparator)(void *key1, void *key2);    
} orderedlist;

ol_node * 
ol_create_node();

void 
ol_destroy_node(ol_node *node);

orderedlist *
ol_create(int(*keyComparator)(void *key1, void *key2));

void 
ol_insert_node(orderedlist *list, ol_node *currentNode, ol_node *valueNode);

void 
ol_insert(orderedlist *list, void *key, void *value);

void 
ol_delete_node(orderedlist *list, ol_node *node, void *key);

void 
ol_delete(orderedlist *list, void *key);

ol_node *
ol_find_node(orderedlist *list, ol_node *node, void *key);

ol_node *
ol_find(orderedlist *list, void *key);

void 
ol_traverse_node(ol_node *node, void(*nodeHandler)(ol_node *node));

void 
ol_traverse(orderedlist *list, void(*nodeHandler)(ol_node *node));

#endif

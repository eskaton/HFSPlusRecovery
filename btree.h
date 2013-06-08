/*
 *  btree.h
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

#ifndef _BTREE_H_
#define _BTREE_H_

typedef struct _btree_node {
    struct _btree_node *lnode;
    struct _btree_node *rnode;
    void *key;
    void *value;
} btree_node;

typedef struct _btree {
    btree_node *root;
    long nodeCount;
    int(*keyComparator)(void *key1, void *key2);
} btree;

btree_node *
btree_create_node();

void 
btree_destroy_node(btree_node *node);

btree *
btree_create(int(*keyComparator)(void *key1, void *key2));

void 
btree_insert_node(btree *tree, btree_node *indexNode, btree_node *valueNode);

void 
btree_insert(btree *tree, void *key, void *value);

btree_node * 
btree_delete_node(btree *tree, btree_node **node, void *key);

btree_node * 
btree_delete(btree *tree, void *key);

btree_node * 
btree_find_node(btree *tree, btree_node *node, void *key);

btree_node *
btree_find(btree *tree, void *key);

void 
btree_inorderTraverse_nodeWithReturn(btree_node *node, 
      void(*nodeHandler)(btree_node *node, void *retVal), void *retVal);

void 
btree_inorderTraverseWithReturn(btree *tree, 
      void(*nodeHandler)(btree_node *node, void *retVal), void *retVal);

void 
btree_inorderTraverse_node(btree_node *node, 
      void(*nodeHandler)(btree_node *node));

void 
btree_inorderTraverse(btree *tree, void(*nodeHandler)(btree_node *node));

#endif

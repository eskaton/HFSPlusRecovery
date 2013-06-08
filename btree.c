/*
 *  btree.c
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

#include <CoreServices/CoreServices.h>
#include "btree.h"

btree_node * 
btree_create_node() {
    btree_node *node = (btree_node*)calloc(1, sizeof(btree_node));
    return node;
}

void 
btree_destroy_node(btree_node *node) {
   free(node);
}

btree *
btree_create(int(*keyComparator)(void *key1, void *key2)) {
    btree *tree = (btree*)calloc(1, sizeof(btree));
    tree->keyComparator = keyComparator;
    return tree;
}

void 
btree_insert_node(btree *tree, btree_node *indexNode, btree_node *valueNode) {
    btree_node **insertNode;
    
    if (tree->keyComparator(indexNode->key, valueNode->key) <= 0) {
        insertNode = &indexNode->rnode;
    } else {
        insertNode = &indexNode->lnode;
    }
    
    if (*insertNode == NULL) {
        *insertNode = valueNode;
        tree->nodeCount++;
    } else {
        btree_insert_node(tree, *insertNode, valueNode);
    }
}

void 
btree_insert(btree *tree, void *key, void *value) {

   if (key == NULL) {
      fprintf(stderr, "btree_insert: key may not be null\n");
      exit(1);
   }
   
   if (value == NULL) {
      fprintf(stderr, "btree_insert: value may not be null\n");
      exit(1);
   }

    btree_node *node = btree_create_node();
    node->key = key;
    node->value = value;
    
    if (tree->root == NULL) {
        tree->root = node;
        tree->nodeCount++;
    } else {
        btree_insert_node(tree, tree->root, node);
    }
}

btree_node *
btree_delete_node(btree *tree, btree_node **node, void *key) {
   int cmp = tree->keyComparator((*node)->key, key);
   if (cmp == 0) {
      btree_node *delNode = *node;
      if (delNode->rnode != NULL && delNode->lnode != NULL) {
         tree->nodeCount-=2;
         *node = delNode->rnode;
         btree_insert_node(tree, *node, delNode->lnode);
      } else {
         tree->nodeCount--;
         *node = delNode->rnode != NULL 
            ? delNode->rnode 
            : (delNode->lnode != NULL ? delNode->lnode : NULL);
      }

      return delNode;
   } else if (cmp < 0) {
      return btree_delete_node(tree, &((*node)->rnode), key);
   } else {
      return btree_delete_node(tree, &((*node)->lnode), key);
   }
}

btree_node *
btree_delete(btree *tree, void *key) {
    
   if (tree->root == NULL) {
      return NULL;
   }
    
   return btree_delete_node(tree, &tree->root, key);
}

btree_node *
btree_find_node(btree *tree, btree_node *node, void *key) {
   int cmp;
    
   if (node == NULL) {
      return NULL;
   }
    
   cmp = tree->keyComparator(node->key, key);
    
   if (cmp < 0) {
      return btree_find_node(tree, node->rnode, key);
   } else if( cmp > 0 ) {
      return btree_find_node(tree, node->lnode, key);
   } else {
      return node;
   }
}

btree_node *
btree_find(btree *tree, void *key) {
   return btree_find_node(tree, tree->root, key);
}

void 
btree_inorderTraverse_nodeWithReturn(btree_node *node, 
      void(*nodeHandler)(btree_node *node, void *retVal), void *retVal) {
   if (node == NULL) {
      return;
   }
    
   btree_inorderTraverse_nodeWithReturn(node->lnode, nodeHandler, retVal);
   (*nodeHandler)(node, retVal);
   btree_inorderTraverse_nodeWithReturn(node->rnode, nodeHandler, retVal);
}

void 
btree_inorderTraverseWithReturn(btree *tree, 
      void(*nodeHandler)(btree_node *node, void *retVal), void *retVal) {
   btree_inorderTraverse_nodeWithReturn(tree->root, nodeHandler, retVal);
}

void 
btree_inorderTraverse_node(btree_node *node, 
      void(*nodeHandler)(btree_node *node)) {
   if (node == NULL) {
      return;
   }
    
   btree_inorderTraverse_node(node->lnode, nodeHandler);
   (*nodeHandler)(node);
   btree_inorderTraverse_node(node->rnode, nodeHandler);
}

void 
btree_inorderTraverse(btree *tree, void(*nodeHandler)(btree_node *node)) {
   btree_inorderTraverse_node(tree->root, nodeHandler);
}

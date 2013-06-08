/*
 *  orderedlist.c
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

#include <CoreServices/CoreServices.h>
#include "orderedlist.h"

ol_node * 
ol_create_node() {
    ol_node *node = (ol_node*)calloc(1, sizeof(ol_node));
    return node;
}

void 
ol_destroy_node(ol_node *node) {
    free(node);
}

orderedlist *
ol_create(int(*keyComparator)(void *key1, void *key2)) {
    orderedlist *list = (orderedlist*)calloc(1, sizeof(orderedlist));
    list->keyComparator = keyComparator;
    return list;
}

void 
ol_insert_node(orderedlist *list, ol_node *currentNode, ol_node *valueNode) {
    if (currentNode->next != NULL) {
        if (list->keyComparator(currentNode->next->key, valueNode->key) < 0) {
            ol_insert_node(list, currentNode->next, valueNode);
        } else {
            valueNode->next = currentNode->next;
            currentNode->next = valueNode;
        }
    } else {
        currentNode->next = valueNode;
    }
}

void 
ol_insert(orderedlist *list, void *key, void *value) {
    ol_node *node = ol_create_node();
    node->key = key;
    node->value = value;
    
    if (list->head == NULL) {
        list->head = node;
    } else {
        if (list->keyComparator(node->key, list->head->key) <= 0) {
            node->next = list->head;
            list->head = node;
        } else {
            ol_insert_node(list, list->head, node);
        }
    }
    
    list->nodeCount++;
}

void 
ol_delete_node(orderedlist *list, ol_node *node, void *key) {
    if (node->next == NULL) {
        return;
    } else if (list->keyComparator(key, node->next->key) == 0) {
        ol_node *tmpNode = node->next;
        node->next = node->next->next;
        ol_destroy_node(tmpNode);
    }
}

void 
ol_delete(orderedlist *list, void *key) {
   if (list->head == NULL) {
      return;
   } else if (list->keyComparator(key, list->head->key) == 0) {
        ol_node *node = list->head;
        list->head = list->head->next;
        ol_destroy_node(node);
    } else if (list->keyComparator(key, list->head->key) < 0) {
        return;
    } else {
        ol_delete_node(list, list->head, key);
    }
}

ol_node* 
ol_find_node(orderedlist *list, ol_node *node, void *key) {
   int cmp;
    
   if (node == NULL) {
      return NULL;
   }
    
   cmp = list->keyComparator(node->key, key);
    
   if (cmp < 0) {
      return ol_find_node(list, node->next, key);
   } else if( cmp == 0 ) {
      return node;
   } else {
      return NULL;
   }
}

ol_node *
ol_find(orderedlist *list, void *key) {
   return ol_find_node(list, list->head, key);
}

void 
ol_traverse_node(ol_node *node, void(*nodeHandler)(ol_node *node)) {
    if (node == NULL) {
        return;
    }
    
    (*nodeHandler)(node);
    ol_traverse_node(node->next, nodeHandler);
}

void 
ol_traverse(orderedlist *list, void(*nodeHandler)(ol_node *node)) {
    ol_traverse_node(list->head, nodeHandler);
}

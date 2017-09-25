#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cs3723p1.h"


void mmInit(StorageManager *pMgr){
    pMgr->pFreeHead = (FreeNode*) pMgr->pBeginStorage;

    memset(pMgr->pFreeHead, '\0',pMgr->iHeapSize);
    pMgr->pFreeHead->cGC = 'F';
    pMgr->pFreeHead->shNodeSize = pMgr->iHeapSize;
    pMgr->pFreeHead->pFreeNext = NULL;

}

void *mmAllocate(StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[], MMResult *pmmResult){
    //points to top of freenode list
    FreeNode *tempHead = pMgr->pFreeHead;
    InUseNode *newNode = (InUseNode*)tempHead;                 //point newNode to the top of freenode pointer we are pointing at currently


    short wantSize = shDataSize + NODE_OVERHEAD_SZ;
    short minNodeSize = pMgr->iMinimumNodeSize;

    if(tempHead == NULL){
        pmmResult->rc = RC_NOT_AVAIL;
        strcpy(pmmResult->szErrorMessage, "There are no FreeNode's available\n");
    }

    //if there is only one node or head node is large enuff
    else if(tempHead->shNodeSize > wantSize){

        //get diff from sizewanted and see if larger than minNodeSize
        int diff = tempHead->shNodeSize - wantSize;

        //if diff is larger than minNodeSize then carve out the freenode and assign freenode head to it
        //and InUseNode to new carved out space
        addNewNodeAndOrFreeNode(pMgr, tempHead, newNode, wantSize, diff, shDataSize, shNodeType,  sbData);
    }
    //if head node size is less then traverse the linked list of freenodes and
    //find one that fits
    else if(tempHead->shNodeSize < wantSize){
        //check if node is NULL
        while(tempHead != NULL){
            //point to next node cuz last one didnt make the cut
            tempHead = tempHead->pFreeNext;
            //if the newly pointed at node is not null then check its size
            if(tempHead != NULL){
                if(tempHead->shNodeSize > wantSize){
                    //get diff from sizewanted and see if larger than minNodeSize
                    int diff = tempHead->shNodeSize - wantSize;

                    //if the leftover is larger enuff for a free node then create
                    //a new freenode and InUseNode
                    addNewNodeAndOrFreeNode(pMgr, tempHead, newNode, wantSize, diff, shDataSize, shNodeType,  sbData);
                }
            }
            //if the next node is null then we return the bad news via the pointer provided
            if (tempHead == NULL) {
                pmmResult->rc = RC_NOT_AVAIL;
                strcpy(pmmResult->szErrorMessage, "FreeNode of specified size does not exist\n");
            }
        }
    }
    else if (tempHead == NULL) {
        pmmResult->rc = RC_NOT_AVAIL;
        strcpy(pmmResult->szErrorMessage, "FreeNode of specified size does not exist\n");
    }
}

void addNewNodeAndOrFreeNode(StorageManager *pMgr, FreeNode *tempHead, InUseNode *newNode,
                             short wantSize, short newFreeNodeSize, short shDataSize, short shNodeType,
                             char sbData[]){

    if(newFreeNodeSize >= pMgr->iMinimumNodeSize){
        void *endtemp = (void*) ((char*)tempHead + wantSize); //point tempHead to the top of leftover of freenode
        void *endHeap = (void*)pMgr->pEndStorage;
        if(endtemp > endHeap){
            pMgr->pFreeHead = NULL;
        }
        else{
            FreeNode *temp = (FreeNode*)((char*)tempHead + wantSize);
            if(temp->pFreeNext != NULL){
                temp->pFreeNext = pMgr->pFreeHead->pFreeNext; //point new freenode to the next freenode in the linklist of freenodes
            }
            else{
                temp->pFreeNext = NULL;
            }
            //setup metadata for new freenode
            temp->shNodeSize = newFreeNodeSize;
            temp->cGC = 'F';
            pMgr->pFreeHead = temp;                     //point StorageManager object freenode head to the new freenode
        }
    }
    else if(tempHead->pFreeNext != NULL){
        pMgr->pFreeHead = tempHead->pFreeNext;
    }
    //new free node size we have is too small and the next node is null; basically we at end of heap
    else{
        newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize + newFreeNodeSize;
        newNode->shNodeType = shNodeType;
        newNode->cGC = 'U';
        memcpy(newNode->sbData, sbData, shDataSize);
        return;
    }

    //setup metadata for new node
    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
    newNode->shNodeType = shNodeType;
    newNode->cGC = 'U';
    memcpy(newNode->sbData, sbData, shDataSize);
}


void mmMark(StorageManager *pMgr, MMResult *pmmResult){
    char *pCh;
    short shTempSize;
    InUseNode *pAlloc;
    FreeNode *pFree;

    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += shTempSize)
    {
        pAlloc = (InUseNode *)pCh;
        shTempSize = pAlloc->shNodeSize;

        // Change the output based on the cGC type
        switch (pAlloc->cGC){
        case 'F':
        case 'U':
            pAlloc->cGC = 'C';
            printf("think it worked!!!!");
            break;
        }
    }

}

void mmFollow(StorageManager *pMgr, void *pUserData, MMResult *pmmResult){
    InUseNode *pData = (InUseNode*)((char*)pUserData - NODE_OVERHEAD_SZ);
    void **ppNode;
    if(pData == NULL || pData->cGC ==  'U'){
        return;
    }

    if(pData->cGC =='C') {
        pData->cGC = 'U';

        if(pData->shNodeType == 0){
            for(int i = 0; i < 10; i++){
                if(strcmp(pMgr->metaAttrM[i].szAttrName, "pNextCust") == 0){
                    MetaAttr *pAttr = &(pMgr->metaAttrM[i]);
                    ppNode = (void**)&(pData->sbData[pAttr->shOffset]);
                    if(*ppNode == NULL){
                        return;
                    }
                    i = 10;
                }
            }
        }
        else if(pData->shNodeType == 1){
            for(int i = 0; i < 10; i++){
                if(strcmp(pMgr->metaAttrM[i].szAttrName, "pNextItem") == 0){
                    MetaAttr *pAttr = &(pMgr->metaAttrM[i]);
                    ppNode = (void**)&(pData->sbData[pAttr->shOffset]);
                    if(*ppNode == NULL){
                        return;
                    }
                    i = 10;
                }
            }
        }
        mmFollow(pMgr, *ppNode, pmmResult);
    }
}

void mmCollect(StorageManager *pMgr, MMResult *pmmResult){
    char *pCh, *pCh2;
    short shTempSize;
    InUseNode *pAlloc, *pAlloc2;
    FreeNode *pFree;
    FreeNode *pFreeNxt;

    //travers the heap nodes and look for C's
    //if there are adjacent C nodes then combine. this node must point to last node if there is a last node that was free.
    //when the end of the heap is reached the last node is made the head of the free node list
    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += shTempSize){
        pAlloc = (InUseNode *)pCh;
        shTempSize = pAlloc->shNodeSize;
        pCh2 = pCh + shTempSize;
        pAlloc2 = (InUseNode *)pCh2;

        //if adjacent node is C then combine into one node
        if(pAlloc2->cGC == 'C')
        {

        }
        //else if next node isnt C then turn this node into a freenode.

        else{
            switch (pAlloc->cGC) {
            case 'C':
                pFree = (FreeNode*)&pAlloc;
                if(pMgr->pBeginStorage == (char*)pFree){
                    pMgr->pFreeHead = pFree;
                }
                pFree->cGC = 'F';
                pFree->pFreeNext = NULL;
                pFree = pFree->pFreeNext;
                break;
            default:
                break;
            }

        }
    }

}

void mmAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, MMResult *pmmResult){
    //I have to search for attrName in pMgr meta-list.
    //if the attrName doesnt exist then say so in pmmResult
    //next check that the meta data for the attrName is a pointer.
    //the attrName is in a meta list metaAttrM the details are in initMetadata()
    //if the attrName is not a pointer then say so
    //else cheange the pointer to point to the next user data or NULL.
    //detail sheet has the code to make fromUserData point to toUserData
    void **ppNode;
    InUseNode *pUserNodeFrom;
    for(int i = 0; i < 10; i++){
        if(strcmp(pMgr->metaAttrM[i].szAttrName, szAttrName) == 0){
            MetaAttr *pAttr = &(pMgr->metaAttrM[i]);
            if(pAttr->cDataType == 'P'){
                pUserNodeFrom = (InUseNode*)((char*)pUserDataFrom - NODE_OVERHEAD_SZ);
                ppNode = (void**)&(pUserNodeFrom->sbData[pAttr->shOffset]);
                *ppNode = pUserDataTo;
                pmmResult->rc = 0;
                i = 10;
                printf("herer I match\n");
            }
            else{
                pmmResult->rc = RC_ASSOC_ATTR_NOT_PTR;
                char errorMessage[]= "Atttribute not a pointer.";
                memcpy(pmmResult->szErrorMessage, errorMessage, sizeof(errorMessage));
            }
        }
        else{
            pmmResult->rc = RC_ASSOC_ATTR_NOT_FOUND;
            char errorMessage[]= "Atttribute not a found.";
            memcpy(pmmResult->szErrorMessage, errorMessage, sizeof(errorMessage));
        }
    }
}

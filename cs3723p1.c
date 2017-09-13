#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cs3723p1.h"


void mmInit(StorageManager *pMgr){
    pMgr->pFreeHead = (FreeNode*) pMgr->pBeginStorage;

    char *temp = pMgr->pBeginStorage;
    char *tempEnd =  pMgr->pEndStorage;
    for(; *temp <= *tempEnd; *temp++){
        *temp = 0;
    }
    pMgr->pFreeHead->cGC = 'F';
    pMgr->pFreeHead->shNodeSize = pMgr->iHeapSize;

}

void *mmAllocate(StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[], MMResult *pmmResult){
    //points to top of freenode list
    FreeNode *tempHead = pMgr->pFreeHead;

    //InUseNode pointer if we find a freenode of size we want
    InUseNode *newNode;

    int wantSize = shDataSize + NODE_OVERHEAD_SZ;
    int minNodeSize = pMgr->iMinimumNodeSize;

    //if there is only one node or head node is large enuff
    if(tempHead->shNodeSize > wantSize){

        //get diff from sizewanted and see if larger than minNodeSize
        int diff = tempHead->shNodeSize - wantSize;

        //if diff is larger than minNodeSize then carve out the freenode and assign freenode head to it
        //and InUseNode to new carved out space
        if(diff >= minNodeSize){
            addNewNodeAndOrFreeNode(pMgr, tempHead, newNode, wantSize, diff, shDataSize, shNodeType,  sbData);
        }
        else{
            addNewNodeAndOrFreeNode(pMgr, tempHead, newNode, wantSize, diff, shDataSize, shNodeType,  sbData);
        }
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
                    //this'll be the size of the newly created freenode if the left
                    //over is large enuff for a free node
                    int newFreeNodeSize = diff;

                    //if the leftover is larger enuff for a free node then create
                    //a new freenode and InUseNode
                    if(diff >= minNodeSize){
                        addNewNodeAndOrFreeNode(pMgr, tempHead, newNode, wantSize, diff, shDataSize, shNodeType,  sbData);
                    }
                    else{
                        addNewNodeAndOrFreeNode(pMgr, tempHead, newNode, wantSize, diff, shDataSize, shNodeType,  sbData);
                    }
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


void *addNewNodeAndOrFreeNode(StorageManager *pMgr, FreeNode *tempHead, InUseNode *newNode,
        int wantSize, int newFreeNodeSize, short shDataSize, short shNodeType,
        char sbData[]){
    newNode = (InUseNode*)tempHead;                 //point newNode to the top of freenode pointer we are pointing at currently

    if(newFreeNodeSize >= pMgr->iMinimumNodeSize){
        tempHead = (FreeNode*) tempHead + wantSize + 1; //point tempHead to the top of leftover of freenode
        tempHead->pFreeNext = pMgr->pFreeHead->pFreeNext; //point new freenode to the next freenode in the linklist of freenodes

        //setup metadata for new freenode
        tempHead->shNodeSize = newFreeNodeSize;
        tempHead->cGC = 'F';
        pMgr->pFreeHead = tempHead;                     //point StorageManager object freenode head to the new freenode
    }
    else{
        pMgr->pFreeHead = tempHead->pFreeNext;
    }

    //setup metadata for new node
    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
    newNode->shNodeType = shNodeType;
    newNode->cGC = 'U';
    memcpy(newNode->sbData, sbData, sizeof(sbData));//check syntax on this
}


void mmMark(StorageManager *pMgr, MMResult *pmmResult){

}

void mmFollow(StorageManager *pMgr, void *pUserData, MMResult *pmmResult){

}

void mmCollect(StorageManager *pMgr, MMResult *pmmResult){

}

void mmAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, MMResult *pmmResult){

}

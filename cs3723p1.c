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
            int newFreeNodeSize = diff;
            newNode = (InUseNode*)tempHead;
            tempHead = (FreeNode*) tempHead + wantSize + 1;
            tempHead->pFreeNext = pMgr->pFreeHead->pFreeNext;
            tempHead->shNodeSize = newFreeNodeSize;
            tempHead->cGC = 'F';
            pMgr->pFreeHead = tempHead;

            newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
            newNode->shNodeType = shNodeType;
            newNode->cGC = 'U';
            memcpy(newNode->sbData, sbData, sizeof(sbData));//check syntax on this
        }
    }
    else if(tempHead->shNodeSize < wantSize){
	while(tempHead != NULL){
	    tempHead = tempHead->pFreeNext;
	    if(tempHead != NULL){
		    if(tempHead->shNodeSize > wantSize){
                //get diff from sizewanted and see if larger than minNodeSize
                int diff = tempHead->shNodeSize - wantSize;
                int newFreeNodeSize = diff;
                if(diff >= minNodeSize){
                    int newFreeNodeSize = diff;
                    newNode = (InUseNode*)tempHead;
                    tempHead = (FreeNode*) tempHead + wantSize + 1;
                    tempHead->pFreeNext = pMgr->pFreeHead->pFreeNext;
                    tempHead->shNodeSize = newFreeNodeSize;
                    tempHead->cGC = 'F';
                    pMgr->pFreeHead = tempHead;

                    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
                    newNode->shNodeType = shNodeType;
                    newNode->cGC = 'U';
                    memcpy(newNode->sbData, sbData, sizeof(sbData));//check syntax on this
                    //addNewNodeAndFreeNode(pMgr, tempHead, newNode, wantSize, newFreeNodeSize, shDataSize, shNodeType,  sbData);
                }
		    }
	    }
	}
    }
    else if (tempHead == NULL) {
        pmmResult->rc = RC_NOT_AVAIL;
        strcpy(pmmResult->szErrorMessage, "FreeNode of specified size does not exist\n");
    }
}


void *addNewNodeAndFreeNode(StorageManager *pMgr, FreeNode *tempHead, InUseNode *newNode,
        int wantSize, int newFreeNodeSize, short shDataSize, short shNodeType,
        char sbData[]){
    newNode = (InUseNode*)tempHead;
    tempHead = (FreeNode*) tempHead + wantSize + 1;
    tempHead->pFreeNext = pMgr->pFreeHead->pFreeNext;
    tempHead->shNodeSize = newFreeNodeSize;
    tempHead->cGC = 'F';
    pMgr->pFreeHead = tempHead;

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

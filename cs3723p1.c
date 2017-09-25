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
    FreeNode *temp1 = pMgr->pFreeHead;
    FreeNode *temp2 = pMgr->pFreeHead->pFreeNext;
    InUseNode *newNode = (InUseNode*)temp1;                 //point newNode to the top of freenode pointer we are pointing at currently
    FreeNode *temp;

    short wantSize = shDataSize + NODE_OVERHEAD_SZ;
    short minNodeSize = pMgr->iMinimumNodeSize;

    if(temp1->shNodeSize >= wantSize){
        int diff = temp1->shNodeSize - wantSize;

        if(diff >= minNodeSize){
            temp = (FreeNode*)((char*)temp1 + wantSize);
            if(temp2 != NULL){
                temp->pFreeNext = temp2;
            }
            temp->cGC = 'F';
            temp->shNodeSize = diff;
            pMgr->pFreeHead = temp;

            newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
        }
        else{
            pMgr->pFreeHead = pMgr->pFreeHead->pFreeNext;
            newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize + diff;
        }

        newNode->shNodeType = shNodeType;
        newNode->cGC = 'U';
        memcpy(newNode->sbData, sbData, shDataSize);

        return (void*)newNode->sbData;
    }

    while(temp2 != NULL){
        newNode = (InUseNode*)temp2;                 //point newNode to the top of freenode pointer we are pointing at currently
        if( (char*)temp2 < pMgr->pEndStorage){
            if(temp2->shNodeSize >= wantSize){
                int diff = temp2->shNodeSize - wantSize;

                if(diff >= minNodeSize){
                    temp = (FreeNode*)((char*)temp2 + wantSize);
                    temp->pFreeNext = pMgr->pFreeHead;
                    temp1->pFreeNext = temp2->pFreeNext;
                    temp->cGC = 'F';
                    pMgr->pFreeHead = temp;

                    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
                }
                else{
                    temp1->pFreeNext = temp2->pFreeNext;
                    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize + diff;
                }

                newNode->shNodeType = shNodeType;
                newNode->cGC = 'U';
                memcpy(newNode->sbData, sbData, shDataSize);
                return (void*)newNode->sbData;
            }
            else{
                temp1 = temp2;
                temp2 = temp2->pFreeNext;
            }
        }
        else{
            break;
        }
    }
    if ((char*)temp2 > pMgr->pEndStorage){
        pmmResult->rc = RC_NOT_AVAIL;
        memcpy(pmmResult->szErrorMessage,"Specified object size not available in Free Memory list\n", sizeof(pmmResult->szErrorMessage));
    }
}



void mmMark(StorageManager *pMgr, MMResult *pmmResult){
    char *pCh;
    short shTempSize;
    InUseNode *pAlloc;

    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += shTempSize)
    {
        pAlloc = (InUseNode *)pCh;
        shTempSize = pAlloc->shNodeSize;

        // Change the output based on the cGC type
        switch (pAlloc->cGC){
        case 'F':
        case 'U':
        case 'C':
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
    char *pCh, *p;
    short size = 0;
    InUseNode *pAlloc;
    FreeNode *pFree;
    FreeNode *pHead = NULL;

    //travers the heap nodes and look for C's
    //if there are adjacent C nodes then combine. this node must point to last node if there is a last node that was free.
    //when the end of the heap is reached the last node is made the head of the free node list
    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += size){
        pAlloc = (InUseNode *)pCh;
        p = pCh;
        size = pAlloc->shNodeSize;

        if(pAlloc->cGC == 'C'){
            pFree = (FreeNode*)pCh;
            pFree->cGC = 'F';
            pFree->shNodeSize = combine(p, pAlloc, size);
            pFree->pFreeNext = pHead;
            pHead = pFree;
            size = pHead->shNodeSize;
        }

    }
    pMgr->pFreeHead = pHead;
}

short combine(char *p, InUseNode *pAlloc, short size){
    char *nextNode = p + pAlloc->shNodeSize;
    InUseNode *nextInUseNode = (InUseNode*)nextNode;
    if(nextInUseNode->cGC != 'C'){
        return size;
    }
    else if(nextInUseNode->cGC == 'C'){
        size += combine(nextNode, nextInUseNode, nextInUseNode->shNodeSize);
        return size;
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

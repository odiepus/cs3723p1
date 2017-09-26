/**********************************************************************
  cs3723p1.c  by Hector Herrera
Purpose:
    Simulate garbage collection of nodes in a heap
Input:
    Commands used to created, associate and collect InUseNodes and FreeNode's
    ALLOC, ASSOC, GCOLL
Results:
    A heap stucture that has every byte accounted for either as an InUseNode or
    a FreeNode.
**********************************************************************/



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cs3723p1.h"

/***************************mmInit****************************************
void mmInit(StorageManager *pMgr);
Purpose:
    Initializes the first free node. The StorageManager head free node
    pointer points to it. The meta data for the free node is initialized
    well.
Parameters:
I StorageManager *pMgr      Container with meta info used to access heap
Notes:
**************************************************************************/

void mmInit(StorageManager *pMgr){
    pMgr->pFreeHead = (FreeNode*) pMgr->pBeginStorage;

    memset(pMgr->pFreeHead, '\0',pMgr->iHeapSize);
    pMgr->pFreeHead->cGC = 'F';
    pMgr->pFreeHead->shNodeSize = pMgr->iHeapSize;
    pMgr->pFreeHead->pFreeNext = NULL;

}

/***************************mmAllocate****************************************
void *mmAllocate(StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[], MMResult *pmmResult);
Purpose:
    Allocate node from free space list. Search for a node that fits. If a free node
    has excess space and space is larger than minimum node size then a new free node
    is created and added to top of free node list
Parameters:
    I StorageManager *pMgr      Container with meta info used to access heap
    I short shDataSize          Size of data pass in
    I short shNodeType          identifies node type
    I char sbData[]             array containing data for the node
    I MMResult *pmmResult       container to pass cause of errors
Notes:
    2 pointers are used to traverse the linked list and find a node
    of suitable size. if node has enough leftover then a free node
    is created and placed at the top of the free node list

Return value:
    pointer to sbData within the node
**************************************************************************/

void *mmAllocate(StorageManager *pMgr, short shDataSize, short shNodeType, char sbData[], MMResult *pmmResult){
    FreeNode *temp1 = pMgr->pFreeHead;                  //points to top of freenode list
    FreeNode *temp2 = pMgr->pFreeHead->pFreeNext;
    InUseNode *newNode = (InUseNode*)temp1;             //point newNode to the top of freenode pointer we are pointing at currently
    FreeNode *temp;

    short wantSize = shDataSize + NODE_OVERHEAD_SZ;
    short minNodeSize = pMgr->iMinimumNodeSize;

    if(temp1->shNodeSize >= wantSize){                  //In this if statement we create freenode and a allocate Inuse node
        int iDiff = temp1->shNodeSize - wantSize;

        if(iDiff >= minNodeSize){                        //if there is enough leftover for a free node then we create one
            temp = (FreeNode*)((char*)temp1 + wantSize);
            if(temp2 != NULL){
                temp->pFreeNext = temp2;
            }
            temp->cGC = 'F';
            temp->shNodeSize = iDiff;
            pMgr->pFreeHead = temp;

            newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
        }
        else{                                           //if there isnt enought leftover for a free node then Inuse node uses the whole thing
            pMgr->pFreeHead = pMgr->pFreeHead->pFreeNext;
            newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize + iDiff;
        }

        newNode->shNodeType = shNodeType;               //setup metadata for new node
        newNode->cGC = 'U';
        memcpy(newNode->sbData, sbData, shDataSize);

        return (void*)newNode->sbData;                  //return pointer to the shData within the container
    }

    while(temp2 != NULL){
        newNode = (InUseNode*)temp2;                    //point newNode to the top of freenode pointer we are pointing at currently
        if( (char*)temp2 < pMgr->pEndStorage){          //make sure the temp2 node is within heap
            if(temp2->shNodeSize >= wantSize){          //if the freenode is large enough for our new node then we use it if leftover is large enough then create new free node
                int iDiff = temp2->shNodeSize - wantSize;

                if(iDiff >= minNodeSize){                //if leftover is large enough then create new free node
                    temp = (FreeNode*)((char*)temp2 + wantSize);
                    temp->pFreeNext = pMgr->pFreeHead;
                    temp1->pFreeNext = temp2->pFreeNext;
                    temp->cGC = 'F';
                    pMgr->pFreeHead = temp;

                    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize;
                }
                else{
                    temp1->pFreeNext = temp2->pFreeNext;
                    newNode->shNodeSize = NODE_OVERHEAD_SZ + shDataSize + iDiff;
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
    if ((char*)temp2 > pMgr->pEndStorage){              //if no suitable node is found then notify the user
        pmmResult->rc = RC_NOT_AVAIL;
        memcpy(pmmResult->szErrorMessage,"Specified object size not available in Free Memory list\n", sizeof(pmmResult->szErrorMessage));
    }
}



/***************************mmMark****************************************
void mmMark(StorageManager *pMgr, MMResult *pmmResult);
Purpose:
    function marks every node in the heap with a 'C'
Parameters:
    I StorageManager *pMgr      Container with meta info used to access heap
    I MMResult *pmmResult       Container used to store error messages
Notes:
    iterate through the heap using each node size as the size to iterate to
**************************************************************************/

void mmMark(StorageManager *pMgr, MMResult *pmmResult){
    char *pCh;
    short shTempSize;
    InUseNode *pAlloc;

    //following logic is attributed to Professor Clark UTSA. Inspiration taken from cs3723p1Driver.c
    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += shTempSize)         //iterate node by node through the heap and mark each with 'C'
    {
        pAlloc = (InUseNode *)pCh;
        shTempSize = pAlloc->shNodeSize;

        // Change the output based on the cGC type
        switch (pAlloc->cGC){
        case 'F':
        case 'U':
        case 'C':
            pAlloc->cGC = 'C';
            break;
        }
    }

}

/***************************mmFollow****************************************
void mmFollow(StorageManager *pMgr, void *pUserData, MMResult *pmmResult);
Purpose: traverse list and mark nodes still in use with 'U'
Parameters:
    I StorageManager *pMgr      Container with meta info used to access heap
    I void *pUserData           Pointer to User node cGC
    I MMResult *pmmResult       Container used to store error messages
Notes:
    based on passed in user data nodes that are traversed are marked as used.
    based on the node type different attribute names are used to search
    for the offset
**************************************************************************/

void mmFollow(StorageManager *pMgr, void *pUserData, MMResult *pmmResult){
    InUseNode *pData = (InUseNode*)((char*)pUserData - NODE_OVERHEAD_SZ);
    void **ppNode;
    if(pData == NULL || pData->cGC ==  'U'){
        return;
    }


    //following logic is attributed to Professor Clark UTSA. Inspiration taken from cs3723p1Driver.c
    if(pData->cGC =='C') {                                              //given data from a pointer check if 'C' and change to 'U'
        pData->cGC = 'U';

        if(pData->shNodeType == 0){
            for(int i = 0; i < 10; i++){                                //then check if current node is linked to another node
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

/***************************mmCollect****************************************
void mmCollect(StorageManager *pMgr, MMResult *pmmResult);
Purpose:
    Function collects all nodes that still have the cGC of 'C' and sets
    them up as free nodes. Creates a linked list out of the free nodes
Parameters:
    I StorageManager *pMgr      Container used to store meta data for use in pointer arithemitic
    I MMResult *pmmResult       Container used to store error messages
Notes:
    single pointer is used to look at each node and a helper function is
    used to search the next sequence of nodes to look for more 'C' nodes to
    add to the currently found 'C' node
**************************************************************************/

void mmCollect(StorageManager *pMgr, MMResult *pmmResult){
    char *pCh, *p;
    short shSize = 0;
    InUseNode *pAlloc;
    FreeNode *pFree;
    FreeNode *pHead = NULL;

    //following logic is attributed to Professor Clark UTSA. Inspiration taken from cs3723p1Driver.c
    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += shSize){      //iterate through each node in heap and collect nodes with 'C'
        pAlloc = (InUseNode *)pCh;
        p = pCh;
        shSize = pAlloc->shNodeSize;

        if(pAlloc->cGC == 'C'){
            pFree = (FreeNode*)pCh;
            pFree->cGC = 'F';
            pFree->shNodeSize = combine(p, pAlloc, shSize);
            pFree->pFreeNext = pHead;
            pHead = pFree;
            shSize = pHead->shNodeSize;
        }

    }
    pMgr->pFreeHead = pHead;                                                    //point StorageManager container FreeNode pointer to top of newly create linked list
}

/***************************combine****************************************
short combine(char *p, InUseNode *pAlloc, short shSize);
Purpose:
    Helper function used to search adjacent nodes for 'C' nodes
    that can be combined to the initial 'C' node found
    The sequence is searched recursively until no more 'C' nodes are
    found.
Parameters:
    I   char *p           passed pointer of last found 'C' node
    I   InUseNode *pAlloc passed in node of last found 'C' node
    I/O short shSize        passed in size of last found 'C' node
Return value:
    return the combined size of all 'C' nodes
**************************************************************************/

short combine(char *p, InUseNode *pAlloc, short shSize){
    char *nextNode = p + pAlloc->shNodeSize;
    InUseNode *nextInUseNode = (InUseNode*)nextNode;
    if(nextInUseNode->cGC != 'C'){
        return shSize;
    }
    else if(nextInUseNode->cGC == 'C'){
        shSize += combine(nextNode, nextInUseNode, nextInUseNode->shNodeSize);
        return shSize;
    }
}

/***************************mmAssoc****************************************
void mmAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, MMResult *pmmResult){
Purpose:
    creates a linked list between specified nodes
Parameters:
    I StorageManager *pMgr
    I void *pUserDataFrom
    I char szAttrName[]
    I void *pUserDataTo
    I MMResult *pmmResult
Notes:
    using the meta data list in the StorageManager container the attribute is searched for
    and used to obtain offset for the pointers that are used to point to other nodes
**************************************************************************/

void mmAssoc(StorageManager *pMgr, void *pUserDataFrom, char szAttrName[], void *pUserDataTo, MMResult *pmmResult){
    void **ppNode;
    InUseNode *pUserNodeFrom;

    //following logic is attributed to Professor Clark UTSA. Inspiration taken from cs3723p1Driver.c
    for(int i = 0; i < 10; i++){
        if(strcmp(pMgr->metaAttrM[i].szAttrName, szAttrName) == 0){
            MetaAttr *pAttr = &(pMgr->metaAttrM[i]);
            if(pAttr->cDataType == 'P'){
                pUserNodeFrom = (InUseNode*)((char*)pUserDataFrom - NODE_OVERHEAD_SZ);
                ppNode = (void**)&(pUserNodeFrom->sbData[pAttr->shOffset]);
                *ppNode = pUserDataTo;
                pmmResult->rc = 0;
                i = 10;
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

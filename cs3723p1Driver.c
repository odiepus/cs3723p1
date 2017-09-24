/**********************************************************************
    cs3723p1Driver.c by Larry Clark
Purpose:
    This program is demonstrates Storage Management software responsible
    for garbage collection.  It places references to data in a hash
    table.  The data is in a heap managed by a student's mmXX... functions.
Command Parameters:
    p1 < infile > outfile
    The stream input file contains commands.  There are commands
    which specify comments, allocating memory, dereferencing memory,
    associating/disaccociating nodes to other nodes, and printing
    nodes.

    Commands in the input file:

    * comment
        This is just a comment which helps explain what is being tested.

    ALLOC key nodeTypeNm val1, val2, ...
        Establish a user data node of the specified type.  The data (val1, val2, ...) will be
        placed in the user data node.  This command invokes mmAllocate which returns a user data
        pointer to allocated memory.  Key is used as an entry in a Hash Table which has a
        corresponding value set to the user data pointer.
    ASSOC keyFrom attrName keyTo
        The user data for the specified keyFrom will be changed to point to the pointer
        referenced by keyTo.  Note that keyTo might be NULL, causing the attribute to
        change to NULL.
    DEREF key
        It sets the value to NULL for the specfied key in the hash table.
    DUMP
        Dumps all the nodes in the memory.
    GCOLL
        Causes the subphases of garbage collection to be invoked.
        Mark Subphase       Walk through all memory from the beginning and mark
                            all nodes to a cGC of 'C' (candidate).  It is easier
                            to combine adjacent free areas if all nodes are
                            marked as 'C'.
        Follow Subphase     From any valid starting point (which we are simulating
                            with a hash table), follow the nodes based on metadata
                            and mark each referenceable node as 'U' (in use).
        Collection Subphase We will build the free list from the entire memory.
                            (We are affectively ignoring the old free linked list.)
                            Walk through all the memory looking for 'C' nodes, combining
                            adjacent ones into a single free area.  Each free area
                            will be placed on the front of the new free list.
    PRTALL
        Prints all nodes in the memory.
    PRTNODE key
        Invokes printNode for the node referenced by the specified key.

Results:
    All output is written to stdout.
    Prints the metadata.
    Prints each of the commands and their command parameters.  Some of the commands
    (PRTNODE, PRTALL, DUMP) also print information.

Returns:
    0       Normal
    99      Processing Error

Notes:
   The Hash Table software uses the C++ unordered map.
**********************************************************************/
// If compiling using visual studio, tell the compiler not to give its warnings
// about the safety of scanf and printf
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "cs3723p1.h"

#define MAX_TOKEN_SIZE 50		// largest token size for tokenizer
#define MAX_BUFFER_SZ 100       // size of input buffer

// prototypes only used by the driver
void processCommands(StorageManager *pMgr, FILE *pfileCommand);
int getSimpleToken(char szInput[], const char szDelims[]
    , int *piBufferPosition, char szToken[]);
void setData(StorageManager *pMgr, short shNodeType, char szInput[], char sbData[]);
void initMetadata(StorageManager *pMgr);
void printMeta(StorageManager *pMgr);

// If on Windows, don't use extern "C" in calling file.
// g++ compiler requires the extern "C".
#if defined(_WIN32) || defined(_WIN64)
extern void *getHash(const char *szKey);
extern void putHash(const char *szKey, void *value);
extern void eraseAll();
extern void getAll(HashMO *pHashMO);
#else
extern "C" void *getHash(const char *szKey);
extern "C" void putHash(const char *szKey, void *value);
extern "C" void eraseAll();
extern "C" void getAll(HashMO *pHashMO);
#endif

int main(int argc, char *argv[])
{
    StorageManager storageManager;

    // Set up the storage manager and our metadata
    smInit(&storageManager);
    initMetadata(&storageManager);

    // Print the metadata
    printMeta(&storageManager);

    // Process commands for manipulating user data nodes
    processCommands(&storageManager, stdin);
    free(storageManager.pBeginStorage);
    printf("\n");
    return 0;
}
/******************** smInit **************************************
    void smInit(StorageManager *pMgr)
Purpose:
    Initializes the heap and corresponding attributes in the
    storage manager.
Parameters:
    O StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Returns:
    n/a
Notes:
    - Uses malloc to actually allocate memory for the heap.
    - Inovkes student's mmInit to initialize the free list and
      its initially huge free node.
**************************************************************************/
#if defined(_WIN32)
#define HEAP_SIZE 810
#else
#define HEAP_SIZE 900
#endif

void smInit(StorageManager *pMgr)
{
    pMgr->pBeginStorage = (char *)malloc(HEAP_SIZE);
    if (pMgr->pBeginStorage == NULL)
        errExit("%s", "malloc failed to allocate heap ");
    pMgr->iHeapSize = HEAP_SIZE;
    pMgr->pEndStorage = pMgr->pBeginStorage + HEAP_SIZE;
    pMgr->pFreeHead = NULL;

    // The minimum node size is the size of a minimum free node.
    // This is 12 bytes on 32-bit or 16 bytes on 64 bit
    // 2 byte - shNodeSize
    // 2 byte - shNodeType
    // 1 byte - cGC
    // 3 slack bytes
    // 4 or 8 byte pointer to the next free node.
    pMgr->iMinimumNodeSize = sizeof(FreeNode);

    // Invoke student's mmInit to initialize the free list and a huge free node
    mmInit(pMgr);
}

/******************** initMetadata **************************************
    void initMetadata(StorageManager *pMgr)
Purpose:
    Initializes the nodeTypeM and metaAttrM arrays with metadata.
Parameters:
    O StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Notes:

**************************************************************************/
void initMetadata(StorageManager *pMgr)
{
#define SHCUST 0   // node type for Customer
#define SHLINE 1   // node type for LineItem
    // The following macro gives values to the size and offset attributes
#define SIZE_OFFSET(ptr,attr) \
    ((short)sizeof(attr)), (short)(((char *)&attr) - ((char *) ptr))

    struct LineItem
    {
        char szProductId[10];
        int  iQtyRequest;
        double dCost;
        struct LineItem *pNextItem;
    } lineItem;
    struct Customer
    {
        char szCustomerId[8];
        char szName[16];
        struct LineItem *pFirstItem;
        struct Customer *pNextCustomer;
        double dBalance;
    } customer;
    char *pCustomer = (char *)&customer;
    char *pLineItem = (char *)&lineItem;
    int iN = 0;
    // nodeTypeM contains metadata about each node type which will be copied to storage manager
                    //this'll be the size of the newly created freenode if the left
                    //over is large enuff for a free node
                    int newFreeNodeSize = diff;
    NodeType nodeTypeM[MAX_NODE_TYPE] =
    {
        { "Customer", 0, sizeof(struct Customer) }
        , { "LineItem", 5, sizeof(struct LineItem) }
        , { "", 0 } // sentinel
    };
    // metaAttrM contains metadata about each user data attribute which will be copied
    // to storage manager.  This is using the excellent initialization capability of C.
    MetaAttr metaAttrM[MAX_NODE_ATTR] =
    {
        { SHCUST, "customerId", 'S', SIZE_OFFSET(pCustomer, customer.szCustomerId) }
        , { SHCUST, "name", 'S', SIZE_OFFSET(pCustomer, customer.szName) }
        , { SHCUST, "pFirstItem", 'P', SIZE_OFFSET(pCustomer, customer.pFirstItem) }
        , { SHCUST, "pNextCust", 'P', SIZE_OFFSET(pCustomer, customer.pNextCustomer) }
        , { SHCUST, "balance", 'D', SIZE_OFFSET(pCustomer, customer.dBalance) }
        , { SHLINE, "productId", 'S', SIZE_OFFSET(pLineItem, lineItem.szProductId) }
        , { SHLINE, "iQtyReq", 'I', SIZE_OFFSET(pLineItem, lineItem.iQtyRequest) }
        , { SHLINE, "dCost", 'D', SIZE_OFFSET(pLineItem, lineItem.dCost) }
        , { SHLINE, "pNextItem", 'P', SIZE_OFFSET(pLineItem, lineItem.pNextItem) }
        , { -1, "END_OF_ATTR" } // sentinel
    };
    memcpy(pMgr->nodeTypeM, nodeTypeM, sizeof(nodeTypeM));
    memcpy(pMgr->metaAttrM, metaAttrM, sizeof(metaAttrM));
}
/******************** printMeta **************************************
    void printMeta(StorageManager *pMgr)
Purpose:
    Prints metadata about each node type.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Notes:

**************************************************************************/
void printMeta(StorageManager *pMgr)
{
    int iN;         // subscript in nodeTypeM array
    int iAt;        // subscript in metaAttrM array
    printf("Metadata\n");
    printf("%-10s %-12s %8s\n", "Node Type", "Beg Attr Sub", "Total Sz");
    // Loop for each node type.  The end is marked by a name which is an empty string.
    for (iN = 0; pMgr->nodeTypeM[iN].szNodeTypeNm[0] != '\0'; iN++)
    {
        printf("%-10s %4d%8s %4d\n"
            , pMgr->nodeTypeM[iN].szNodeTypeNm
            , pMgr->nodeTypeM[iN].shBeginMetaAttr
            , " "
            , pMgr->nodeTypeM[iN].shNodeTotalSize);
        printf("\t\t%-14s %-4s %-6s %-4s\n"
            , "Attribute Name"
            , "Type"
            , "Offset"
            , "Size");
        // The attributes for the node type begin at its shBeginMetaAttr subscript and continue while
        // the shNodeType is this node's node type (which is the node type's subscript in
        // the nodeTypeM array).
        for (iAt = pMgr->nodeTypeM[iN].shBeginMetaAttr; pMgr->metaAttrM[iAt].shNodeType == iN; iAt++)
        {
            printf("\t\t%-14s   %c  %6i %4i\n"
                , pMgr->metaAttrM[iAt].szAttrName
                , pMgr->metaAttrM[iAt].cDataType
                , pMgr->metaAttrM[iAt].shOffset
                , pMgr->metaAttrM[iAt].shSizeBytes);
        }

    }
}
/******************** processCommands **************************************
    void processCommands(StorageManager *pMgr, FILE *pfileCommand)
Purpose:
    Reads the Command file to process commands.  There are several types of
    records (see the program header for more information).
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I FILE *pfileCommand    command stream input
Notes:
    This calls functions inside:
        hashAPi
        hexDump64
        cs3723p1
**************************************************************************/
void processCommands(StorageManager *pMgr, FILE *pfileCommand)
{
    // variables for command processing
    char szInputBuffer[MAX_BUFFER_SZ+1];    // input buffer for a single text line
    char szCommand[MAX_TOKEN_SIZE + 1];     // command
    int bGotToken;                          // TRUE if getSimpleToken got a token
    int iBufferPosition;                    // This is used by getSimpleToken to
                                            // track parsing position within input buffer

    // variables used for the buffer passed to hexdump
    int iBytesPerLine = 40;                 // number of bytes to be displayed per line
                                            // by hexDump
    int iScanfCnt;                          // number of values returned by sscanf
    int iBytesToSend = 0;                   // number of bytes sent to hexDump
    int iLines = 0;                         // number of lines returned from hexDump

    MMResult mmResult = { 0, "" };

    //  get command data until EOF
    while (fgets(szInputBuffer, MAX_BUFFER_SZ, pfileCommand) != NULL)
    {
        // if the line is just a line feed, ignore it
        if (szInputBuffer[0] == '\n')
            continue;

        // get the command
        iBufferPosition = 0;                // reset buffer position
        bGotToken = getSimpleToken(szInputBuffer, " \n\r", &iBufferPosition, szCommand);
        if (! bGotToken)
            errExit("Invalid command: %s", szInputBuffer);

        // see if the command is a comment
        if (szCommand[0]== '*')
        {
            printf("%s", szInputBuffer);
            continue;       // it was just a comment
        }
        memset(&mmResult, '\0', sizeof(mmResult));  // in case the mm functions didn't
        printf(">>> %s", szInputBuffer);

        if (strcmp(szCommand, "ALLOC") == 0)
        {   // ALLOC key nodeTypeNm val1, val2, ...
            char szKey[MAX_KEY_SIZE + 1];
            char szNodeTypeNm[MAX_STRING + 1];
            char szRemainingInput[MAX_DATA_SZ + 1]; // Used for a node's data values
            char sbData[MAX_DATA_SZ];
            void *pUserData = NULL;
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s %10s %100[^\n]"
                , szKey
                , szNodeTypeNm
                , szRemainingInput);
            if (iScanfCnt < 3)
                errExit("Invalid ALLOC argument; only %d valid values\nInput is %s"
                    , iScanfCnt, szInputBuffer);
            // get the node type (i.e., subscript in the nodeTypeM array
            short shNodeType = findNodeType(pMgr, szNodeTypeNm);
            if (shNodeType == NOT_FOUND)
                errExit("ALLOC command has invalid node type = '%s'", szNodeTypeNm);
            short shSize = pMgr->nodeTypeM[shNodeType].shNodeTotalSize;
            // Set up the data attributes in the user data
            setData(pMgr, shNodeType, szRemainingInput, sbData);
            // Invoke the memory manager allocate function
            pUserData = mmAllocate(pMgr, shSize, shNodeType, sbData, &mmResult);

            // If it allocated memory, record the key and pointer in the hash table
            if (pUserData != NULL)
            {   // they gave it memory, confirm that the pointer is a user pointer
                InUseNode *pInUse;
                if ((char *)pUserData < pMgr->pBeginStorage || (char *)pUserData >= pMgr->pEndStorage)
                    errExit("mmAllocate returned a pointer outside of heap range");
                pInUse = (InUseNode *)((char *)pUserData - NODE_OVERHEAD_SZ);
                if (pInUse->cGC != 'U')
                    errExit("mmAllocate returned a pointer which has a cGC not equal to 'U'");
                // Must be ok
                putHash(szKey, pUserData);  // record where it was placed
            }
            else
            {    // Did not allocate memory
                printf("\t!!! Memory not allocated\n");
                printf("\t\tsmAllocate rc=%d, %s\n"
                    , mmResult.rc
                    , mmResult.szErrorMessage);
            }

        }
        else if (strcmp(szCommand, "DEREF") == 0)
        {   // FREE key
            char szKey[MAX_KEY_SIZE + 1];
            char szNULL[MAX_STRING + 1] = "";

            // get the key from the input
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s"
                , szKey);
            // was the key in it?
            if (iScanfCnt <  1)
                errExit("Invalid DEREF argument; only %d valid values\nInput is %s"
                , iScanfCnt, szInputBuffer);

            // Set the reference in the hash table to NULL.
            putHash(szKey, NULL);
        }
        else if (strcmp(szCommand, "ASSOC") == 0)
        {   // ASSOC keyFrom attrName keyTo
            char szKeyFrom[MAX_KEY_SIZE + 1];
            char szKeyTo[MAX_KEY_SIZE + 1];
            char szAttrNm[MAX_STRING + 1];
            void *pUserFromData = NULL;
            void *pUserToData = NULL;
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s %10s %10s"
                , szKeyFrom
                , szAttrNm
                , szKeyTo);
            if (iScanfCnt < 3)
                errExit("Invalid ASSOC argument; only %d valid values\nInput is %s"
                , iScanfCnt, szInputBuffer);
            // get the user address of the from
            pUserFromData = (char *)getHash(szKeyFrom);
            if (pUserFromData == NULL)
            {
                printf("*** getHash for 'from' returned NULL\n");
                continue;
            }
            // get the user address of the to
            pUserToData = (char *)getHash(szKeyTo);
            if (pUserToData == NULL)
            {
                printf("*** getHash for 'to' returned NULL\n");
                continue;
            }
            mmAssoc(pMgr, pUserFromData, szAttrNm, pUserToData, &mmResult);
            if (mmResult.rc != 0)
            {
                printf("\t\tmmAssoc rc=%d, %s\n"
                    , mmResult.rc
                    , mmResult.szErrorMessage);
            }
        }
        else if (strcmp(szCommand, "GCOLL") == 0)
        {  // Garbage Collection Process
            garbageCollection(pMgr, &mmResult);
        }
        else if (strcmp(szCommand, "DUMP") == 0)
            smDump(pMgr);
        else if (strcmp(szCommand, "PRTNODE") == 0)
        {   // PRTNODE key
            char szKey[MAX_KEY_SIZE + 1];
            char *pUserData;

            // get the key from the input
            iScanfCnt = sscanf(&szInputBuffer[iBufferPosition]
                , "%10s"
                , szKey);
            // was the key in it?
            if (iScanfCnt <  1)
                errExit("Invalid PRTNODE argument; only %d valid values\nInput is %s"
                , iScanfCnt, szInputBuffer);

            // get the address to user data
            pUserData = (char *)getHash(szKey);
            if (pUserData == NULL)
            {
                printf("*** getHash returned NULL\n");
                continue;
            }
            else
                printNode(pMgr, pUserData);
        }
        else if (strcmp(szCommand, "PRTALL") == 0)
        { // Print all the nodes having a reference in the hash table
            printAll(pMgr);
            printf("\n");
        }
        else
            errExit("Invalid command: '%s'", szCommand);
    }
    printf("\n");   // good place for a breakpoint
}
/******************** findNodeType **************************************
    short findNodeType(StorageManager *pMgr, char szNodeTypeNm[])
Purpose:
    Searches the storage manager's nodeTypeM array to find the
    specified node type and returns its subscript.  If not found
    it returns NOT_FOUND.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I char szNodeTypeNm[]   Node type to find.
Returns:
    Subscript of the node type in the nodeTypeM array.
    NOT_FOUND (-1) is it isn't found.
**************************************************************************/
short findNodeType(StorageManager *pMgr, char szNodeTypeNm[])
{
    int iN;
    // Loop for each node type.  The end is marked by a name which is an empty string.
    for (iN = 0; pMgr->nodeTypeM[iN].szNodeTypeNm[0] != '\0'; iN++)
    {
        if (strcmp(pMgr->nodeTypeM[iN].szNodeTypeNm, szNodeTypeNm) == 0)
            return iN;
    }
    return NOT_FOUND;
}
/******************** setData **************************************
    void setData(StorageManager *pMgr, short shNodeType
        , char szInput[], char sbData[])
Purpose:
    Uses metadata to set the attributes of user data based on comma-
    separated input text.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    I short shNodeType      Node type of the user data
    I char szInput[]        String containing the comma separated input text
    O char sbData[]         Binary data set by this function
Returns:
    n/a
Notes:
    Assumes that shNodeType is a valid subscript in the nodeTypeM array.
**************************************************************************/
void setData(StorageManager *pMgr, short shNodeType, char szInput[], char sbData[])
{
    int iAt;                        // control variable representing subscript in metaAttrM
    char szToken[MAX_STRING];       // token returned by getSimpleToken
    int iBufferPos = 0;             // current buffer position used by getSimpleToken
    MetaAttr *pAttr;                // slightly simplifies referencing item in metaAttrM
    int iScanfCnt;                  // returned by sscanf
    int iValue;                     // integer value when attribute is an int
    double dValue;                  // double value when attribute is a double
    char *pszMemAtOffset;           // pointer into user data if this attribute is a string
    int *piMemAtOffset;             // pointer into user data if this attribute is an int
    void **ppNode;                  // pointer into user data if this attribute is a pointer
    double *pdMemAtOffset;          // pointer into user data if this attribute is a double
    int iLen;                       // helps with checking too long of a string value
    // zero out the user data
    memset(sbData, '\0', pMgr->nodeTypeM[shNodeType].shNodeTotalSize);
    // Loop through each of the user data's attributes.  The subscripts start with
    // shBeginMetaAttr from nodeTypeM and end when the corresponding metaAttr's node type is
    // different.
    for (iAt = pMgr->nodeTypeM[shNodeType].shBeginMetaAttr; pMgr->metaAttrM[iAt].shNodeType == shNodeType; iAt++)
    {
        pAttr = &(pMgr->metaAttrM[iAt]);   // slightly simplify the reference in the metaAttrM array
        // get the next token from the input
        if (!getSimpleToken(szInput, ",\n\r",&iBufferPos, szToken))
            return;

        // set the data based on the attribute data type
        switch (pAttr->cDataType)
        {
            case 'P':  // pointer
                // The value in the data must state NULL
                if (strcmp(szToken, "NULL")!= 0)
                    errExit("Invalid ALLOC command argument: '%s'", szToken);
                // get the attribute's address based on its offset
                ppNode = (void **) &(sbData[pAttr->shOffset]);
                *ppNode = NULL;                 // assign it NULL
                break;
            case 'S':  // char string
                // check for too long of a value
                iLen = strlen(szToken);
                if (iLen > pAttr->shSizeBytes - 1)
                    errExit("Invalid ALLOC command argument, value too long: '%s'", szToken);
                // get the attribute's address based on its offset
                pszMemAtOffset = (char *) &(sbData[pAttr->shOffset]);
                strcpy(pszMemAtOffset, szToken);
                break;
            case 'I':  // int
                // Convert the token to an int
                iScanfCnt = sscanf(szToken, "%d", &iValue);
                if (iScanfCnt < 1)
                    errExit("Invalid ALLOC command argument, not an int: '%s'", szToken);
                // get the attribute's address based on its offset
                piMemAtOffset = (int *) &(sbData[pAttr->shOffset]);
                *piMemAtOffset = iValue;
                break;
            case 'D':  // double
                // Convert the token to a double
                iScanfCnt = sscanf(szToken, "%lf", &dValue);
                if (iScanfCnt < 1)
                    errExit("Invalid ALLOC command argument, not an double: '%s'", szToken);
                // get the attribute's address based on its offset
                pdMemAtOffset = (double *)&(sbData[pAttr->shOffset]);
                *pdMemAtOffset = dValue;
                break;
            default:
                errExit("Invalid data type '%c' for attribute named '%s'"
                    , pAttr->cDataType
                    , pAttr->szAttrName);
        }
    }
}
/******************** smDump **************************************
    void smDump(StorageManager *pMgr)
Purpose:
    Uses hexDump to dump each node in the heap.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Returns:
    n/a
Notes:
    - The heap begins at pBeginStorage and ends at pEndStorage.
    - Uses hexDump64.c's hexDump function.
**************************************************************************/
void smDump(StorageManager *pMgr)
{
    int iBytesToSend;
    int iBytesPerLine = 20;
    char *pCh;
    short shTempSize;
    InUseNode *pAlloc;
    FreeNode *pFree;
    // Print each item from the beginning of the heap to the end
    printf("\t%-8s %-5s %4s %8s\n", "Address", "Mem", "Size", "NodeType");
    for (pCh = pMgr->pBeginStorage; pCh < pMgr->pEndStorage; pCh += shTempSize)
    {
        pAlloc = (InUseNode *)pCh;
        shTempSize = pAlloc->shNodeSize;

        // Change the output based on the cGC type
        switch (pAlloc->cGC)
        {
        case 'F':
            // It is a free item
            printf("\t%08lX %-5s %4d\n"
                , ULAddr(pAlloc), "Free", shTempSize);
            pFree = (FreeNode *)pAlloc;
            printf("\t\t\tNext:%08lX", ULAddr(pFree->pFreeNext));
            // check for bad pFreeNext pointer
            if ( pFree->pFreeNext != NULL &&
                ((char *)pFree->pFreeNext) < pMgr->pBeginStorage
                || ((char *) pFree->pFreeNext) > pMgr->pEndStorage )
                printf(" *** next pointer is outside of heap\n");
            else
                printf("\n");
            break;
        case 'U':
            // It is in use
            printf("\t%08lX %-5s %4d   %d\n"
                , ULAddr(pAlloc), "InUse", shTempSize, pAlloc->shNodeType);
            iBytesToSend = shTempSize - NODE_OVERHEAD_SZ;
            if (iBytesToSend > HEAP_SIZE)
                iBytesToSend = HEAP_SIZE; // checking bad node size
            hexDump(pCh + NODE_OVERHEAD_SZ, iBytesToSend, iBytesPerLine);
            break;
        case 'C':
            // It is a candidate
            printf("\t%08lX %-5s %4d\n"
                , ULAddr(pAlloc), "Cand", shTempSize);
            iBytesToSend = shTempSize - NODE_OVERHEAD_SZ;
            if (iBytesToSend > HEAP_SIZE)
                iBytesToSend = HEAP_SIZE; // checking bad node size
            hexDump(pCh + NODE_OVERHEAD_SZ, iBytesToSend, iBytesPerLine);
            break;
        default:
            // It is unknown
            printf("\t%08lX %-5s %4d *** unknown cGC\n"
                , ULAddr(pAlloc), "Unk", shTempSize);
            iBytesToSend = shTempSize - NODE_OVERHEAD_SZ;
            if (iBytesToSend > HEAP_SIZE)
                iBytesToSend = HEAP_SIZE; // checking bad node size
            hexDump(pCh + NODE_OVERHEAD_SZ, iBytesToSend, iBytesPerLine);
        }
        if (shTempSize <= 0)
            errExit("shNodeSize is zero or negative");
    }
}
/******************** printAll *****************************************
    void printAll(StorageManager *pMgr)
Purpose:
    Prints all nodes referenced by the hash table which have a pointer
    value which isn't NULL.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
Notes:
    - This uses the hash table C++ interface hashApi to return a
      message object (structure in C) that contains the pairs of keys
      and values.
**************************************************************************/
void printAll(StorageManager *pMgr)
{
    int i;
    void *pUserData;
    HashMO hashMO;

    // get the hash table entries as an array
    getAll(&hashMO);

    // Iterate through the entire hash table
    for (i = 0; i < hashMO.iNumEntries; i += 1)
    {
        pUserData = hashMO.entryM[i].pUserData;
        // Print the node if the hash value is not NULL
        if (pUserData != NULL)
            printNode(pMgr, pUserData);
    }
}
/******************** garbageCollection **************************************
    void garbageCollection(StorageManager *pMgr, MMResult *pmmResult)
Purpose:
    Executes all subphases of the garbage collection process.  This includes
        Mark Subphase       Walk through all memory from the beginning and mark
                            all nodes to a cGC of 'C' (candidate).  It is easier
                            to combine adjacent free areas if all nodes are
                            marked as 'C'.
        Follow Subphase     From any valid starting point (which we are simulating
                            with a hash table), follow the nodes based on metadata
                            and mark each referenceable node as 'U' (in use).
        Collection Subphase We will build the free list from the entire memory.
                            (We are affectively ignoring the old free linked list.)
                            Walk through all the memory looking for 'C' nodes, combining
                            adjacent ones into a single free area.  Each free area
                            will be placed on the front of the new free list.
Parameters:
    I StorageManager *pMgr  Provides metadata about the user data and
                            information for storage management.
    O MMResult *pmmResult   Result strcuture for returning errors
Returns:
    n/a
Notes:
    - This uses the hash table C++ interface hashApi to return a
      message object (structure in C) that contains the pairs of keys
      and values.
**************************************************************************/
void garbageCollection(StorageManager *pMgr, MMResult *pmmResult)
{
    int i;
    void *pUserData;
    HashMO hashMO;
    // mark all nodes as candidates ('C') for collection
    mmMark(pMgr, pmmResult);
    if (pmmResult->rc != 0)
    {
        printf("\t\tmmMark rc=%d, %s\n"
            , pmmResult->rc
            , pmmResult->szErrorMessage);
        return;
    }
    // To get the references, we need the contents of the hash table.
    // In most GC solutions, we would get the entries from either the
    // runtime memory stack or (in the case of Python) a hash table.
    getAll(&hashMO);

    // go through the array of hash entries, calling mmFollow
    for (i = 0; i < hashMO.iNumEntries; i += 1)
    {
        pUserData = hashMO.entryM[i].pUserData;
        if (pUserData != NULL)
        {   // mark any reachable nodes as in use ('U')
            mmFollow(pMgr, pUserData, pmmResult);
            if (pmmResult->rc != 0)
            {
                printf("\t\tmmFollow rc=%d, %s\n"
                    , pmmResult->rc
                    , pmmResult->szErrorMessage);
                return;
            }
        }
    }
    // Collect all the candidate entries as free.
    mmCollect(pMgr, pmmResult);
    if (pmmResult->rc != 0)
    {
        printf("\t\tmmCollect rc=%d, %s\n"
            , pmmResult->rc
            , pmmResult->szErrorMessage);
        return;
    }
}

/*
** This Hex Dump can be used instead of calling the real
** hexDump from smDump when using Visual Studio.  Rename this to hexDump.
** Please remember to remove it when you run your code on a fox server.
*/
int dumbHexDump(char *sbBuffer, int iBufferLength, int iBytesPerLine)
{
    printf("\t\t\t, %s", sbBuffer);  // print the data
    return 1;	// arbitrary value and meaningless in this case
}
/*
** This is the dumb version of print node which can be used instead
** of calling the real printnode.  Rename this to printNode.
** Please remember to remove it when you run your code on a fox server.
*/
void dumbPrintNode(StorageManager *pMgr, void *pUserData)
{
    InUseNode *pAlloc = (InUseNode *)(((char *)pUserData) - 2 * sizeof(short) - 1);
    printf("\t%-14s %-4s %-9s %-14s\n ", "Alloc Address", "Size", "Node Type", "Data Address");
    printf("\t%p      %4i %6i    %08lX\n", pAlloc
        , pAlloc->shNodeSize, pAlloc->shNodeType, ULAddr(pUserData));

    printf("dumb print");
}

/******************** getSimpleToken **************************************
 int getSimpleToken(char szInput[], char szDelims[]
     , int *piBufferPosition, char szToken[])
 Purpose:
    Returns the next token in a string.  The delimiter(s) for the token is
    passed as a parameter.  To help positioning for a subsequent call, it
    also returns the next position in the buffer.
Parameters:
    I   char szInput[]          input buffer to be parsed
    I   char szDelims[]         delimiters for tokens
    I/O int *piBufferPosition   Position to begin looking for the next token.
                                This is also used to return the next
                                position to look for a token (which will
                                follow the delimiter).
    O   char szToken[]          Returned token.
Returns:
    Functionally:
        TRUE - a token is returned
        FALSE - no more tokens
    iBufferPosition parm - the next position for parsing
    szToken parm - the returned token.  If not found, it will be an empty string.
Notes:
    - If the token is larger than MAX_TOKEN_SIZE, we return a truncated value.
**************************************************************************/

int getSimpleToken(char szInput[], const char szDelims[]
    , int *piBufferPosition, char szToken[])
{
    int iDelimPos;                      // found position of delim
    int iCopy;                          // number of characters to copy

    // check for past the end of the string
    if (*piBufferPosition >= (int) strlen(szInput))
    {
        szToken[0] = '\0';              // mark it empty
        return FALSE;                   // no more tokens
    }

    // get the position of the first delim, relative to the iBufferPosition
    iDelimPos = strcspn(&szInput[*piBufferPosition], szDelims);

    // see if we have more characters than target token, if so, trunc
    if (iDelimPos > MAX_TOKEN_SIZE)
        iCopy = MAX_TOKEN_SIZE;             // truncated size
    else
        iCopy = iDelimPos;

    // copy the token into the target token variable
    memcpy(szToken, &szInput[*piBufferPosition], iCopy);
    szToken[iCopy] = '\0';              // null terminate

    // advance the position
    *piBufferPosition += iDelimPos + 1;
    return TRUE;
}


/******************** errExit **************************************
  void errExit(const char szFmt[], ... )
Purpose:
    Prints an error message defined by the printf-like szFmt and the
    corresponding arguments to that function.  The number of
    arguments after szFmt varies dependent on the format codes in
    szFmt.
    It also exits the program.
Parameters:
    I   const char szFmt[]      This contains the message to be printed
                                and format codes (just like printf) for
                                values that we want to print.
    I   ...                     A variable-number of additional arguments
                                which correspond to what is needed
                                by the format codes in szFmt.
Returns:
    Exits the program with a return code of 99.
Notes:
    - Prints "ERROR: " followed by the formatted error message specified
      in szFmt.
    - Requires including <stdarg.h>
**************************************************************************/
void errExit(const char szFmt[], ... )
{
    va_list args;               // This is the standard C variable argument list type
    va_start(args, szFmt);      // This tells the compiler where the variable arguments
                                // begins.  They begin after szFmt.
    printf("ERROR: ");
    vprintf(szFmt, args);       // vprintf receives a printf format string and  a
                                // va_list argument
    va_end(args);               // let the C environment know we are finished with the
                                // va_list argument
    printf("\n");
    exit(99);
}

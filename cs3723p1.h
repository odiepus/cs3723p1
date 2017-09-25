/**********************************************************************
cs3723p1.h
Purpose:
    Defines constants for
        boolean values
        maximum sizes
        program return codes
        error messages
    Defines typedefs for
        MetaAttr - describes one attribute within a node type
        NodeType - describes one node type
        InUseNode - represents an allocated node that will be in use.
            The actual size of an InUse node is actually larger than
            what the user requested.  The size of an allocated
            item cannot be less than the size of a FreeNode.
        FreeNode - represents a node that is on the free list.
        StorageManager - the primary structure used by this program.
        MMResult - used by the mm... functions to
            specify whether they executed successfully.
        HashEntryPair - hash key and value pair used in the HashMO
        HashMO - returned by getAll. This contains an array of
            HashEntryPair and the number of entries in that array.
    Prototypes
Notes:

**********************************************************************/
#define TRUE 1
#define FALSE 0

#define MAX_KEY_SIZE 10                     // Maximum size of a key for Hash Table
#define MAX_MESSAGE_SIZE 100                // Maximum message size for smResult
#define MAX_STRING 30                       // Maximum size of strings like
                                            // node type names, attribute names
#define MAX_NODE_TYPE 5	                    // Maximum number of node types
#define MAX_NODE_ATTR 50                    // Maximum number of combined node attr
#define MAX_DATA_SZ 500                     // Maximum size of sbData
#define ERROR_PROCESSING 3                  // error during processing - exit value
#define MAX_HASH_ENTRIES 100                // Maximum number of hash entries

#define NOT_FOUND -1                        // Could not find name in metadata

// Errors returned in the rc of MMResult
#define RC_NOT_AVAIL 901            // There isn't any free memory to handle alloc request
#define RC_INVALID_ADDR 903         // Invalid address which isn't within heap
#define RC_ASSOC_ATTR_NOT_PTR 801   // Attribute name for ASSOC not a pointer attribute
#define RC_ASSOC_ATTR_NOT_FOUND 802 // Attribute name for ASSOC not found for the from node

// MetaAttr describes an attribute within a node type
typedef struct MetaAttr
{
    short  shNodeType;                      // Type of node
    char   szAttrName[MAX_STRING+1];        // Name of the attribute
    char   cDataType;                       // Data type: S - char string, P -Ptr, D - double, I - int
    short  shSizeBytes;                     // size in bytes including zero byte for strings
    short  shOffset;
}MetaAttr;
// NodeType describes one type of node
typedef struct NodeType
{
    char szNodeTypeNm[MAX_STRING+1];
    short shBeginMetaAttr;              // Subscript in metaAttrM of first attribute for
                                        // this node type.
    short shNodeTotalSize;
}NodeType;

// InUseNode represents an allocated node.  The actual size of an allocated item may be much
// larger.
typedef struct InUseNode
{
    short shNodeSize;                   // total size of the allocated item.
    short shNodeType;                   // Node Type subscript.
    char  cGC;                          // Garbage Collection status byte has one of these
                                        // values:  F - free, C - candidate to free,
                                        //          U - in use
    char  sbData[MAX_DATA_SZ];          // This is the user's data in the node.  It might
                                        // be bigger than MAX_STRING.
} InUseNode;

// Define the size of overhead in an InUseNode
#define NODE_OVERHEAD_SZ (sizeof(short)+sizeof(short)+1)

// FreeNode represent a free node.  Note that an actual free node
// will occupt more than the size of this structure.
typedef struct FreeNode
{
    short shNodeSize;                   // Total size of this free node.
    short shNodeType;                   // Not used
    char  cGC;                          // Garbage Collection status byte has one of these
                                        // values:  F - free, C - candidate to free,
                                        //          U - in use
    struct FreeNode *pFreeNext;         // Points to next free node
} FreeNode;

// StorageManager is the primary structure used by this program.
typedef struct
{
    int iHeapSize;                       // Total size of the heap memory being managed
    int iMinimumNodeSize;                // The minimum size of any node.
    char *pBeginStorage;                 // Beginning of the heap memory being managed
    char *pEndStorage;                   // End address immediately after the heap memory
    FreeNode *pFreeHead;                 // Head of the free list
    NodeType nodeTypeM[MAX_NODE_TYPE];   // array of node types
    MetaAttr metaAttrM[MAX_NODE_ATTR];   // array of attribute meta data
} StorageManager;

// This is returned by many of the mm functions via the parameter list.
typedef struct
{
    int rc;                                // Return Code is 0 if it is normal.  Otheriwise,
                                           // it is not zero.  See the defined constants.
    char szErrorMessage[MAX_MESSAGE_SIZE + 1];  // If a problem is encountered, this should
                                                // explain the error.
} MMResult;

// This is for returning one Hash Table entry pair
typedef struct
{
    char szKey[MAX_KEY_SIZE + 1];           // the hash key
    void *pUserData;                        // the entry contains just a ptr
} HashEntryPair;
// This is used to return the entire contents of the Hash Table
typedef struct
{
    int iNumEntries;
    HashEntryPair entryM[MAX_HASH_ENTRIES];
} HashMO;

// student functions
void  *mmAllocate(StorageManager *pMgr
    , short shDataSize, short shNodeType, char sbData[], MMResult *pmmResult);
void mmInit(StorageManager *pMgr);
void mmMark(StorageManager *pMgr, MMResult *pmmResult);
void mmFollow(StorageManager *pMgr, void *pUserData, MMResult *pmmResult);
void mmCollect(StorageManager *pMgr, MMResult *pmmResult);
void mmAssoc(StorageManager *pMgr
    , void *pUserDataFrom, char szAttrName[], void *pUserDataTo, MMResult *pmmResult);
short combine(char *p, InUseNode *pAlloc, short size);

// Driver functions
void smPrintMeta(StorageManager *pMgr);
void smPrintFree(StorageManager *pMgr);
short findNodeType(StorageManager *pMgr, char szNodeTypeNm[]);
void smInit(StorageManager *pMgr);
void smDump(StorageManager *pMgr);
void garbageCollection(StorageManager *pMgr, MMResult *pmmResult);
void printAll(StorageManager *pMgr);

void errExit(const char szFmt[], ...);

// Larry provided .o versions of these functions.
// If you are running your code on Microsoft, you must use
// the dummy versions
void printNode(StorageManager *pMgr, void *pUserData);
int hexDump(char *psbBuffer, int iBufferLength, int iBytesPerLine);

// Simple macro for converting addresses to unsigned long
#if defined(_WIN32)
#define ULAddr(addr) ((unsigned long) addr)
#else
#define ULAddr(addr) (((unsigned long) addr)&0x00000000FFFFFFFF)
#endif

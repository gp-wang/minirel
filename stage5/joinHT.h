
class joinHashTbl
{
private:
    union JAttrType
    {
	int iValue;
	float fValue;
	char* sValue;
    };
	
    struct joinhashBucket
    {
	union JAttrType	attrValue;
       	RID	rid;
       	joinhashBucket*     next;    // next node in the hash table
    };

    struct HTentry
    {
	int bucketCnt;  // nuumber of buckets on this chain
	joinhashBucket*   chain;  // pointer to first bucket on the chain
    };

    AttrDesc 	joinAttr;
    int 	HTSIZE;
    HTentry 	*ht; // actual hash table
    int  hash(const char* attr, int attrType); // returns value between 0 and HTSIZE-1

public:
    joinHashTbl(const int size, const AttrDesc attr);  // constructor
    ~joinHashTbl();

     // insert a new (JoinAttrValue, RID) pair into hash table
     Status insert(const RID newRid,  const char* tuple);

     // get RIDs of records whose join attribute value matches innerJoinAttrValue
     Status lookup(const char* innerJoinAttrPtr, int & ridCount, RID *&outRids);
};


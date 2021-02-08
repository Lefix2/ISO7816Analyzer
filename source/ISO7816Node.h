#ifndef ISO7816_NODE_H
#define ISO7816_NODE_H


#include <Analyzer.h>
#include <vector>
#include <string>

typedef enum{
    nodeLevel_char,
    nodeLevel_tpdu,
    nodeLevel_apdu,
    nodeLevel_pps,
    nodeLevel_atr,
    nodeLevel_transaction,
    nodeLevel_count_or_invalid,
}nodeLevel_t;

typedef enum
{
	sender_card,
	sender_reader,
	sender_undefined,
	sender_count_or_invalid
}sender_t;

/*
 * ISO7816 Node abstract class definition
 */
class ISO7816Node
{
public:
	ISO7816Node(sender_t sender, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816Node();

public:
    void SetNodeId(U64 nodeId);
    U64 GetNodeId(void);
    void SetStartSample(S64 startSample);
    S64 GetStartSample(void);
    void SetEndSample(S64 endSample);
    S64 GetEndSample(void);
    sender_t GetSender(void);

    void AddChildNode(ISO7816Node* child);
    ISO7816Node* GetFirstNode(void);
    ISO7816Node* GetLastNode(void);
    virtual nodeLevel_t GetLevel(void);
    virtual void GetDataStr(char* resultString, U32 maxStrLen) = 0;

protected: //vars
    sender_t mSender;
    S64 mStartSample;
    S64 mEndSample;
    U64 mNodeId;
    std::vector<ISO7816Node*> mChilds;
};


/*
 * ISO7816 Node for transaction class definition
 */
class ISO7816NodeTransaction : public ISO7816Node
{
public:
	ISO7816NodeTransaction(S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816NodeTransaction();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);
};


/*
 * ISO7816 Node for PPS class definition
 */
class ISO7816NodePPS : public ISO7816Node
{
public:
	ISO7816NodePPS(sender_t sender, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816NodePPS();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);
};


/*
 * ISO7816 Node for ATR class definition
 */
class ISO7816NodeATR : public ISO7816Node
{
public:
	ISO7816NodeATR(S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816NodeATR();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);
};


/*
 * ISO7816 Node for character class definition
 */
class ISO7816NodeChar : public ISO7816Node
{
public:
    ISO7816NodeChar(sender_t sender, U8 charVal, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
    virtual ~ISO7816NodeChar();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);

public:
    U8 mCharVal;

};

#endif //ISO7816_NODE_H

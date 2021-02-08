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

class ISO7816Node
{
public:
	ISO7816Node(S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816Node();

public:
    void SetNodeId(U64 nodeId);
    U64 GetNodeId(void);
    void SetStartSample(S64 startSample);
    S64 GetStartSample(void);
    void SetEndSample(S64 endSample);
    S64 GetEndSample(void);

    void AddChildNode(ISO7816Node* child);
    ISO7816Node* GetFirstNode(void);
    ISO7816Node* GetLastNode(void);
    virtual nodeLevel_t GetLevel(void);
    virtual void GetDataStr(char* resultString, U32 maxStrLen) = 0;

protected: //vars
    S64 mStartSample;
    S64 mEndSample;
    U64 mNodeId;
    std::vector<ISO7816Node*> mChilds;
};

class ISO7816NodeTransaction : public ISO7816Node
{
public:
	ISO7816NodeTransaction(S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816NodeTransaction();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);
};

class ISO7816NodeATR : public ISO7816Node
{
public:
	ISO7816NodeATR(S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
	virtual ~ISO7816NodeATR();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);
};

class ISO7816NodeChar : public ISO7816Node
{
public:
    ISO7816NodeChar(U8 charVal, S64 startSample = 0, S64 endSample = 0, U64 nodeId = 0);
    virtual ~ISO7816NodeChar();

public:
    nodeLevel_t GetLevel(void);
    void GetDataStr(char* resultString, U32 maxStrLen);

public:
    U8 mCharVal;

};

#endif //ISO7816_NODE_H

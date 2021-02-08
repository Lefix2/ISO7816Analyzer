#include "ISO7816Node.h"

ISO7816Node::ISO7816Node(S64 startSample, S64 endSample, U64 nodeId)
:   mStartSample(startSample),
    mEndSample(endSample),
    mNodeId(nodeId)
{
}

ISO7816Node::~ISO7816Node()
{
    mChilds.empty();
}
void ISO7816Node::SetNodeId(U64 nodeId)
{
    mNodeId = nodeId;
}

U64 ISO7816Node::GetNodeId(void)
{
    return mNodeId;
}

void ISO7816Node::SetStartSample(S64 startSample)
{
    mStartSample = startSample;
}

S64 ISO7816Node::GetStartSample(void)
{
    return mStartSample;
}

void ISO7816Node::SetEndSample(S64 endSample)
{
    mEndSample = endSample;
}

S64 ISO7816Node::GetEndSample(void)
{
    return mEndSample;
}

nodeLevel_t ISO7816Node::GetLevel(void)
{
    return nodeLevel_count_or_invalid;
}

ISO7816Node* ISO7816Node::GetFirstNode(void)
{
    return mChilds.front();
}

ISO7816Node* ISO7816Node::GetLastNode(void)
{
    return mChilds.back();
}

void ISO7816Node::AddChildNode(ISO7816Node* child)
{
    mChilds.push_back(child);
}


ISO7816NodeTransaction::ISO7816NodeTransaction(S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(startSample, endSample, nodeId)
{}

ISO7816NodeTransaction::~ISO7816NodeTransaction()
{}

nodeLevel_t ISO7816NodeTransaction::GetLevel(void)
{
    return nodeLevel_transaction;
}

void ISO7816NodeTransaction::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "ISO7816 Transaction");
}


ISO7816NodeATR::ISO7816NodeATR(S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(startSample, endSample, nodeId)
{

}

ISO7816NodeATR::~ISO7816NodeATR()
{}

nodeLevel_t ISO7816NodeATR::GetLevel(void)
{
    return nodeLevel_atr;
}

void ISO7816NodeATR::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "ATR");
}


ISO7816NodeChar::ISO7816NodeChar(U8 charVal, S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(startSample, endSample, nodeId),
    mCharVal(charVal)
{

}

ISO7816NodeChar::~ISO7816NodeChar()
{}

nodeLevel_t ISO7816NodeChar::GetLevel(void)
{
    return nodeLevel_char;
}

void ISO7816NodeChar::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "0x%02X", mCharVal);
}
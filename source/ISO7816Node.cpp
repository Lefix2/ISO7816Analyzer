#include "ISO7816Node.h"

#include "ISO7816Exception.h"

/*
 * ISO7816 Node abstract class implementation
 */
ISO7816Node::ISO7816Node(nodeLevel_t nodeLevel, sender_t sender, S64 startSample, S64 endSample, U64 nodeId)
:   mSender(sender),
    mNodeLevel(nodeLevel),
    mStartSample(startSample),
    mEndSample(endSample),
    mNodeId(nodeId),
    mDescription()
{}

ISO7816Node::~ISO7816Node()
{
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

sender_t ISO7816Node::GetSender(void)
{
    return mSender;
}

nodeLevel_t ISO7816Node::GetLevel(void)
{
    return mNodeLevel;
}

ISO7816Node* ISO7816Node::GetNodeAt(S64 index)
{
    if (index >= 0)
        return mChilds.at(index);
    else if(-index < (S64)mChilds.size())
        return mChilds.at((S64)mChilds.size() + index);

    throw ISO7816ExceptionExecution("Bad index");
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

void ISO7816Node::AddDescription(const char* str)
{
    mDescription += str;
}


/*
 * ISO7816 Node for APDU class definition
 */
ISO7816NodeAPDU::ISO7816NodeAPDU(S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(nodeLevel_apdu, sender_undefined, startSample, endSample, nodeId)
{}

ISO7816NodeAPDU::~ISO7816NodeAPDU()
{}

void ISO7816NodeAPDU::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "APDU");
}


/*
 * ISO7816 Node for TPDU class definition
 */
ISO7816NodeTPDU::ISO7816NodeTPDU(S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(nodeLevel_tpdu, sender_undefined, startSample, endSample, nodeId)
{}

ISO7816NodeTPDU::~ISO7816NodeTPDU()
{}

void ISO7816NodeTPDU::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "TPDU");
}



/*
 * ISO7816 Node for PPS class implementation
 */
ISO7816NodePPS::ISO7816NodePPS(sender_t sender, S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(nodeLevel_pps, sender, startSample, endSample, nodeId)
{}

ISO7816NodePPS::~ISO7816NodePPS()
{}

void ISO7816NodePPS::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "PPS");
}


/*
 * ISO7816 Node for ATR class implementation
 */
ISO7816NodeATR::ISO7816NodeATR(S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(nodeLevel_atr, sender_card, startSample, endSample, nodeId)
{}

ISO7816NodeATR::~ISO7816NodeATR()
{}

void ISO7816NodeATR::GetDataStr(char* resultString, U32 maxStrLen)
{
    snprintf(resultString, maxStrLen, "ATR");
}


/*
 * ISO7816 Node for character class implementation
 */
ISO7816NodeChar::ISO7816NodeChar(sender_t sender, U8 charVal, S64 startSample, S64 endSample, U64 nodeId)
:   ISO7816Node(nodeLevel_char, sender, startSample, endSample, nodeId),
    mCharVal(charVal)
{}

ISO7816NodeChar::~ISO7816NodeChar()
{}

void ISO7816NodeChar::GetDataStr(char* resultString, U32 maxStrLen)
{
    if (mDescription.empty())
    {
        snprintf(resultString, maxStrLen, "0x%02X", mCharVal);
    }
    else
    {
        snprintf(resultString, maxStrLen, "%s(0x%02X)", mDescription.c_str(), mCharVal);
    }
}
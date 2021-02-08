#ifndef ISO7816_PROTOCOLTPDUT0_H
#define ISO7816_PROTOCOLTPDUT0_H

#include "ISO7816Node.h"
#include "ISO7816ProtocolLayer.h"

typedef enum {
    stateTPDUT0_HEADER,
    stateTPDUT0_procedure,
    stateTPDUT0_SW2,
    stateTPDUT0_Bytes,
    stateTPDUT0_Byte,
    stateTPDUT0_finished,
    stateTPDUT0_count_or_invalid
}stateTPDUT0_t;


class ISO7816ProtocolTPDUT0 : public ISO7816ProtocolLayer
{
public:
    ISO7816ProtocolTPDUT0(ISO7816Analyzer* analyzer);
    virtual ~ISO7816ProtocolTPDUT0();

    virtual void initTransaction (void);
    virtual bool isTransactionComplete(void);
    virtual void newData(ISO7816Node* node);

protected: // functions
    void nextState(ISO7816NodeChar* charNode);

protected: //vars
    stateTPDUT0_t       mStateTPDUT0;
    U64                 mDataLen;
    ISO7816NodeTPDU*    mNode;
};

#endif // ISO7816_PROTOCOLTPDUT0_H
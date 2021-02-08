#ifndef ISO7816_PROTOCOLCHAR_H
#define ISO7816_PROTOCOLCHAR_H

#include "ISO7816ProtocolLayer.h"
#include "ISO7816ProtocolATR.h"
#include "ISO7816ProtocolPPS.h"
#include "ISO7816ProtocolTPDUT0.h"

class ISO7816ProtocolChar : public ISO7816ProtocolLayer
{
public:
	ISO7816ProtocolChar(ISO7816Analyzer* analyzer);
	virtual ~ISO7816ProtocolChar();

    virtual void initTransaction (void);
    virtual bool isTransactionComplete(void);
    virtual void newData(ISO7816Node* node);

protected: // functions
    void nextState(U8 data);

protected: //vars
    ISO7816ProtocolATR      mProtocolAtr;
    ISO7816ProtocolPPS      mProtocolPPS;
    ISO7816ProtocolTPDUT0   mProtocolTPDUT0;
};

#endif //ISO7816_PROTOCOLCHAR_H
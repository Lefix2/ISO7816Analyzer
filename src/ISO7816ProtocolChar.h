#ifndef ISO7816_PROTOCOLCHAR_H
#define ISO7816_PROTOCOLCHAR_H

#include "ISO7816ProtocolLayer.h"
#include "ISO7816ProtocolATR.h"
#include "ISO7816ProtocolPPS.h"
#ifndef LOGIC2
#include "ISO7816ProtocolTPDUT0.h"
#include "ISO7816ProtocolTPDUT1.h"
#endif

class ISO7816ProtocolChar : public ISO7816ProtocolLayer
{
  public:
    ISO7816ProtocolChar( ISO7816Analyzer* analyzer );
    virtual ~ISO7816ProtocolChar();

    virtual void initTransaction( void );
    virtual bool isTransactionComplete( void );
    virtual void newData( ISO7816Node* node );

  protected: // functions
    void nextState( U8 data );

  protected: // vars
    ISO7816ProtocolATR mProtocolAtr;
    ISO7816ProtocolPPS mProtocolPPS;
#ifndef LOGIC2
    ISO7816ProtocolTPDUT0 mProtocolTPDUT0;
    ISO7816ProtocolTPDUT1 mProtocolTPDUT1;
#endif
};

#endif // ISO7816_PROTOCOLCHAR_H
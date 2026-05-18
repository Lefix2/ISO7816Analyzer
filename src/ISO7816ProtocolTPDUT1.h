#ifndef ISO7816_PROTOCOLTPDUT1_H
#define ISO7816_PROTOCOLTPDUT1_H

#include "ISO7816Node.h"
#include "ISO7816ProtocolLayer.h"

typedef enum
{
    stateTPDUT1_NAD,
    stateTPDUT1_PCB,
    stateTPDUT1_LEN,
    stateTPDUT1_DATA,
    stateTPDUT1_EDC,
    stateTPDUT1_count_or_invalid
} stateTPDUT1_t;

class ISO7816ProtocolTPDUT1 : public ISO7816ProtocolLayer
{
  public:
    ISO7816ProtocolTPDUT1( ISO7816Analyzer* analyzer );
    virtual ~ISO7816ProtocolTPDUT1();

    virtual void initTransaction( void );
    virtual bool isTransactionComplete( void );
    virtual void newData( ISO7816Node* node );

  protected:
    void nextState( ISO7816NodeChar* charNode );
    void emitAPDU( void );
    static void GetBlockDesc( U8 pcb, char* buf, U32 maxLen );

    stateTPDUT1_t mState;
    U8 mPCB;
    U16 mLEN;
    U16 mDataCount;
    U8 mEDCLen;
    U8 mEDCCount;

    bool mAPDUActive;
    S64 mAPDUStartSample;
    S64 mAPDUEndSample;
    U8 mAPDUFirst8[ 8 ];
    U8 mAPDUFirstCount;
    U16 mAPDUTotalLen;

    ISO7816NodeTPDU* mNode;
};

#endif // ISO7816_PROTOCOLTPDUT1_H

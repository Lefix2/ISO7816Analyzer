#ifndef ISO7816_PROTOCOLATR_H
#define ISO7816_PROTOCOLATR_H

#include "ISO7816Node.h"
#include "ISO7816ProtocolLayer.h"

typedef enum
{
    stateATR_TS,
    stateATR_T0,
    stateATR_TA,
    stateATR_TB,
    stateATR_TC,
    stateATR_TD,
    stateATR_Historical,
    stateATR_TCK,
    stateATR_finished,
    stateATR_count_or_invalid
} stateATR_t;

class ISO7816ProtocolATR : public ISO7816ProtocolLayer
{
  public:
    ISO7816ProtocolATR( ISO7816Analyzer* analyzer );
    virtual ~ISO7816ProtocolATR();

    virtual void initTransaction( void );
    virtual bool isTransactionComplete( void );
    virtual void newData( ISO7816Node* node );

  protected: // functions
    void nextTDState( void );
    sc_convention_t GetGlobalConvention( void );
    U32 GetGlobalfMax( void );
    U32 GetGlobalFi( void );
    U32 GetGlobalDi( void );
    U8 GetGlobalN( void );
    U8 GetGlobalWI( void );
    U8 GetT1SpecificEDC( void );
    U8 GetT1SpecificIFS( void );
    U8 GetT1SpecificCWI( void );
    U8 GetT1SpecificBWI( void );
    bool IsNegociableMode( void );
    void decodeATR( void );

  protected: // vars
    stateATR_t mStateATR;
    U8 mTDi;
    U8 mNb_T;
    ISO7816NodeATR* mNode;
};

#endif // ISO7816_PROTOCOLATR_H
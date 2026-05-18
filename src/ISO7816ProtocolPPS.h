#ifndef ISO7816_PROTOCOLPPS_H
#define ISO7816_PROTOCOLPPS_H

#include "ISO7816Node.h"
#include "ISO7816ProtocolLayer.h"

typedef enum
{
    statePPS_PPSS,
    statePPS_PPS0,
    statePPS_PPS1,
    statePPS_PPS2,
    statePPS_PPS3,
    statePPS_PCK,
    statePPS_finished,
    statePPS_count_or_invalid
} statePPS_t;


class ISO7816ProtocolPPS : public ISO7816ProtocolLayer
{
  public:
    ISO7816ProtocolPPS( ISO7816Analyzer* analyzer );
    virtual ~ISO7816ProtocolPPS();

    virtual void initTransaction( void );
    virtual bool isTransactionComplete( void );
    virtual void newData( ISO7816Node* node );

    bool isPPSStartingWithData( U8 data );

  protected: // functions
    void nextState( void );
    void decodePPS( void );

  protected: // vars
    statePPS_t mStatePPS;
    ISO7816NodePPS* mNode;
};

#endif // ISO7816_PROTOCOLPPS_H
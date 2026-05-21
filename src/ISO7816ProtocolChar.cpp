#include "ISO7816ProtocolChar.h"

ISO7816ProtocolChar::ISO7816ProtocolChar( ISO7816Analyzer* analyzer )
    : ISO7816ProtocolLayer( analyzer ),
      mProtocolAtr( analyzer ),
      mProtocolPPS( analyzer )
#ifndef LOGIC2
      ,
      mProtocolTPDUT0( analyzer ),
      mProtocolTPDUT1( analyzer )
#endif
{
    initTransaction();
}

ISO7816ProtocolChar::~ISO7816ProtocolChar()
{
}

void ISO7816ProtocolChar::initTransaction( void )
{
    mAnalyzer->GetContext()->init();

    mProtocolAtr.initTransaction();
    mProtocolPPS.initTransaction();
#ifndef LOGIC2
    mProtocolTPDUT0.initTransaction();
    mProtocolTPDUT1.initTransaction();
#endif
}

bool ISO7816ProtocolChar::isTransactionComplete( void )
{
    return true;
}

void ISO7816ProtocolChar::newData( ISO7816Node* node )
{
    U8& data = ( dynamic_cast<ISO7816NodeChar*>( node ) )->mCharVal;

    nextState( data );

    switch( mAnalyzer->GetContext()->mState )
    {
    case S_ATR:
        mProtocolAtr.newData( node );
        break;

    case S_PPS:
        mProtocolPPS.newData( node );
        break;

#ifndef LOGIC2
    case S_T0:
        mProtocolTPDUT0.newData( node );
        break;

    case S_T1:
        mProtocolTPDUT1.newData( node );
        break;
#endif

    default:
        // Lost in samples, just display characters
        break;
    }

    mAnalyzer->newFrame( node );
}

void ISO7816ProtocolChar::nextState( U8 data )
{
    switch( mAnalyzer->GetContext()->mState )
    {
    case S_ATR:
        if( mProtocolAtr.isTransactionComplete() )
        {
            if( mProtocolPPS.isPPSStartingWithData( data ) )
            {
                mAnalyzer->GetContext()->mState = S_PPS;
            }
            else
            {
                switch( mAnalyzer->GetContext()->mISOParams.default_protocol )
                {
                case SC_PROTOCOL_T0:
                    mAnalyzer->GetContext()->mState = S_T0;
                    break;

                case SC_PROTOCOL_T1:
                    mAnalyzer->GetContext()->mState = S_T1;
                    break;

                default:
                    mAnalyzer->GetContext()->mState = S_NUMBER_OR_INVALID;
                    break;
                }
            }
        }
        break;

    case S_PPS:
        if( mProtocolPPS.isTransactionComplete() )
            switch( mAnalyzer->GetContext()->mISOParams.default_protocol )
            {
            case SC_PROTOCOL_T0:
                mAnalyzer->GetContext()->mState = S_T0;
                break;

            case SC_PROTOCOL_T1:
                mAnalyzer->GetContext()->mState = S_T1;
                break;

            default:
                mAnalyzer->GetContext()->mState = S_NUMBER_OR_INVALID;
                break;
            }
        break;

    case S_T0:
        // Stay in T0
        break;

    case S_T1:
        // Stay in T1
        break;

    default:
        // Lost in samples, just display characters
        break;
    }
}
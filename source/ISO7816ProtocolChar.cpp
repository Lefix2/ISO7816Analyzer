#include "ISO7816ProtocolChar.h"

ISO7816ProtocolChar::ISO7816ProtocolChar(ISO7816Analyzer* analyzer )
:   ISO7816ProtocolLayer(analyzer),
    mProtocolAtr(analyzer)
{

}

ISO7816ProtocolChar::~ISO7816ProtocolChar()
{

}

void ISO7816ProtocolChar::initTransaction( void )
{
    mAnalyzer->GetContext()->init();

    // Close everything
    mProtocolAtr.initTransaction();
}

bool ISO7816ProtocolChar::isTransactionComplete( void )
{
    return true;
}

void ISO7816ProtocolChar::newData(ISO7816Node* node)
{
    U8 &data = (dynamic_cast<ISO7816NodeChar*>(node))->mCharVal;

    switch (mAnalyzer->GetContext()->mState)
    {
        case S_ATR:
            mProtocolAtr.newData(node);
            if(mProtocolAtr.isTransactionComplete())
                mAnalyzer->GetContext()->mState = S_PPS;
            break;

        case S_PPS:
        break;

        case S_T0:
        break;

        case S_T1:
        break;

        default:
        break;
    }

    mAnalyzer->newFrame(node);
}
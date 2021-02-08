#include "ISO7816ProtocolATR.h"

#include <cstring>

#define ATR_T0_HIST_MASK        0x0F
#define ATR_TDI_TA_MASK         0x10
#define ATR_TDI_TB_MASK         0x20
#define ATR_TDI_TC_MASK         0x40
#define ATR_TDI_TD_MASK         0x80


ISO7816ProtocolATR::ISO7816ProtocolATR(ISO7816Analyzer* analyzer)
:   ISO7816ProtocolLayer(analyzer),
    mNode(NULL)
{
    initTransaction();
}

ISO7816ProtocolATR::~ISO7816ProtocolATR()
{
    delete mNode;
}

void ISO7816ProtocolATR::initTransaction (void)
{
    mStateATR = stateATR_TS;
    atr_init(&mATR);
    mTDi = 0;
    mNb_T = 0;
    
    delete mNode;
    mNode = new ISO7816NodeATR();
}

bool ISO7816ProtocolATR::isTransactionComplete(void)
{
    return mStateATR == stateATR_finished;
}

void ISO7816ProtocolATR::newData(ISO7816Node* node)
{
    mNode->AddChildNode(node);

    U8 &data = (dynamic_cast<ISO7816NodeChar*>(node))->mCharVal;

    switch (mStateATR)
    {      
        case stateATR_TS:
            if(data == 0x3B)
                mAnalyzer->GetContext()->mISOParams.convention = convention_direct;
            else if(data == 0x03)
            {
                data = 0x3F;
                mAnalyzer->GetContext()->mISOParams.convention = convention_reverse;
            }
            else
            {
                throw ISO7816ExceptionProtocol("Bad TS");
            }
            mATR.TS = data;
            break;

        case stateATR_T0:
            mATR.T0 = data;
            mTDi    = data;
            mNb_T   = 0;
            break;

        case stateATR_TA:
			mATR.T[mNb_T][ATR_INTERFACE_A].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_A].value    = data;
            break;

        case stateATR_TB:
			mATR.T[mNb_T][ATR_INTERFACE_B].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_B].value    = data;
            break;

        case stateATR_TC:
			mATR.T[mNb_T][ATR_INTERFACE_C].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_C].value    = data;
            break;

        case stateATR_TD:
			mATR.T[mNb_T][ATR_INTERFACE_D].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_D].value    = data;
            mNb_T++;
            mTDi = data;
			mATR.TCK.present = ((mTDi & 0x0F) != 0x00)?true:false;
            break;

        case stateATR_Historical:
            mATR.HB[mATR.nb_HB] = data;
            mATR.nb_HB++;
            break;

        case stateATR_TCK:
            mATR.TCK.value = data;
            break;

        default:
            throw ISO7816ExceptionProtocol("Invalid ATR state");
            break;
    }
    
    nextTDState();

    if(mStateATR == stateATR_finished)
    {
        mNb_T++;
        mNode->SetStartSample(mNode->GetFirstNode()->GetStartSample());
        mNode->SetEndSample(mNode->GetLastNode()->GetEndSample());
        mAnalyzer->newFrame(mNode);
        mNode = NULL;
        mAnalyzer->GetContext()->toggleSender();
    }
}

void ISO7816ProtocolATR::nextTDState(void)
{
    switch(mStateATR)
    {
        case stateATR_TS:
            mStateATR = stateATR_T0;
            return;

        case stateATR_T0:
        case stateATR_TD:
            if(GETBIT(mTDi, ATR_TDI_TA_MASK))
            {
                mStateATR = stateATR_TA;
                return;
            }
            //nobreak

        case stateATR_TA:
            if(GETBIT(mTDi, ATR_TDI_TB_MASK))
            {
                mStateATR = stateATR_TB;
                return;
            }
            // nobreak

        case stateATR_TB:
            if(GETBIT(mTDi, ATR_TDI_TC_MASK))
            {
                mStateATR = stateATR_TC;
                return;
            }
            // nobreak;

        case stateATR_TC:
            if(GETBIT(mTDi, ATR_TDI_TD_MASK))
            {
                mStateATR = stateATR_TD;
                return;
            }
            // nobreak

        case stateATR_Historical:
            if(mATR.nb_HB < (mATR.T0 & ATR_T0_HIST_MASK))
            {
                mStateATR = stateATR_Historical;
                return;
            }

            if(mATR.TCK.present)
            {
                mStateATR = stateATR_TCK;
                return;
            }
            
            mStateATR = stateATR_finished;
            return;
            // nobreak

        case stateATR_TCK:
            mStateATR = stateATR_finished;
            return;
            // nobreak;

        default:
            throw ISO7816ExceptionProtocol("Can't compute next state");
    }
}
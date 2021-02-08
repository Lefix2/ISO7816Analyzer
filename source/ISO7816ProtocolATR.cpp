#include "ISO7816ProtocolATR.h"

#include <cstring>


/* nb_Tx[Yi] table, number of interface bytes for a given Y */
static const U8 nb_Tx_table[16] =
{
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/* fmax table, indicated fmax for given TA1[8-5] */
static const U32 fmax_table[16] =
{
	4000000, 5000000, 6000, 8000000, 12000000, 16000000, 20000000, 0, 0, 5000000, 7500000, 10000000, 15000000, 20000000, 0, 0
};

/* Fi table, indicated F for given TA1[8-5] */
static const U16 f_table[16] =
{
	372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048, 0, 0
};

/* Di table, indicated D for given TA1[4-1] */
static const U16 d_table[16] =
{
	0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 0, 0, 0, 0, 0, 0
};

static const U16 i_table[4] =
{
	25, 50, 100, 0
};


ISO7816ProtocolATR::ISO7816ProtocolATR(ISO7816Analyzer* analyzer)
:   ISO7816ProtocolLayer(analyzer)
{
    initTransaction();
}

ISO7816ProtocolATR::~ISO7816ProtocolATR()
{
    
}

void ISO7816ProtocolATR::initTransaction (void)
{
    mStateATR = stateATR_TS;
    memset(&mATR, 0, sizeof(mATR));
    mTDi = 0;
    mNb_T = 0;
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
                mAnalyzer->GetContext()->mConvention = CONV_DIR;
            else if(data == 0x03)
            {
                data = 0x3F;
                mAnalyzer->GetContext()->mConvention = CONV_INV;
            }
            else
            {
                throw ISO7816ExceptionProtocol("Bad TS");
            }
            mATR.TS = data;
            mStateATR = stateATR_T0;
            break;

        case stateATR_T0:
            mATR.T0 = data;
            mTDi    = data;
            mNb_T   = 0;
            nextTDState();
            break;

        case stateATR_TA:
			mATR.T[mNb_T][ATR_INTERFACE_A].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_A].value    = data;
            nextTDState();
            break;

        case stateATR_TB:
			mATR.T[mNb_T][ATR_INTERFACE_B].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_B].value    = data;
            nextTDState();
            break;

        case stateATR_TC:
			mATR.T[mNb_T][ATR_INTERFACE_C].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_C].value    = data;
            nextTDState();
            break;

        case stateATR_TD:
			mATR.T[mNb_T][ATR_INTERFACE_D].present	= true;
			mATR.T[mNb_T][ATR_INTERFACE_D].value    = data;
            mNb_T++;
            mTDi = data;
			mATR.TCK.present = ((mTDi & 0x0F) != 0x00)?true:false;
            nextTDState();
            break;

        case stateATR_Historical:
            mATR.HB[mATR.nb_HB] = data;
            mATR.nb_HB++;
            nextTDState();
            break;

        case stateATR_TCK:
            mATR.TCK.value = data;
            nextTDState();
            break;

        case stateATR_finished:
            break;

        default:
            break;
    }

    if(mStateATR == stateATR_finished)
    {
        mNb_T++;
        mNode->SetStartSample(mNode->GetFirstNode()->GetStartSample());
        mNode->SetEndSample(mNode->GetLastNode()->GetEndSample());
        mAnalyzer->newFrame(mNode);
        // @TODO Call upper layer
    }
}

void ISO7816ProtocolATR::nextTDState(void)
{
    switch(mStateATR)
    {
        case stateATR_T0:
        case stateATR_TD:
            if((mTDi & 0x10) == 0x10)
            {
                mStateATR = stateATR_TA;
                return;
            }
            //nobreak

        case stateATR_TA:
            if((mTDi & 0x20) == 0x20)
            {
                mStateATR = stateATR_TB;
                return;
            }
            // nobreak

        case stateATR_TB:
            if((mTDi & 0x40) == 0x40)
            {
                mStateATR = stateATR_TC;
                return;
            }
            // nobreak;

        case stateATR_TC:
            if((mTDi & 0x80) == 0x80)
            {
                mStateATR = stateATR_TD;
                return;
            }
            // nobreak

        case stateATR_Historical:
            if(mATR.nb_HB < (mATR.T0 & 0x0F))
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
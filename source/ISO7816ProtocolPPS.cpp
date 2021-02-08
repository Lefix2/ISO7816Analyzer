#include "ISO7816ProtocolPPS.h"

#define PPS_PPSS_VALUE          0xFF

#define PPS_PPS0_PROT_OFF       0
#define PPS_PPS0_PROT_MASK      0x0F
#define PPS_PPS0_PPS1_MASK      0x10
#define PPS_PPS0_PPS2_MASK      0x20
#define PPS_PPS0_PPS3_MASK      0x40

#define PPS_PPS1_F_OFF          4
#define PPS_PPS1_F_MASK         0xF0
#define PPS_PPS1_D_OFF          0
#define PPS_PPS1_D_MASK         0x0F

ISO7816ProtocolPPS::ISO7816ProtocolPPS(ISO7816Analyzer* analyzer)
:   ISO7816ProtocolLayer(analyzer)
{
    initTransaction();
}

ISO7816ProtocolPPS::~ISO7816ProtocolPPS()
{

}

void ISO7816ProtocolPPS::initTransaction (void)
{
    mStatePPS   = statePPS_PPSS;
    delete(mNode);
    pps_init(&mPPS);
    mNode = new ISO7816NodePPS(sender_reader);
}

bool ISO7816ProtocolPPS::isTransactionComplete(void)
{
    return mStatePPS == statePPS_finished;
}

void ISO7816ProtocolPPS::newData(ISO7816Node* node)
{
    sender_t sender = mAnalyzer->GetContext()->GetSender();

    mNode->AddChildNode(node);

    U8 &data = (dynamic_cast<ISO7816NodeChar*>(node))->mCharVal;

    if( (sender != sender_card) && (sender != sender_reader))
        throw ISO7816ExceptionProtocol("Invalid sender state for current context in PPS");

    switch (mStatePPS)
    {
        case statePPS_PPSS:
            if (data != PPS_PPSS_VALUE)
                throw ISO7816ExceptionProtocol("Invalid PPSS command");
            mPPS.PPSS = data;
            break;
        
        case statePPS_PPS0:
            mPPS.PPS0 = data;
            break;
        
        case statePPS_PPS1:
            mPPS.PPS1 = data;
            break;
        
        case statePPS_PPS2:
            mPPS.PPS2 = data;
            break;
        
        case statePPS_PPS3:
            mPPS.PPS3 = data;
            break;
        
        case statePPS_PCK:
            mPPS.PCK = data;
            break;
        
        default:
            throw ISO7816ExceptionProtocol("Invalid PPS state");
            break;
    }
    
    nextState();

    if (mStatePPS == statePPS_finished)
    {
        // Give node to analyzer

        // Toggle transmission direction
        sender = mAnalyzer->GetContext()->toggleSender();

        // Prepare to receive if it's card's turn!
        if(sender == sender_card)
        {
            mStatePPS = statePPS_PPSS;
        }
        // Else update transaction parameters
        else
        {
            decodePPS();
            mNode->SetStartSample(mNode->GetFirstNode()->GetStartSample());
            mNode->SetEndSample(mNode->GetLastNode()->GetEndSample());
            mAnalyzer->newFrame(mNode);
            mNode = NULL;
        }
    }
}

bool ISO7816ProtocolPPS::isPPSStartingWithData(U8 data)
{
    if( (mAnalyzer->GetContext()->GetSender() == sender_reader) &&
        (mStatePPS == statePPS_PPSS) &&
        (data == PPS_PPSS_VALUE))
    {
        return true;   
    }
    
    return false;
}

void ISO7816ProtocolPPS::nextState(void)
{
    switch(mStatePPS)
    {
        case statePPS_PPSS:
            mStatePPS = statePPS_PPS0;
            return;
            // nobreak
        
        case statePPS_PPS0:
            if(GETBIT(mPPS.PPS0,PPS_PPS0_PPS1_MASK))
            {
                mStatePPS = statePPS_PPS1;
                return;
            }
            // nobreak
        
        case statePPS_PPS1:
            if(GETBIT(mPPS.PPS0,PPS_PPS0_PPS2_MASK))
            {
                mStatePPS = statePPS_PPS2;
                return;
            }
            // nobreak
        
        case statePPS_PPS2:
            if(GETBIT(mPPS.PPS0,PPS_PPS0_PPS3_MASK))
            {
                mStatePPS = statePPS_PPS3;
                return;
            }
            // nobreak
        
        case statePPS_PPS3:
            mStatePPS = statePPS_PCK;
            return;
            // nobreak
        
        case statePPS_PCK:
            mStatePPS = statePPS_finished;
            return;
            // nobreak
        
        default:
            throw ISO7816ExceptionProtocol("Invalid PPS state");
            // nobreak
    }
}

void ISO7816ProtocolPPS::decodePPS(void)
{
    mAnalyzer->GetContext()->mISOParams.default_protocol = GETVAL(mPPS.PPS0, PPS_PPS0_PROT_MASK,PPS_PPS0_PROT_OFF);
    
    if(GETBIT(mPPS.PPS0,PPS_PPS0_PPS1_MASK))
    {
        mAnalyzer->GetContext()->mISOParams.F = GetFn(GETVAL(mPPS.PPS1, PPS_PPS1_F_MASK, PPS_PPS1_F_OFF));
        mAnalyzer->GetContext()->mISOParams.D = GetDn(GETVAL(mPPS.PPS1, PPS_PPS1_D_MASK, PPS_PPS1_D_OFF));
    }
    
    if(GETBIT(mPPS.PPS0,PPS_PPS0_PPS2_MASK))
    {
        if (mPPS.PPS2 != 0x00)
        {
            mAnalyzer->GetContext()->mISOParams.SPU.present = true;
        }
        mAnalyzer->GetContext()->mISOParams.SPU.value = mPPS.PPS2; 
    }
    
    if(GETBIT(mPPS.PPS0,PPS_PPS0_PPS3_MASK))
    {
        // RFU
    }
}
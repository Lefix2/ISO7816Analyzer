#include "ISO7816ProtocolPPS.h"

#include "ISO7816Exception.h"

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
:   ISO7816ProtocolLayer(analyzer),
    mNode(NULL)
{
    initTransaction();
}

ISO7816ProtocolPPS::~ISO7816ProtocolPPS()
{
    delete mNode;
}

void ISO7816ProtocolPPS::initTransaction (void)
{
    mStatePPS   = statePPS_PPSS;
    pps_init(&mAnalyzer->GetContext()->mISOParams.PPS);

    delete mNode;
    mNode = new ISO7816NodePPS(sender_reader);
}

bool ISO7816ProtocolPPS::isTransactionComplete(void)
{
    return mStatePPS == statePPS_finished;
}

void ISO7816ProtocolPPS::newData(ISO7816Node* node)
{
    ISO7816NodeChar* charNode = dynamic_cast<ISO7816NodeChar*>(node);
    if(charNode == NULL) throw ISO7816ExceptionExecution("NullPtr cast");
    sender_t sender = mAnalyzer->GetContext()->GetSender();
    pps_t &pps = mAnalyzer->GetContext()->mISOParams.PPS;

    mNode->AddChildNode(node);

    U8 &data = charNode->mCharVal;

    if( (sender != sender_card) && (sender != sender_reader))
        throw ISO7816ExceptionProtocol("Invalid sender state for current context in PPS");

    switch (mStatePPS)
    {
        case statePPS_PPSS:
            if (data != PPS_PPSS_VALUE)
                throw ISO7816ExceptionProtocol("Invalid PPSS command");
            charNode->AddDescription("PPSS");
            pps.PPSS = data;
            break;

        case statePPS_PPS0:
            charNode->AddDescription("PPS0");
            pps.PPS0 = data;
            break;

        case statePPS_PPS1:
            charNode->AddDescription("PPS1");
            pps.PPS1 = data;
            break;

        case statePPS_PPS2:
            charNode->AddDescription("PPS2");
            pps.PPS2 = data;
            break;

        case statePPS_PPS3:
            charNode->AddDescription("PPS3");
            pps.PPS3 = data;
            break;

        case statePPS_PCK:
            charNode->AddDescription("PCK");
            pps.PCK = data;
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
    pps_t &pps = mAnalyzer->GetContext()->mISOParams.PPS;

    switch(mStatePPS)
    {
        case statePPS_PPSS:
            mStatePPS = statePPS_PPS0;
            return;
            // nobreak

        case statePPS_PPS0:
            if(GETBIT(pps.PPS0,PPS_PPS0_PPS1_MASK))
            {
                mStatePPS = statePPS_PPS1;
                return;
            }
            // nobreak

        case statePPS_PPS1:
            if(GETBIT(pps.PPS0,PPS_PPS0_PPS2_MASK))
            {
                mStatePPS = statePPS_PPS2;
                return;
            }
            // nobreak

        case statePPS_PPS2:
            if(GETBIT(pps.PPS0,PPS_PPS0_PPS3_MASK))
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
    pps_t &pps = mAnalyzer->GetContext()->mISOParams.PPS;

    mAnalyzer->GetContext()->mISOParams.default_protocol = GETVAL(pps.PPS0, PPS_PPS0_PROT_MASK,PPS_PPS0_PROT_OFF);

    if(GETBIT(pps.PPS0,PPS_PPS0_PPS1_MASK))
    {
        mAnalyzer->GetContext()->mISOParams.F = GetFn(GETVAL(pps.PPS1, PPS_PPS1_F_MASK, PPS_PPS1_F_OFF));
        mAnalyzer->GetContext()->mISOParams.D = GetDn(GETVAL(pps.PPS1, PPS_PPS1_D_MASK, PPS_PPS1_D_OFF));
    }

    if(GETBIT(pps.PPS0,PPS_PPS0_PPS2_MASK))
    {
        if (pps.PPS2 != 0x00)
        {
            mAnalyzer->GetContext()->mISOParams.SPU.present = true;
        }
        mAnalyzer->GetContext()->mISOParams.SPU.value = pps.PPS2;
    }

    if(GETBIT(pps.PPS0,PPS_PPS0_PPS3_MASK))
    {
        // RFU
    }
}
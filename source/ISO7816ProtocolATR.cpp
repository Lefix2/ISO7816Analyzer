#include "ISO7816ProtocolATR.h"

#include <cstring>

#define ATR_T0_HIST_MASK        0x0F

#define ATR_TA1_F_OFF           4
#define ATR_TA1_F_MASK          0xF0
#define ATR_TA1_D_OFF           0
#define ATR_TA1_D_MASK          0x0F

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
    atr_init(&mAnalyzer->GetContext()->mISOParams.ATR);
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
    char charDesc[5];
    ISO7816NodeChar* charNode = dynamic_cast<ISO7816NodeChar*>(node);
    if(charNode == NULL) throw ISO7816ExceptionExecution("NullPtr cast");
    U8 &data = charNode->mCharVal;
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

    mNode->AddChildNode(node);

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
            charNode->AddDescription("TS");
            atr.TS = data;
            break;

        case stateATR_T0:
            charNode->AddDescription("T0");
            atr.T0 = data;
            mTDi    = data;
            mNb_T   = 0;
            break;

        case stateATR_TA:
            sprintf(charDesc,"TA%d", mNb_T);
            charNode->AddDescription(charDesc);
			atr.T[mNb_T][ATR_INTERFACE_A].present	= true;
			atr.T[mNb_T][ATR_INTERFACE_A].value    = data;
            break;

        case stateATR_TB:
            sprintf(charDesc,"TB%d", mNb_T);
            charNode->AddDescription(charDesc);
			atr.T[mNb_T][ATR_INTERFACE_B].present	= true;
			atr.T[mNb_T][ATR_INTERFACE_B].value    = data;
            break;

        case stateATR_TC:
            sprintf(charDesc,"TC%d", mNb_T);
            charNode->AddDescription(charDesc);
			atr.T[mNb_T][ATR_INTERFACE_C].present	= true;
			atr.T[mNb_T][ATR_INTERFACE_C].value    = data;
            break;

        case stateATR_TD:
            sprintf(charDesc,"TD%d", mNb_T);
            charNode->AddDescription(charDesc);
			atr.T[mNb_T][ATR_INTERFACE_D].present	= true;
			atr.T[mNb_T][ATR_INTERFACE_D].value    = data;
            mNb_T++;
            mTDi = data;
			atr.TCK.present = ((mTDi & 0x0F) != 0x00)?true:false;
            break;

        case stateATR_Historical:
            atr.HB[atr.nb_HB] = data;
            atr.nb_HB++;
            break;

        case stateATR_TCK:
            charNode->AddDescription("TCK");
            atr.TCK.value = data;
            break;

        default:
            throw ISO7816ExceptionProtocol("Invalid ATR state");
            break;
    }
    
    nextTDState();

    if(mStateATR == stateATR_finished)
    {
        // Update transaction parameters
        decodeATR();

        // give node back to analyzer
        mNode->SetStartSample(mNode->GetFirstNode()->GetStartSample());
        mNode->SetEndSample(mNode->GetLastNode()->GetEndSample());
        mAnalyzer->newFrame(mNode);
        mNode = NULL;

        // switch to reader sender
        mAnalyzer->GetContext()->toggleSender();
    }
}

void ISO7816ProtocolATR::nextTDState(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

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
            if(atr.nb_HB < (atr.T0 & ATR_T0_HIST_MASK))
            {
                mStateATR = stateATR_Historical;
                return;
            }

            if(atr.TCK.present)
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

sc_convention_t ISO7816ProtocolATR::GetGlobalConvention(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

    if (atr.TS == 0x3B)
        return convention_direct;
    else if (atr.TS == 0x3F)
        return convention_reverse;

    throw ISO7816ExceptionProtocol("Bad ATR TS");
}

U32 ISO7816ProtocolATR::GetGlobalfMax(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	if (atr.T[0][ATR_INTERFACE_A].present)
		return GetfMax(GETVAL(atr.T[0][ATR_INTERFACE_A].value, ATR_TA1_F_MASK, ATR_TA1_F_OFF));
	else
		return ATR_DEFAULT_FMAX;
}

U32 ISO7816ProtocolATR::GetGlobalFi(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	if (atr.T[0][ATR_INTERFACE_A].present)
		return GetFn(GETVAL(atr.T[0][ATR_INTERFACE_A].value, ATR_TA1_F_MASK, ATR_TA1_F_OFF));
	else
		return ATR_DEFAULT_F;
}

U32 ISO7816ProtocolATR::GetGlobalDi(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	if (atr.T[0][ATR_INTERFACE_A].present)
		return GetFn(GETVAL(atr.T[0][ATR_INTERFACE_A].value, ATR_TA1_D_MASK, ATR_TA1_D_OFF));
	else
		return ATR_DEFAULT_F;
}

U8 ISO7816ProtocolATR::GetGlobalN(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	if (atr.T[0][ATR_INTERFACE_C].present)
		return atr.T[0][ATR_INTERFACE_C].value;
	else
		return ATR_DEFAULT_N;
	
}

U8 ISO7816ProtocolATR::GetGlobalWI(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	itfB_t TC2 = atr.T[1][ATR_INTERFACE_C];

	if (TC2.present){
		if(TC2.value == 0)
			throw ISO7816ExceptionProtocol("Bad ATR TC2");
		return TC2.value;
	}else{
		return ATR_DEFAULT_WI;
	}
}

U8 ISO7816ProtocolATR::GetT1SpecificEDC(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	for (int i = 2; i < ATR_MAX_PROTOCOL; i++){
		/* For TD defining T1 */
		if( (atr.T[i][ATR_INTERFACE_C].present) &&
			((atr.T[i-1][ATR_INTERFACE_D].value & 0x0F) == SC_PROTOCOL_T1))
			return atr.T[i][ATR_INTERFACE_C].value & 1;
	}

	return ATR_DEFAULT_EDC;
}

U8 ISO7816ProtocolATR::GetT1SpecificIFS(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;
    U8 IFS;

	for (int i = 2; i < ATR_MAX_PROTOCOL; i++){
		/* For TD defining T1 */
		if( (atr.T[i][ATR_INTERFACE_A].present) &&
			((atr.T[i-1][ATR_INTERFACE_D].value & 0x0F) == SC_PROTOCOL_T1))
		{
			IFS = atr.T[i][ATR_INTERFACE_A].value;
			if(IFS == 0x00 || IFS == 0xFF)
				throw ISO7816ExceptionProtocol("Bad ATR IFS");
			return IFS;
		}
	}

    return ATR_DEFAULT_IFS;
}

U8 ISO7816ProtocolATR::GetT1SpecificCWI(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;

	for (int i = 2; i < ATR_MAX_PROTOCOL; i++){
		/* For TD defining T1 */
		if( (atr.T[i][ATR_INTERFACE_B].present) &&
			((atr.T[i-1][ATR_INTERFACE_D].value & 0x0F) == SC_PROTOCOL_T1))
			return atr.T[i][ATR_INTERFACE_B].value & 0x0F;
	}

	return ATR_DEFAULT_CWI;
}

U8 ISO7816ProtocolATR::GetT1SpecificBWI(void)
{
    atr_t &atr = mAnalyzer->GetContext()->mISOParams.ATR;
    U8 BWI;

	for (int i = 2; i < ATR_MAX_PROTOCOL; i++){
		/* For TD defining T1 */
		if( (atr.T[i][ATR_INTERFACE_B].present) &&
			((atr.T[i-1][ATR_INTERFACE_D].value & 0x0F) == SC_PROTOCOL_T1))
		{
			BWI = (atr.T[i][ATR_INTERFACE_B].value >> 4) & 0x0F;
			if(BWI > 0x09)
				throw ISO7816ExceptionProtocol("Bad ATR BWI");
			return BWI;
		}
	}

	return ATR_DEFAULT_BWI;
}

void ISO7816ProtocolATR::decodeATR(void)
{
    iso_params_t &params = mAnalyzer->GetContext()->mISOParams;
    atr_t &atr = params.ATR;

    /* Decode protocol */
    bool unset = true;
    uint8_t supported_prot = 0;
    uint8_t prot = SC_PROTOCOL_T0;
    uint8_t tmp_prot = 0;
    uint32_t i;

    for (i = 2; i < ATR_MAX_PROTOCOL; i++)
    {
        if (atr.T[i][ATR_INTERFACE_D].present)
        {
            tmp_prot = atr.T[i][ATR_INTERFACE_D].value & 0x0F;
            supported_prot |= 1 << tmp_prot;
            if (unset == true)
            {
                prot = tmp_prot;
            }
        }
    }

    if (atr.T[1][ATR_INTERFACE_A].present)
    {
        prot = atr.T[1][ATR_INTERFACE_A].value & 0x0F;
        supported_prot |= 1 << prot;
    }

    if (unset)
    {
        prot = SC_PROTOCOL_T0;
        supported_prot |= 1 << prot;
    }

    params.supported_prot = supported_prot;
    params.default_protocol = prot;

    /* Decode global parameters*/
    params.WI = GetGlobalWI();
    params.Fi = GetGlobalFi();
    params.Di = GetGlobalDi();
    params.N = GetGlobalN();

    /* Decode T1 specific parameters*/
    params.EDC = GetT1SpecificEDC();
    params.IFSC = GetT1SpecificIFS();
    params.CWI = GetT1SpecificCWI();
    params.BWI = GetT1SpecificBWI();
}
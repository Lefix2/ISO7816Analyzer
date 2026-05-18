#include "ISO7816ProtocolTPDUT1.h"

#include "ISO7816Defs.h"
#include "ISO7816Exception.h"

#include <cstring>

ISO7816ProtocolTPDUT1::ISO7816ProtocolTPDUT1( ISO7816Analyzer* analyzer ) : ISO7816ProtocolLayer( analyzer ), mNode( NULL )
{
    initTransaction();
}

ISO7816ProtocolTPDUT1::~ISO7816ProtocolTPDUT1()
{
    delete mNode;
}

void ISO7816ProtocolTPDUT1::initTransaction( void )
{
    mState = stateTPDUT1_NAD;
    mPCB = 0;
    mLEN = 0;
    mDataCount = 0;
    mEDCLen = 1;
    mEDCCount = 0;
    mAPDUActive = false;
    mAPDUStartSample = 0;
    mAPDUEndSample = 0;
    mAPDUFirstCount = 0;
    mAPDUTotalLen = 0;
    memset( mAPDUFirst8, 0, sizeof( mAPDUFirst8 ) );

    delete mNode;
    mNode = new ISO7816NodeTPDU();
}

bool ISO7816ProtocolTPDUT1::isTransactionComplete( void )
{
    return false;
}

void ISO7816ProtocolTPDUT1::newData( ISO7816Node* node )
{
    ISO7816NodeChar* charNode = dynamic_cast<ISO7816NodeChar*>( node );
    if( charNode == NULL )
        throw ISO7816ExceptionExecution( "NullPtr cast" );

    mNode->AddChildNode( node );
    nextState( charNode );
}

void ISO7816ProtocolTPDUT1::GetBlockDesc( U8 pcb, char* buf, U32 maxLen )
{
    if( ( pcb & 0x80 ) == 0 )
    {
        // I-block: b8=0, N(S)=b7(bit6), M=b6(bit5)
        U8 ns = ( pcb >> 6 ) & 1;
        U8 m = ( pcb >> 5 ) & 1;
        snprintf( buf, maxLen, "I(NS=%u,M=%u)", ( unsigned )ns, ( unsigned )m );
    }
    else if( ( pcb & 0xC0 ) == 0x80 )
    {
        // R-block: b8=1, b7=0, N(R)=b5(bit4), err=bits1-0
        U8 nr = ( pcb >> 4 ) & 1;
        U8 err = pcb & 0x03;
        if( err == 0 )
            snprintf( buf, maxLen, "R(NR=%u)", ( unsigned )nr );
        else
            snprintf( buf, maxLen, "R(NR=%u,e%u)", ( unsigned )nr, ( unsigned )err );
    }
    else
    {
        // S-block: b8=1, b7=1, resp=b6(bit5), code=bits4-0
        bool resp = ( pcb & PCB_S_RESPONSE ) != 0;
        U8 code = pcb & 0x1F;
        const char* codeStr;
        switch( code )
        {
        case PCB_S_RESYNC:
            codeStr = "RESYNC";
            break;
        case PCB_S_IFS:
            codeStr = "IFS";
            break;
        case PCB_S_ABORT:
            codeStr = "ABORT";
            break;
        case PCB_S_WTX:
            codeStr = "WTX";
            break;
        default:
            codeStr = "?";
            break;
        }
        snprintf( buf, maxLen, "S(%s,%s)", codeStr, resp ? "resp" : "req" );
    }
}

void ISO7816ProtocolTPDUT1::emitAPDU( void )
{
    ISO7816NodeAPDU* apduNode = new ISO7816NodeAPDU( mAPDUStartSample, mAPDUEndSample );
    char desc[ 64 ];

    if( mAPDUTotalLen == 2 )
    {
        char swStr[ 48 ];
        GetSWString( mAPDUFirst8[ 0 ], mAPDUFirst8[ 1 ], swStr, sizeof( swStr ) );
        snprintf( desc, sizeof( desc ), "%s", swStr );
    }
    else if( mAPDUFirstCount >= 2 )
    {
        U8 b0 = mAPDUFirst8[ 0 ];
        bool looksLikeCommand = !( ( b0 >= 0x60 && b0 <= 0x6F ) || b0 == 0x90 );
        if( looksLikeCommand )
            snprintf( desc, sizeof( desc ), "%s", GetINSName( mAPDUFirst8[ 1 ] ) );
        else
            snprintf( desc, sizeof( desc ), "RSP(%uB)", ( unsigned )mAPDUTotalLen );
    }
    else
    {
        snprintf( desc, sizeof( desc ), "APDU" );
    }

    apduNode->AddDescription( desc );
    mAnalyzer->newFrame( apduNode );
    mAPDUActive = false;
}

void ISO7816ProtocolTPDUT1::nextState( ISO7816NodeChar* charNode )
{
    U8 val = charNode->mCharVal;

    switch( mState )
    {
    case stateTPDUT1_NAD:
        charNode->AddDescription( "NAD" );
        mState = stateTPDUT1_PCB;
        break;

    case stateTPDUT1_PCB:
    {
        mPCB = val;
        char desc[ 32 ];
        GetBlockDesc( mPCB, desc, sizeof( desc ) );
        charNode->AddDescription( desc );
        mEDCLen = ( mAnalyzer->GetContext()->mISOParams.EDC == SC_EDC_CRC ) ? 2 : 1;
        mState = stateTPDUT1_LEN;
        break;
    }

    case stateTPDUT1_LEN:
    {
        mLEN = val;
        mDataCount = 0;
        charNode->AddDescription( "LEN" );

        if( ( mPCB & 0x80 ) == 0 ) // I-block: start or continue APDU accumulation
        {
            if( !mAPDUActive )
            {
                mAPDUActive = true;
                mAPDUStartSample = mNode->GetFirstNode()->GetStartSample();
                mAPDUFirstCount = 0;
                mAPDUTotalLen = 0;
            }
            mAPDUTotalLen += mLEN;
        }

        mEDCCount = 0;
        mState = ( mLEN == 0 ) ? stateTPDUT1_EDC : stateTPDUT1_DATA;
        break;
    }

    case stateTPDUT1_DATA:
    {
        char label[ 16 ];
        snprintf( label, sizeof( label ), "INF%u", ( unsigned )mDataCount );
        charNode->AddDescription( label );

        if( ( mPCB & 0x80 ) == 0 && mAPDUActive && mAPDUFirstCount < 8 )
            mAPDUFirst8[ mAPDUFirstCount++ ] = val;

        mDataCount++;
        if( mDataCount >= mLEN )
        {
            mEDCCount = 0;
            mState = stateTPDUT1_EDC;
        }
        break;
    }

    case stateTPDUT1_EDC:
    {
        char label[ 8 ];
        if( mEDCLen == 1 )
            snprintf( label, sizeof( label ), "LRC" );
        else
            snprintf( label, sizeof( label ), "EDC%u", ( unsigned )mEDCCount );
        charNode->AddDescription( label );

        mEDCCount++;
        if( mEDCCount >= mEDCLen )
        {
            // Block complete — set description and emit TPDU frame
            char blockDesc[ 32 ];
            GetBlockDesc( mPCB, blockDesc, sizeof( blockDesc ) );
            mNode->AddDescription( blockDesc );
            mNode->SetStartSample( mNode->GetFirstNode()->GetStartSample() );
            mNode->SetEndSample( mNode->GetLastNode()->GetEndSample() );
            mAnalyzer->newFrame( mNode );

            // Emit APDU when last I-block of chain completes (M=0)
            if( ( mPCB & 0x80 ) == 0 && ( mPCB & 0x20 ) == 0 )
            {
                if( mAPDUActive )
                {
                    mAPDUEndSample = mNode->GetEndSample();
                    emitAPDU();
                }
            }

            // Fresh node for next block (old node owned by mAnalyzer)
            mNode = new ISO7816NodeTPDU();
            mState = stateTPDUT1_NAD;
        }
        break;
    }

    default:
        throw ISO7816ExceptionProtocol( "TPDU T1 bad state" );
    }
}

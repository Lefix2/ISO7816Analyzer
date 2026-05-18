#include "ISO7816ProtocolTPDUT0.h"

#include "ISO7816Defs.h"
#include "ISO7816Exception.h"

ISO7816ProtocolTPDUT0::ISO7816ProtocolTPDUT0( ISO7816Analyzer* analyzer ) : ISO7816ProtocolLayer( analyzer ), mNode( NULL )
{
    initTransaction();
}

ISO7816ProtocolTPDUT0::~ISO7816ProtocolTPDUT0()
{
}

void ISO7816ProtocolTPDUT0::initTransaction( void )
{
    mStateTPDUT0 = stateTPDUT0_HEADER;
    mDataLen = 0;

    delete mNode;
    mNode = new ISO7816NodeTPDU();
}

bool ISO7816ProtocolTPDUT0::isTransactionComplete( void )
{
    return mStateTPDUT0 == stateTPDUT0_finished;
}

void ISO7816ProtocolTPDUT0::newData( ISO7816Node* node )
{
    ISO7816NodeChar* charNode = dynamic_cast<ISO7816NodeChar*>( node );
    if( charNode == NULL )
        throw ISO7816ExceptionExecution( "NullPtr cast" );

    mNode->AddChildNode( node );

    nextState( charNode );

    if( mStateTPDUT0 == stateTPDUT0_finished )
    {
        mNode->SetStartSample( mNode->GetFirstNode()->GetStartSample() );
        mNode->SetEndSample( mNode->GetLastNode()->GetEndSample() );
        mAnalyzer->newFrame( mNode );
        emitAPDU( mNode );
        mNode = NULL;
        initTransaction();
    }
}

void ISO7816ProtocolTPDUT0::emitAPDU( ISO7816NodeTPDU* tpdu )
{
    if( tpdu->GetChildCount() < 7 )
        return;

    ISO7816NodeChar* insNode = dynamic_cast<ISO7816NodeChar*>( tpdu->GetNodeAt( INS_IDX ) );
    ISO7816NodeChar* sw1Node = dynamic_cast<ISO7816NodeChar*>( tpdu->GetNodeAt( -2 ) );
    ISO7816NodeChar* sw2Node = dynamic_cast<ISO7816NodeChar*>( tpdu->GetNodeAt( -1 ) );
    if( !insNode || !sw1Node || !sw2Node )
        return;

    char swStr[ 32 ];
    char desc[ 64 ];
    GetSWString( sw1Node->mCharVal, sw2Node->mCharVal, swStr, sizeof( swStr ) );
    snprintf( desc, sizeof( desc ), "%s %s", GetINSName( insNode->mCharVal ), swStr );

    ISO7816NodeAPDU* apduNode = new ISO7816NodeAPDU( tpdu->GetStartSample(), tpdu->GetEndSample() );
    apduNode->AddDescription( desc );
    mAnalyzer->newFrame( apduNode );
}

void ISO7816ProtocolTPDUT0::nextState( ISO7816NodeChar* charNode )
{
    ISO7816NodeChar* tmpNode;
    static const char* headerDesc[ TPDU_HEADER_SIZE ] = { "CLA", "INS", "P1", "P2", "P3" };

    switch( mStateTPDUT0 )
    {
    case stateTPDUT0_HEADER:
        charNode->AddDescription( headerDesc[ mDataLen ] );
        mDataLen++;
        if( mDataLen == TPDU_HEADER_SIZE )
        {
            mDataLen = 0;
            mAnalyzer->GetContext()->toggleSender();
            mStateTPDUT0 = stateTPDUT0_procedure;
        }
        return;
        // nobreak

    case stateTPDUT0_procedure:
    {
        U8 PROC;
        U8 INS;

        tmpNode = dynamic_cast<ISO7816NodeChar*>( mNode->GetLastNode() );
        if( tmpNode == NULL )
            throw ISO7816ExceptionExecution( "NullPtr cast" );
        PROC = tmpNode->mCharVal;

        tmpNode = dynamic_cast<ISO7816NodeChar*>( mNode->GetNodeAt( INS_IDX ) );
        if( tmpNode == NULL )
            throw ISO7816ExceptionExecution( "NullPtr cast" );
        INS = tmpNode->mCharVal;

        // null byte
        if( PROC == 0x60 )
        {
            charNode->AddDescription( "NULL" );
            return;
        }

        // SW1 byte
        if( ( PROC & 0xF0 ) == 0x60 || ( PROC & 0xF0 ) == 0x90 )
        {
            charNode->AddDescription( "SW1" );
            mStateTPDUT0 = stateTPDUT0_SW2;
            return;
        }

        // ack bytes
        if( PROC == ( INS ) || PROC == ( INS ^ 0x01 ) )
        {
            charNode->AddDescription( "ACK" );
            mStateTPDUT0 = stateTPDUT0_Bytes;
            return;
        }

        // ack byte
        if( PROC == ( INS ^ 0xFF ) || PROC == ( INS ^ 0xFE ) )
        {
            charNode->AddDescription( "ACK" );
            mStateTPDUT0 = stateTPDUT0_Byte;
            return;
        }

        throw ISO7816ExceptionProtocol( "Bad procedure byte" );
        // nobreak
    }

    case stateTPDUT0_SW2:
        charNode->AddDescription( "SW2" );
        mAnalyzer->GetContext()->toggleSender();
        mStateTPDUT0 = stateTPDUT0_finished;
        return;
        // nobreak

    case stateTPDUT0_Bytes:
    {
        U8 P3;
        U16 len;

        tmpNode = dynamic_cast<ISO7816NodeChar*>( mNode->GetNodeAt( P3_IDX ) );
        if( tmpNode == NULL )
            throw ISO7816ExceptionExecution( "NullPtr cast" );
        P3 = tmpNode->mCharVal;

        mDataLen++;
        if( mAnalyzer->GetContext()->GetSender() == sender_card )
        {
            len = P3 == 0 ? 256 : P3;
        }
        else
        {
            len = P3;
        }
        if( len == mDataLen )
            mStateTPDUT0 = stateTPDUT0_procedure;
        return;
        // nobreak
    }

    case stateTPDUT0_Byte:
    {
        mDataLen++;
        mStateTPDUT0 = stateTPDUT0_procedure;
        return;
        // nobreak
    }

    case stateTPDUT0_finished:
        // nobreak

    default:
        throw ISO7816ExceptionProtocol( "TPDU T0 bad state" );
        // nobreak
    }
}
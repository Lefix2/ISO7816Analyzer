#include "ISO7816Node.h"

#include "ISO7816Defs.h"
#include "ISO7816Exception.h"
#include <AnalyzerHelpers.h>
#include <cstring>

/*
 * ISO7816 Node abstract class implementation
 */
ISO7816Node::ISO7816Node( nodeLevel_t nodeLevel, sender_t sender, S64 startSample, S64 endSample, U64 nodeId )
    : mSender( sender ), mNodeLevel( nodeLevel ), mStartSample( startSample ), mEndSample( endSample ), mNodeId( nodeId ), mDescription()
{
}

ISO7816Node::~ISO7816Node()
{
}

void ISO7816Node::SetNodeId( U64 nodeId )
{
    mNodeId = nodeId;
}

U64 ISO7816Node::GetNodeId( void )
{
    return mNodeId;
}

void ISO7816Node::SetStartSample( S64 startSample )
{
    mStartSample = startSample;
}

S64 ISO7816Node::GetStartSample( void )
{
    return mStartSample;
}

void ISO7816Node::SetEndSample( S64 endSample )
{
    mEndSample = endSample;
}

S64 ISO7816Node::GetEndSample( void )
{
    return mEndSample;
}

sender_t ISO7816Node::GetSender( void )
{
    return mSender;
}

nodeLevel_t ISO7816Node::GetLevel( void )
{
    return mNodeLevel;
}

void ISO7816Node::GetShortStr( char* buf, U32 maxLen )
{
    GetDataStr( buf, maxLen );
}

ISO7816Node* ISO7816Node::GetNodeAt( S64 index )
{
    if( index >= 0 )
        return mChilds.at( index );
    else if( -index < ( S64 )mChilds.size() )
        return mChilds.at( ( S64 )mChilds.size() + index );

    throw ISO7816ExceptionExecution( "Bad index" );
}

#ifdef LOGIC2
const char* ISO7816Node::GetFrameV2Type( void )
{
    const char* types[] = {
        "char", "tpdu", "apdu", "pps", "atr",
    };

    if( mNodeLevel < nodeLevel_count_or_invalid )
    {
        return types[ mNodeLevel ];
    }
    return "data";
}
#endif

ISO7816Node* ISO7816Node::GetFirstNode( void )
{
    return mChilds.front();
}

ISO7816Node* ISO7816Node::GetLastNode( void )
{
    return mChilds.back();
}

void ISO7816Node::AddChildNode( ISO7816Node* child )
{
    mChilds.push_back( child );
}

void ISO7816Node::AddDescription( const char* str )
{
    mDescription += str;
}

size_t ISO7816Node::GetChildCount( void ) const
{
    return mChilds.size();
}


/*
 * ISO7816 Node for APDU class definition
 */
ISO7816NodeAPDU::ISO7816NodeAPDU( S64 startSample, S64 endSample, U64 nodeId )
    : ISO7816Node( nodeLevel_apdu, sender_undefined, startSample, endSample, nodeId )
{
}

ISO7816NodeAPDU::~ISO7816NodeAPDU()
{
}

void ISO7816NodeAPDU::GetDataStr( char* resultString, U32 maxStrLen )
{
    if( !mDescription.empty() )
        snprintf( resultString, maxStrLen, "%s", mDescription.c_str() );
    else
        snprintf( resultString, maxStrLen, "APDU" );
}

void ISO7816NodeAPDU::GetShortStr( char* buf, U32 maxLen )
{
    snprintf( buf, maxLen, "APDU" );
}


/*
 * ISO7816 Node for TPDU class definition
 */
ISO7816NodeTPDU::ISO7816NodeTPDU( S64 startSample, S64 endSample, U64 nodeId )
    : ISO7816Node( nodeLevel_tpdu, sender_undefined, startSample, endSample, nodeId )
{
}

ISO7816NodeTPDU::~ISO7816NodeTPDU()
{
}

void ISO7816NodeTPDU::GetDataStr( char* resultString, U32 maxStrLen )
{
    if( !mDescription.empty() )
    {
        snprintf( resultString, maxStrLen, "%s", mDescription.c_str() );
        return;
    }
    // T=0: decode INS + SW from children (header[1]=INS, last-2=SW1, last-1=SW2)
    if( mChilds.size() >= 7 )
    {
        ISO7816NodeChar* insNode = dynamic_cast<ISO7816NodeChar*>( GetNodeAt( INS_IDX ) );
        ISO7816NodeChar* sw1Node = dynamic_cast<ISO7816NodeChar*>( GetNodeAt( -2 ) );
        ISO7816NodeChar* sw2Node = dynamic_cast<ISO7816NodeChar*>( GetNodeAt( -1 ) );
        if( insNode && sw1Node && sw2Node )
        {
            char swStr[ 32 ];
            GetSWString( sw1Node->mCharVal, sw2Node->mCharVal, swStr, sizeof( swStr ) );
            snprintf( resultString, maxStrLen, "%s %s", GetINSName( insNode->mCharVal ), swStr );
            return;
        }
    }
    snprintf( resultString, maxStrLen, "TPDU" );
}

void ISO7816NodeTPDU::GetShortStr( char* buf, U32 maxLen )
{
    snprintf( buf, maxLen, "TPDU" );
}


/*
 * ISO7816 Node for PPS class implementation
 */
ISO7816NodePPS::ISO7816NodePPS( sender_t sender, S64 startSample, S64 endSample, U64 nodeId )
    : ISO7816Node( nodeLevel_pps, sender, startSample, endSample, nodeId )
{
}

ISO7816NodePPS::~ISO7816NodePPS()
{
}

void ISO7816NodePPS::GetDataStr( char* resultString, U32 maxStrLen )
{
    if( !mDescription.empty() )
        snprintf( resultString, maxStrLen, "%s", mDescription.c_str() );
    else
        snprintf( resultString, maxStrLen, "PPS" );
}

void ISO7816NodePPS::GetShortStr( char* buf, U32 maxLen )
{
    snprintf( buf, maxLen, "PPS" );
}


/*
 * ISO7816 Node for ATR class implementation
 */
ISO7816NodeATR::ISO7816NodeATR( S64 startSample, S64 endSample, U64 nodeId )
    : ISO7816Node( nodeLevel_atr, sender_card, startSample, endSample, nodeId )
{
}

ISO7816NodeATR::~ISO7816NodeATR()
{
}

void ISO7816NodeATR::GetDataStr( char* resultString, U32 maxStrLen )
{
    if( !mDescription.empty() )
        snprintf( resultString, maxStrLen, "%s", mDescription.c_str() );
    else
        snprintf( resultString, maxStrLen, "ATR" );
}

void ISO7816NodeATR::GetShortStr( char* buf, U32 maxLen )
{
    snprintf( buf, maxLen, "ATR" );
}


/*
 * ISO7816 Node for character class implementation
 */
ISO7816NodeChar::ISO7816NodeChar( sender_t sender, U8 charVal, S64 startSample, S64 endSample, U64 nodeId )
    : ISO7816Node( nodeLevel_char, sender, startSample, endSample, nodeId ), mCharVal( charVal )
{
}

ISO7816NodeChar::~ISO7816NodeChar()
{
}

static const char* DescPrefix( const char* desc, char* tmp, size_t tmpLen )
{
    const char* paren = strchr( desc, '(' );
    if( paren )
    {
        size_t n = ( size_t )( paren - desc );
        if( n >= tmpLen ) n = tmpLen - 1;
        memcpy( tmp, desc, n );
        tmp[ n ] = '\0';
        return tmp;
    }
    return desc;
}

void ISO7816NodeChar::GetDataStr( char* resultString, U32 maxStrLen )
{
    if( mDescription.empty() )
    {
        snprintf( resultString, maxStrLen, "0x%02X", mCharVal );
        return;
    }
    const char* desc = mDescription.c_str();
    const char* paren = strchr( desc, '(' );
    if( paren )
    {
        const char* close = strrchr( desc, ')' );
        size_t prefixLen = ( size_t )( paren - desc );
        if( close && close > paren )
            snprintf( resultString, maxStrLen, "%.*s(0x%02X) %.*s",
                      ( int )prefixLen, desc, mCharVal,
                      ( int )( close - paren - 1 ), paren + 1 );
        else
            snprintf( resultString, maxStrLen, "%.*s(0x%02X)", ( int )prefixLen, desc, mCharVal );
    }
    else
    {
        snprintf( resultString, maxStrLen, "%s(0x%02X)", desc, mCharVal );
    }
}

void ISO7816NodeChar::GetShortStr( char* buf, U32 maxLen )
{
    if( mDescription.empty() )
    {
        snprintf( buf, maxLen, "0x%02X", mCharVal );
        return;
    }
    char tmp[ 64 ];
    snprintf( buf, maxLen, "%s", DescPrefix( mDescription.c_str(), tmp, sizeof( tmp ) ) );
}

void ISO7816NodeChar::GetMedStr( char* buf, U32 maxLen, DisplayBase displayBase )
{
    char valStr[ 32 ];
    AnalyzerHelpers::GetNumberString( mCharVal, displayBase, 8, valStr, sizeof( valStr ) );
    if( mDescription.empty() )
    {
        snprintf( buf, maxLen, "%s", valStr );
        return;
    }
    char tmp[ 64 ];
    const char* prefix = DescPrefix( mDescription.c_str(), tmp, sizeof( tmp ) );
    snprintf( buf, maxLen, "%s(%s)", prefix, valStr );
}
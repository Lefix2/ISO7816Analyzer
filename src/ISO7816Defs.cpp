#include "ISO7816Defs.h"

#include <cstdio>
#include <cstring>

/* nb_Tx[Yi] table, number of interface bytes for a given Y */
static const U8 nb_Tx_table[ 16 ] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

/* fmax table, indicated fmax for given TA1[8-5] */
static const U32 fmax_table[ 16 ] = { 4000000, 5000000, 6000,    8000000,  12000000, 16000000, 20000000, 0,
                                      0,       5000000, 7500000, 10000000, 15000000, 20000000, 0,        0 };

/* Fi table, indicated F for given TA1[8-5] */
static const U16 f_table[ 16 ] = { 372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048, 0, 0 };

/* Di table, indicated D for given TA1[4-1] */
static const U16 d_table[ 16 ] = { 0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 0, 0, 0, 0, 0, 0 };

static const U16 i_table[ 4 ] = { 25, 50, 100, 0 };

void atr_init( atr_t* atr )
{
    memset( atr, 0, sizeof( atr_t ) );
}

void pps_init( pps_t* pps )
{
    memset( pps, 0, sizeof( pps_t ) );
}

void iso_params_init( iso_params_t* params )
{
    params->state = sc_state_power_off;

    params->frequency = 4000000;
    params->convention = convention_direct;
    params->supported_prot = 0;
    params->default_protocol = 0;

    atr_init( &( params->ATR ) );
    pps_init( &( params->PPS ) );

    params->F = ATR_DEFAULT_F;
    params->D = ATR_DEFAULT_D;
    params->N = ATR_DEFAULT_N;
    params->WI = ATR_DEFAULT_WI;

    params->Nd = 0;
    params->Nc = 0;
    params->DAD = ATR_DEFAULT_DAD;
    params->SAD = ATR_DEFAULT_SAD;
    params->WTX = 0;
    params->IFSC = ATR_DEFAULT_IFS;
    params->IFSD = ATR_DEFAULT_IFS;
    params->BWI = ATR_DEFAULT_BWI;
    params->CWI = ATR_DEFAULT_CWI;
    params->EDC = ATR_DEFAULT_EDC;
    params->SPU.present = false;
}

U32 GetfMax( U8 F )
{
    return fmax_table[ F ];
}

U16 GetFn( U8 F )
{
    return f_table[ F ];
}

U16 GetDn( U8 D )
{
    return d_table[ D ];
}

const char* GetINSName( U8 ins )
{
    switch( ins & 0xFE )
    {
    case 0x04:
        return "DEACTIVATE FILE";
    case 0x0E:
        return "ERASE BINARY";
    case 0x20:
        return "VERIFY";
    case 0x22:
        return "MANAGE SE";
    case 0x24:
        return "CHANGE REF DATA";
    case 0x26:
        return "DISABLE VERIF";
    case 0x28:
        return "ENABLE VERIF";
    case 0x2A:
        return "PERFORM SEC OP";
    case 0x2C:
        return "RESET RETRY";
    case 0x44:
        return "ACTIVATE FILE";
    case 0x46:
        return "GEN KEYPAIR";
    case 0x70:
        return "MANAGE CHANNEL";
    case 0x82:
        return "EXT AUTH";
    case 0x84:
        return "GET CHALLENGE";
    case 0x86:
        return "GEN AUTH";
    case 0x88:
        return "INT AUTH";
    case 0xA0:
        return "SEARCH BINARY";
    case 0xA2:
        return "SEARCH RECORD";
    case 0xA4:
        return "SELECT";
    case 0xB0:
        return "READ BINARY";
    case 0xB2:
        return "READ RECORD";
    case 0xC0:
        return "GET RESPONSE";
    case 0xC2:
        return "ENVELOPE";
    case 0xCA:
        return "GET DATA";
    case 0xD0:
        return "WRITE BINARY";
    case 0xD2:
        return "WRITE RECORD";
    case 0xD6:
        return "UPDATE BINARY";
    case 0xDA:
        return "PUT DATA";
    case 0xDC:
        return "UPDATE RECORD";
    case 0xE0:
        return "CREATE FILE";
    case 0xE2:
        return "APPEND RECORD";
    case 0xE4:
        return "DELETE FILE";
    case 0xF2:
        return "STATUS";
    case 0xFE:
        return "TERMINATE CARD";
    default:
        return "INS";
    }
}

void GetSWString( U8 sw1, U8 sw2, char* buf, U32 maxLen )
{
    if( sw1 == 0x90 && sw2 == 0x00 )
    {
        snprintf( buf, maxLen, "OK" );
        return;
    }
    if( sw1 == 0x67 && sw2 == 0x00 )
    {
        snprintf( buf, maxLen, "Wrong length" );
        return;
    }
    if( sw1 == 0x6B && sw2 == 0x00 )
    {
        snprintf( buf, maxLen, "Wrong P1-P2" );
        return;
    }
    if( sw1 == 0x6D && sw2 == 0x00 )
    {
        snprintf( buf, maxLen, "INS not supported" );
        return;
    }
    if( sw1 == 0x6E && sw2 == 0x00 )
    {
        snprintf( buf, maxLen, "CLA not supported" );
        return;
    }
    if( sw1 == 0x6F && sw2 == 0x00 )
    {
        snprintf( buf, maxLen, "Unknown error" );
        return;
    }
    if( sw1 == 0x61 )
    {
        snprintf( buf, maxLen, "+%uB avail", ( unsigned )sw2 );
        return;
    }
    if( sw1 == 0x6C )
    {
        snprintf( buf, maxLen, "Wrong Le=%u", ( unsigned )sw2 );
        return;
    }

    if( sw1 == 0x62 )
    {
        switch( sw2 )
        {
        case 0x00:
            snprintf( buf, maxLen, "Warn:no info" );
            return;
        case 0x81:
            snprintf( buf, maxLen, "Warn:corrupted" );
            return;
        case 0x82:
            snprintf( buf, maxLen, "Warn:EOF" );
            return;
        case 0x83:
            snprintf( buf, maxLen, "Warn:deactivated" );
            return;
        case 0x84:
            snprintf( buf, maxLen, "Warn:FCI format" );
            return;
        default:
            snprintf( buf, maxLen, "Warn:62%02X", sw2 );
            return;
        }
    }

    if( sw1 == 0x63 )
    {
        if( ( sw2 & 0xF0 ) == 0xC0 )
        {
            snprintf( buf, maxLen, "PIN fail(%u left)", ( unsigned )( sw2 & 0x0F ) );
            return;
        }
        snprintf( buf, maxLen, "Warn:63%02X", sw2 );
        return;
    }

    if( sw1 == 0x64 )
    {
        if( sw2 == 0x00 )
        {
            snprintf( buf, maxLen, "Exec err:no chg" );
            return;
        }
        snprintf( buf, maxLen, "Exec err:64%02X", sw2 );
        return;
    }

    if( sw1 == 0x65 )
    {
        if( sw2 == 0x81 )
        {
            snprintf( buf, maxLen, "Memory fail" );
            return;
        }
        snprintf( buf, maxLen, "Exec err:65%02X", sw2 );
        return;
    }

    if( sw1 == 0x68 )
    {
        switch( sw2 )
        {
        case 0x81:
            snprintf( buf, maxLen, "No log chan" );
            return;
        case 0x82:
            snprintf( buf, maxLen, "No sec msg" );
            return;
        default:
            snprintf( buf, maxLen, "CLA:68%02X", sw2 );
            return;
        }
    }

    if( sw1 == 0x69 )
    {
        switch( sw2 )
        {
        case 0x81:
            snprintf( buf, maxLen, "Incompat file" );
            return;
        case 0x82:
            snprintf( buf, maxLen, "Security status" );
            return;
        case 0x83:
            snprintf( buf, maxLen, "Auth blocked" );
            return;
        case 0x84:
            snprintf( buf, maxLen, "Ref invalid" );
            return;
        case 0x85:
            snprintf( buf, maxLen, "No current EF" );
            return;
        case 0x86:
            snprintf( buf, maxLen, "No cmd expected" );
            return;
        case 0x87:
            snprintf( buf, maxLen, "SecMsg:no Lc" );
            return;
        case 0x88:
            snprintf( buf, maxLen, "SecMsg:incorrect" );
            return;
        default:
            snprintf( buf, maxLen, "Cmd:69%02X", sw2 );
            return;
        }
    }

    if( sw1 == 0x6A )
    {
        switch( sw2 )
        {
        case 0x80:
            snprintf( buf, maxLen, "Bad data" );
            return;
        case 0x81:
            snprintf( buf, maxLen, "Func not supp" );
            return;
        case 0x82:
            snprintf( buf, maxLen, "File not found" );
            return;
        case 0x83:
            snprintf( buf, maxLen, "Record not found" );
            return;
        case 0x84:
            snprintf( buf, maxLen, "Not enough space" );
            return;
        case 0x85:
            snprintf( buf, maxLen, "Lc/TLV mismatch" );
            return;
        case 0x86:
            snprintf( buf, maxLen, "Wrong P1-P2" );
            return;
        case 0x87:
            snprintf( buf, maxLen, "Lc/P3 mismatch" );
            return;
        case 0x88:
            snprintf( buf, maxLen, "Ref not found" );
            return;
        default:
            snprintf( buf, maxLen, "Param:6A%02X", sw2 );
            return;
        }
    }

    snprintf( buf, maxLen, "SW=%02X%02X", sw1, sw2 );
}
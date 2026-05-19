#include "ISO7816AnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "ISO7816Analyzer.h"
#include "ISO7816AnalyzerSettings.h"
#include "ISO7816Node.h"
#include <iostream>
#include <fstream>

ISO7816AnalyzerResults::ISO7816AnalyzerResults( ISO7816Analyzer* analyzer, ISO7816AnalyzerSettings* settings )
    : AnalyzerResults(), mSettings( settings ), mAnalyzer( analyzer )
{
}

ISO7816AnalyzerResults::~ISO7816AnalyzerResults()
{
}

void ISO7816AnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
    ClearResultStrings();
    ISO7816Node* node = mAnalyzer->GetNodeByFrameId( frame_index );

    if( channel == mSettings->mChannelVCC )
    {
        // none
    }
    else if( channel == mSettings->mChannelRST )
    {
        if( node->GetLevel() == nodeLevel_apdu )
        {
            char longStr[ 256 ], shortStr[ 16 ];
            node->GetDataStr( longStr, sizeof( longStr ) );
            node->GetShortStr( shortStr, sizeof( shortStr ) );
            AddResultString( longStr );
            AddResultString( shortStr );
        }
    }
    else if( channel == mSettings->mChannelCLK )
    {
        if( ( node->GetLevel() == nodeLevel_atr ) || ( node->GetLevel() == nodeLevel_pps ) || ( node->GetLevel() == nodeLevel_tpdu ) )
        {
            char longStr[ 256 ], shortStr[ 16 ];
            node->GetDataStr( longStr, sizeof( longStr ) );
            node->GetShortStr( shortStr, sizeof( shortStr ) );
            AddResultString( longStr );
            AddResultString( shortStr );
        }
    }
    else if( channel == mSettings->mChannelIO )
    {
        if( node->GetLevel() == nodeLevel_char )
        {
            ISO7816NodeChar* charNode = dynamic_cast<ISO7816NodeChar*>( node );
            if( charNode )
            {
                char longStr[ 256 ], medStr[ 128 ], shortStr[ 64 ];
                charNode->GetDataStr( longStr, sizeof( longStr ) );
                charNode->GetMedStr( medStr, sizeof( medStr ), display_base );
                charNode->GetShortStr( shortStr, sizeof( shortStr ) );
                AddResultString( longStr );
                AddResultString( medStr );
                AddResultString( shortStr );
            }
        }
    }
}

void ISO7816AnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
    ( void )export_type_user_id;

    std::ofstream file_stream( file, std::ios::out );

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    file_stream << "Time [s],Value" << std::endl;

    U64 num_frames = GetNumFrames();
    for( U32 i = 0; i < num_frames; i++ )
    {
        Frame frame = GetFrame( i );

        char time_str[ 128 ];
        AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

        char number_str[ 128 ];
        AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

        file_stream << time_str << "," << number_str << std::endl;

        if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
        {
            file_stream.close();
            return;
        }
    }

    file_stream.close();
}

void ISO7816AnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
    char dataStr[ 64 ];
    ISO7816Node* node = mAnalyzer->GetNodeByFrameId( frame_index );

    node->GetDataStr( dataStr, sizeof( dataStr ) );

    Frame frame = GetFrame( frame_index );
    ClearTabularText();

    // @TODO for now only display char
    if( node->GetLevel() == nodeLevel_char )
        AddTabularText( dataStr );
#else
    ( void )frame_index;
    ( void )DisplayBase;
#endif
}

void ISO7816AnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
    ( void )packet_id;
    ( void )display_base;
    // not supported
}

void ISO7816AnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
    ( void )transaction_id;
    ( void )display_base;
    // not supported
}
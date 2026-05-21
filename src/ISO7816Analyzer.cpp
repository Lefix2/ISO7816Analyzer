#include "ISO7816Analyzer.h"
#include "ISO7816AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

#include "ISO7816ProtocolChar.h"
#include "ISO7816Exception.h"


#include <iostream>

static const bool parity[ 256 ] = {
#define P2( n ) n, n ^ 1, n ^ 1, n
#define P4( n ) P2( n ), P2( n ^ 1 ), P2( n ^ 1 ), P2( n )
#define P6( n ) P4( n ), P4( n ^ 1 ), P4( n ^ 1 ), P4( n )
    P6( 0 ), P6( 1 ), P6( 1 ), P6( 0 )
};

void ISO7816Analyzer::newFrame( ISO7816Node* node )
{
#ifdef LOGIC2
    // In Logic2 LLA mode only char-level frames are visible; the Python HLA handles higher levels.
    if( node->GetLevel() != nodeLevel_char )
    {
        delete node;
        return;
    }
#endif

    // Add node
    mNodes.push_back( node );

    // we have a byte to save.
    Frame frame;
    frame.mStartingSampleInclusive = node->GetStartSample();
    frame.mEndingSampleInclusive = node->GetEndSample();

    node->SetNodeId( mResults->AddFrame( frame ) );

#ifdef LOGIC2
    FrameV2 framev2;
    ISO7816NodeChar* cn = dynamic_cast<ISO7816NodeChar*>( node );
    if( cn )
    {
        char label[ 64 ];
        cn->GetShortStr( label, sizeof( label ) );
        framev2.AddByte( "data", cn->mCharVal );
        framev2.AddString( "description", label );
        framev2.AddString( "from", cn->GetSender() == sender_card ? "card" : "reader" );
        const char* phaseStr;
        switch( mContext->mState )
        {
        case S_ATR:    phaseStr = "atr"; break;
        case S_PPS:    phaseStr = "pps"; break;
        case S_T0:     phaseStr = "t0";  break;
        case S_T1:     phaseStr = "t1";  break;
        default:       phaseStr = "unknown"; break;
        }
        framev2.AddString( "phase", phaseStr );
    }
    mResults->AddFrameV2( framev2, node->GetFrameV2Type(), node->GetStartSample(), node->GetEndSample() );
#endif

    mResults->CommitResults();
}

ISO7816Context* ISO7816Analyzer::GetContext( void )
{
    return mContext;
}

ISO7816Node* ISO7816Analyzer::GetNodeByFrameId( U64 frameId )
{
    for( auto const& node : mNodes )
        if( node->GetNodeId() == frameId )
            return node;
    return NULL;
}

ISO7816Analyzer::ISO7816Analyzer()
    : Analyzer2(), mSettings( new ISO7816AnalyzerSettings() ), mSimulationInitialized( false ), mContext( new ISO7816Context() )
{
    SetAnalyzerSettings( mSettings.get() );
}

ISO7816Analyzer::~ISO7816Analyzer()
{
    KillThread();
}

void ISO7816Analyzer::Setup()
{
    mSampleRateHz = GetSampleRate();
    mNodes.clear();

    if( mSettings->mChannelVCC != UNDEFINED_CHANNEL )
        mVCC = GetAnalyzerChannelData( mSettings->mChannelVCC );
    else
        mVCC = NULL;
    mRST = GetAnalyzerChannelData( mSettings->mChannelRST );
    mCLK = GetAnalyzerChannelData( mSettings->mChannelCLK );
    mIO = GetAnalyzerChannelData( mSettings->mChannelIO );
}

void ISO7816Analyzer::SyncToSample( U64 to_sample )
{
    if( mVCC != NULL )
        mVCC->AdvanceToAbsPosition( to_sample );
    mRST->AdvanceToAbsPosition( to_sample );
    mCLK->AdvanceToAbsPosition( to_sample );
    mIO->AdvanceToAbsPosition( to_sample );
}

void ISO7816Analyzer::AdvanceEtu( double etu )
{
    U64 nbEdges = ( ( ( double )( mContext->mISOParams.F ) / ( double )( mContext->mISOParams.D ) ) * etu ) * 2;

    for( U64 i = 0; i < nbEdges; i++ )
    {
        mCLK->AdvanceToNextEdge();
    }

    SyncToSample( mCLK->GetSampleNumber() );
}

void ISO7816Analyzer::SetupResults()
{
    mResults.reset( new ISO7816AnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mChannelIO );
#ifndef LOGIC2
    mResults->AddChannelBubblesWillAppearOn( mSettings->mChannelCLK );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mChannelRST );
#endif

#ifdef LOGIC2
    UseFrameV2();
#endif
}

void ISO7816Analyzer::WorkerThread()
{
    U64 data = 0;
    DataBuilder word;
    ISO7816ProtocolChar* protocolChar;

    protocolChar = new ISO7816ProtocolChar( this );

    Setup();

    for( ;; )
    {
        protocolChar->initTransaction();

        // Find first RST rising edge
        SyncToSample( mRST->GetSampleOfNextEdge() );
        if( mRST->GetBitState() == BIT_LOW )
            SyncToSample( mRST->GetSampleOfNextEdge() );

        // Mark reset
        mResults->AddMarker( mRST->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mChannelRST );

        // Decode char until reset
        while( 1 )
        {
            word.Reset( &data, mContext->mISOParams.convention == convention_direct ? AnalyzerEnums::LsbFirst : AnalyzerEnums::MsbFirst,
                        8 );

            // Check that a reset don't happen
            if( mRST->WouldAdvancingToAbsPositionCauseTransition( mIO->GetSampleOfNextEdge() ) )
                break;

            // Start bit
            while( mIO->GetBitState() != BIT_LOW )
                SyncToSample( mIO->GetSampleOfNextEdge() );
            U64 starting_sample = mIO->GetSampleNumber();

            // Shift to sample between edges
            AdvanceEtu( 0.5 );
            // Mark start bit (start dot)
            mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Start, mSettings->mChannelIO );

            // 8 data bit
            for( U32 i = 0; i < 8; i++ )
            {
                AdvanceEtu();

                // Mark data (dot)
                mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mChannelIO );

                word.AddBit( mContext->mISOParams.convention == convention_direct ? mIO->GetBitState()
                             : mIO->GetBitState() == BIT_HIGH                     ? BIT_LOW
                                                                                  : BIT_HIGH );
            }

            // Parity bit
            AdvanceEtu();
            if( ( mIO->GetBitState() == BIT_HIGH ? true : false ) != parity[ data ] )
                mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mChannelIO );
            else
                mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Square, mSettings->mChannelIO );

            // Stop bit
            AdvanceEtu();
            if( mIO->GetBitState() == BIT_LOW )
                mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::ErrorSquare, mSettings->mChannelIO );
            else
                mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mChannelIO );

            try
            {
                // Todo add errors flags to data
                protocolChar->newData( new ISO7816NodeChar( mContext->GetSender(), data, starting_sample, mIO->GetSampleNumber() ) );
            }
            catch( ISO7816ExceptionProtocol e )
            {
                std::cout << "Exception catched while building protocol : " << e.GetDetails() << std::endl;
                continue;
            }
            catch( ISO7816ExceptionExecution e )
            {
                std::cout << "Exception catched : " << e.GetDetails() << std::endl;
                exit( 1 );
            }

            ReportProgress( mIO->GetSampleNumber() );
        }
        std::cout << "End of ISO transmission" << std::endl;
    }
}

bool ISO7816Analyzer::NeedsRerun()
{
    return false;
}

U32 ISO7816Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                             SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitialized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitialized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 ISO7816Analyzer::GetMinimumSampleRateHz()
{
    return 4000000 * 2;
}

const char* ISO7816Analyzer::GetAnalyzerName() const
{
    return "ISO/IEC-7816";
}

const char* GetAnalyzerName()
{
    return "ISO/IEC-7816";
}

Analyzer* CreateAnalyzer()
{
    return new ISO7816Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}
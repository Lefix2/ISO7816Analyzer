#include "ISO7816Analyzer.h"
#include "ISO7816AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

#include <iostream>

ISO7816Analyzer::ISO7816Analyzer()
:	Analyzer2(),  
	mSettings( new ISO7816AnalyzerSettings() ),
	mSimulationInitialized( false )
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

	mVCC = GetAnalyzerChannelData( mSettings->mChannelVCC);
	mRST = GetAnalyzerChannelData( mSettings->mChannelRST); 
	mCLK = GetAnalyzerChannelData( mSettings->mChannelCLK);
	mIO = GetAnalyzerChannelData( mSettings->mChannelIO );

	mF = 372;
	mD = 1;
}

void ISO7816Analyzer::SyncToSample(U64 to_sample)
{
	if (mVCC->GetSampleNumber() < to_sample)
		mVCC->AdvanceToAbsPosition(to_sample);
	if (mRST->GetSampleNumber() < to_sample)
		mRST->AdvanceToAbsPosition(to_sample);
	if (mCLK->GetSampleNumber() < to_sample)
		mCLK->AdvanceToAbsPosition(to_sample);
	if (mIO->GetSampleNumber() < to_sample)
		mIO->AdvanceToAbsPosition(to_sample);
}

void ISO7816Analyzer::AdvanceEtu(double etu)
{
	U64 nbEdges = (((double)(mF)/(double)(mD))*etu)*2;

	for (U64 i = 0; i < nbEdges; i++)
	{
		mCLK->AdvanceToNextEdge();
	}

	SyncToSample(mCLK->GetSampleNumber());
}

void ISO7816Analyzer::SetupResults()
{
	mResults.reset( new ISO7816AnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mChannelIO );
}

void ISO7816Analyzer::WorkerThread()
{

	Setup();

	// Find first RST rising edge
	SyncToSample(mRST->GetSampleOfNextEdge());
	if(mRST->GetBitState() == BIT_LOW)
		mRST->AdvanceToNextEdge();

	// Mark reset
	mResults->AddMarker(mRST->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mChannelRST);

	for( ; ; )
	{
		U8 data = 0;
		
		// Start bit
		SyncToSample(mIO->GetSampleOfNextEdge());
		U64 starting_sample = mIO->GetSampleNumber();

		// sample between edges
		AdvanceEtu(0.5);
		mResults->AddMarker(mIO->GetSampleNumber(), AnalyzerResults::Start, mSettings->mChannelIO);


		for( U32 i=0; i<8; i++ )
		{
			data >>= 1;

			AdvanceEtu();

			//let's put a dot exactly where we sample this bit:
			mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mChannelIO );

			if( mIO->GetBitState() == BIT_HIGH )
				data |= 0x80;
		}

		// Parity bit
		AdvanceEtu();
		if(mIO->GetBitState() != (data & 0x01))
			mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mChannelIO );
		else
			mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mChannelIO );

		// Stop bit
		AdvanceEtu();
		if(mIO->GetBitState() == BIT_LOW)
			mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::ErrorSquare, mSettings->mChannelIO );
		else
			mResults->AddMarker( mIO->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mChannelIO );


		//we have a byte to save. 
		Frame frame;
		frame.mData1 = data;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = starting_sample;
		frame.mEndingSampleInclusive = mIO->GetSampleNumber();

		mResults->AddFrame( frame );
		mResults->CommitResults();
		ReportProgress( frame.mEndingSampleInclusive );
	}
}

bool ISO7816Analyzer::NeedsRerun()
{
	return false;
}

U32 ISO7816Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
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
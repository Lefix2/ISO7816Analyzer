#include "ISO7816SimulationDataGenerator.h"
#include "ISO7816AnalyzerSettings.h"

#include <AnalyzerHelpers.h>

const U8 cATR[] = {0x3B, 0xFF, 0x96, 0x00, 0xFF, 0x80, 0x31, 0xFE, 0x45, 0x00, 0xB8, 0x54, 0x34, 0x06, 0x0E, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC};
const U8 cPPS[] = {0xFF, 0x40, 0x01, 0xBE};

ISO7816SimulationDataGenerator::ISO7816SimulationDataGenerator()
{
}

ISO7816SimulationDataGenerator::~ISO7816SimulationDataGenerator()
{
}

void ISO7816SimulationDataGenerator::Initialize( U32 simulation_sample_rate, ISO7816AnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	/* VCC */
	mSimulationDataVCC = mSimulationChannelsISO7816.Add(	mSettings->mChannelVCC,
															simulation_sample_rate,
															BIT_LOW );

	/* RST */
	mSimulationDataRST = mSimulationChannelsISO7816.Add(	mSettings->mChannelRST,
															simulation_sample_rate,
															BIT_LOW );

	/* CLK */
	mSimulationDataCLK = mSimulationChannelsISO7816.Add(	mSettings->mChannelCLK,
															simulation_sample_rate,
															BIT_LOW );

	/* IO */
	mSimulationDataIO = mSimulationChannelsISO7816.Add(		mSettings->mChannelIO,
															simulation_sample_rate,
															BIT_LOW );
}

U32 ISO7816SimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );
	U64 current_sample = 0;
	U8 char_value = 0;

	mEtu = 372;
	mCLKGenerator.Init(4000000, sample_rate);

	mSimulationDataVCC->TransitionIfNeeded( BIT_LOW );
	mSimulationChannelsISO7816.AdvanceAll(20000);
	mSimulationDataVCC->TransitionIfNeeded( BIT_HIGH );
	mSimulationChannelsISO7816.AdvanceAll(400);
	mSimulationDataCLK->TransitionIfNeeded( BIT_HIGH );
	mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
	mSimulationChannelsISO7816.AdvanceAll(200);
	GenerateEtu(100);
	mSimulationDataRST->TransitionIfNeeded( BIT_HIGH );
	GenerateEtu(20);

	// ATR
	for (U8 i = 0; i < sizeof(cATR); i++)
	{
		GenerateChar(cATR[i]);
	}
	GenerateEtu(150);

	// PPS Request
	for (U8 i = 0; i < sizeof(cPPS); i++)
	{
		GenerateChar(cPPS[i]);
	}
	GenerateEtu(80);

	// PPS Response
	for (U8 i = 0; i < sizeof(cPPS); i++)
	{
		GenerateChar(cPPS[i]);
	}
	GenerateEtu(200);

	while( char_value != 0xFF )
	{
		GenerateChar((char_value++) & 0xFF);
		current_sample = mSimulationDataCLK->GetCurrentSampleNumber();
	}

	*simulation_channel = mSimulationChannelsISO7816.GetArray();

	return mSimulationChannelsISO7816.GetCount();
}

void ISO7816SimulationDataGenerator::GenerateChar(const U8 value)
{
	/* Generate start bit */
	mSimulationDataIO->TransitionIfNeeded( BIT_LOW );
	GenerateEtu(1);

	/* Generate character */
	for( U8 i=0; i<8; i++ )
	{
		if( ( value & (0x01 << i) ) != 0 )
			mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
		else
			mSimulationDataIO->TransitionIfNeeded( BIT_LOW );

		GenerateEtu(1);
	}

	/* Generate parity bit */
	if(AnalyzerHelpers::IsOdd(value))
	{
		mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
	}else
	{
		mSimulationDataIO->TransitionIfNeeded( BIT_LOW );
	}
	GenerateEtu(1);

	/* Generate stop bit */
	mSimulationDataIO->TransitionIfNeeded( BIT_HIGH );
	GenerateEtu(2);
}

void ISO7816SimulationDataGenerator::GenerateEtu(const U8 value)
{
	U64 clk_hit;
	for (clk_hit = 0; clk_hit < (U64)(mEtu * value); clk_hit++)
	{
		mSimulationDataCLK->TransitionIfNeeded( BIT_LOW );
		mSimulationChannelsISO7816.AdvanceAll(mCLKGenerator.AdvanceByHalfPeriod(0.5));
		mSimulationDataCLK->TransitionIfNeeded( BIT_HIGH );
		mSimulationChannelsISO7816.AdvanceAll(mCLKGenerator.AdvanceByHalfPeriod(0.5));
	}
}
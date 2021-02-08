#ifndef ISO7816_SIMULATION_DATA_GENERATOR
#define ISO7816_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>

#include <AnalyzerHelpers.h>

class ISO7816AnalyzerSettings;

class ISO7816SimulationDataGenerator
{
public:
	ISO7816SimulationDataGenerator();
	~ISO7816SimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, ISO7816AnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	ISO7816AnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void GenerateChar(const U8 value);
	void GenerateEtu(const U8 value);

	double mEtu;
	ClockGenerator mCLKGenerator;

	SimulationChannelDescriptorGroup mSimulationChannelsISO7816;

	SimulationChannelDescriptor* mSimulationDataVCC;
	SimulationChannelDescriptor* mSimulationDataRST;
	SimulationChannelDescriptor* mSimulationDataCLK;
	SimulationChannelDescriptor* mSimulationDataIO;

};
#endif //ISO7816_SIMULATION_DATA_GENERATOR
#ifndef ISO7816_SIMULATION_DATA_GENERATOR
#define ISO7816_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
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

	double mEtu;
	bool mInverseConvention;

	ClockGenerator mCLKGenerator;

	SimulationChannelDescriptorGroup mSimulationChannelsISO7816;

	SimulationChannelDescriptor* mSimulationDataVCC;
	SimulationChannelDescriptor* mSimulationDataRST;
	SimulationChannelDescriptor* mSimulationDataCLK;
	SimulationChannelDescriptor* mSimulationDataIO;

	// Character generators
	void GenerateCharDirect(U8 value, bool force_parity_err = false);
	void GenerateCharInverse(U8 value, bool force_parity_err = false);
	void GenerateCharAuto(U8 value, bool force_parity_err = false);
	void GenerateEtu(U8 value);

	// Power cycle helpers
	void PowerOn();
	void PowerOff();

	// Session generators
	void GenerateSessionT0DirectPPS();     // T=0, direct, PPS F=9/D=4, 6 APDUs
	void GenerateSessionT0MinimalNoPPS();  // T=0, minimal ATR, no PPS, 3 APDUs
	void GenerateSessionT1DirectPPS();     // T=1, direct, PPS, full T=1 block suite
	void GenerateSessionT0Inverse();       // T=0, inverse convention, 2 APDUs
	void GenerateSessionErrors();          // parity err, NULL spam, bad PROC, abort+recovery
};

#endif //ISO7816_SIMULATION_DATA_GENERATOR

#ifndef ISO7816_ANALYZER_H
#define ISO7816_ANALYZER_H

#include <Analyzer.h>
#include <vector>
#include "ISO7816AnalyzerResults.h"
#include "ISO7816SimulationDataGenerator.h"
#include "ISO7816Context.h"
#include "ISO7816Node.h"

class ISO7816AnalyzerSettings;
class ANALYZER_EXPORT ISO7816Analyzer : public Analyzer2
{
  public:
    ISO7816Analyzer();
    virtual ~ISO7816Analyzer();

    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
    virtual U32 GetMinimumSampleRateHz();

    virtual const char* GetAnalyzerName() const;
    virtual bool NeedsRerun();

    void newFrame( ISO7816Node* node );
    ISO7816Context* GetContext( void );
    ISO7816Node* GetNodeByFrameId( U64 frameId );

  protected: // functions
    void Setup();
    void SyncToSample( U64 to_sample );
    void AdvanceEtu( double etu = 1.0 );


  protected: // vars
    std::unique_ptr<ISO7816AnalyzerSettings> mSettings;
    std::unique_ptr<ISO7816AnalyzerResults> mResults;
    AnalyzerChannelData* mVCC;
    AnalyzerChannelData* mRST;
    AnalyzerChannelData* mCLK;
    AnalyzerChannelData* mIO;

    ISO7816SimulationDataGenerator mSimulationDataGenerator;
    bool mSimulationInitialized;

    // Serial analysis vars:
    U32 mSampleRateHz;
    ISO7816Context* mContext;
    std::vector<ISO7816Node*> mNodes;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif // ISO7816_ANALYZER_H

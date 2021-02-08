#ifndef ISO7816_ANALYZER_SETTINGS
#define ISO7816_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class ISO7816AnalyzerSettings : public AnalyzerSettings
{
public:
	ISO7816AnalyzerSettings();
	virtual ~ISO7816AnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mChannelVCC;
	Channel mChannelRST;
	Channel mChannelCLK;
	Channel mChannelIO;

	double mEtu;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mChannelInterfaceVCC;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mChannelInterfaceRST;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mChannelInterfaceCLK;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mChannelInterfaceIO;
};

#endif //ISO7816_ANALYZER_SETTINGS

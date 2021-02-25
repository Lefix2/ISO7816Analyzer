#include "ISO7816AnalyzerSettings.h"
#include <AnalyzerHelpers.h>

ISO7816AnalyzerSettings::ISO7816AnalyzerSettings()
:	mChannelVCC( UNDEFINED_CHANNEL ),
	mChannelRST( UNDEFINED_CHANNEL ),
	mChannelCLK( UNDEFINED_CHANNEL ),
	mChannelIO( UNDEFINED_CHANNEL )
{
	mChannelInterfaceVCC.reset( new AnalyzerSettingInterfaceChannel() );
	mChannelInterfaceVCC->SetTitleAndTooltip( "VCC", "ISO/IEC-7816 Power line" );
	mChannelInterfaceVCC->SetChannel( mChannelVCC );
	mChannelInterfaceVCC->SetSelectionOfNoneIsAllowed(true);

	mChannelInterfaceRST.reset( new AnalyzerSettingInterfaceChannel() );
	mChannelInterfaceRST->SetTitleAndTooltip( "RST", "ISO/IEC-7816 Reset line" );
	mChannelInterfaceRST->SetChannel( mChannelRST );

	mChannelInterfaceCLK.reset( new AnalyzerSettingInterfaceChannel() );
	mChannelInterfaceCLK->SetTitleAndTooltip( "CLK", "ISO/IEC-7816 Clock line" );
	mChannelInterfaceCLK->SetChannel( mChannelCLK );

	mChannelInterfaceIO.reset( new AnalyzerSettingInterfaceChannel() );
	mChannelInterfaceIO->SetTitleAndTooltip( "IO", "ISO/IEC-7816 Data line" );
	mChannelInterfaceIO->SetChannel( mChannelIO );

	AddInterface( mChannelInterfaceVCC.get() );
	AddInterface( mChannelInterfaceRST.get() );
	AddInterface( mChannelInterfaceCLK.get() );
	AddInterface( mChannelInterfaceIO.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mChannelVCC, "VCC", false );
	AddChannel( mChannelRST, "RST", false );
	AddChannel( mChannelCLK, "CLK", false );
	AddChannel( mChannelIO,  "IO",  false );
}

ISO7816AnalyzerSettings::~ISO7816AnalyzerSettings()
{
}

bool ISO7816AnalyzerSettings::SetSettingsFromInterfaces()
{
	const int NUM_CHANNELS = 4;

	Channel all_channels[NUM_CHANNELS] = {
			mChannelInterfaceVCC->GetChannel(),
			mChannelInterfaceRST->GetChannel(),
			mChannelInterfaceCLK->GetChannel(),
			mChannelInterfaceIO->GetChannel()
	};

	if (all_channels[1] == UNDEFINED_CHANNEL ||
		all_channels[2] == UNDEFINED_CHANNEL ||
		all_channels[3] == UNDEFINED_CHANNEL)
	{
		SetErrorText("Please select inputs for each channels.");
		return false;
	}

	if (AnalyzerHelpers::DoChannelsOverlap(all_channels, NUM_CHANNELS))
	{
		SetErrorText("Please select different channels for each input.");
		return false;
	}

	mChannelVCC = all_channels[0];
	mChannelRST = all_channels[1];
	mChannelCLK = all_channels[2];
	mChannelIO  = all_channels[3];

	ClearChannels();
	AddChannel( mChannelVCC, "VCC", true );
	AddChannel( mChannelRST, "RST", true );
	AddChannel( mChannelCLK, "CLK", true );
	AddChannel( mChannelIO,  "IO",  true );

	return true;
}

void ISO7816AnalyzerSettings::UpdateInterfacesFromSettings()
{
	mChannelInterfaceVCC->SetChannel( mChannelVCC );
	mChannelInterfaceRST->SetChannel( mChannelRST );
	mChannelInterfaceCLK->SetChannel( mChannelCLK );
	mChannelInterfaceIO->SetChannel( mChannelIO );
}

void ISO7816AnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mChannelVCC;
	text_archive >> mChannelRST;
	text_archive >> mChannelCLK;
	text_archive >> mChannelIO;

	ClearChannels();
	AddChannel( mChannelVCC, "VCC", true );
	AddChannel( mChannelRST, "RST", true );
	AddChannel( mChannelCLK, "CLK", true );
	AddChannel( mChannelIO,  "IO",  true );

	UpdateInterfacesFromSettings();
}

const char* ISO7816AnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mChannelVCC;
	text_archive << mChannelRST;
	text_archive << mChannelCLK;
	text_archive << mChannelIO;

	return SetReturnString( text_archive.GetString() );
}

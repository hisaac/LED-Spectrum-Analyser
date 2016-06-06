/*
 *  ITSpectrumAnalyserPlugIn.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Wed Mar 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include	"ITPlugIn.h"
#include	"ITBarDisplay.h"


class ITSpectrumAnalyserPlugIn : public ITPlugIn
{
public:
	ITSpectrumAnalyserPlugIn();
	virtual ~ITSpectrumAnalyserPlugIn();

	virtual void		GetPlugInName( Str63 outName );
	virtual void		Initialise();

	static pascal void		ControlCallback( ControlRef theControl, ControlPartCode partCode );
	
protected:
	virtual void		InitGraphics();
	virtual void		DisposeGraphics();
	virtual void		Render();
	virtual void		Idle();
	virtual void		Update();
	virtual void		TrackInfoChanged( ITTrackInfo* newTrackInfo );
	virtual void		StreamInfoChanged( ITStreamInfo* newStreamInfo );
	virtual void		CleanUp();

	virtual Boolean		DoKeyDownEvent( const UInt16 theKey, const EventModifiers modifiers );
	
	virtual void		NewConfigDialog();
	virtual void		ConfigDialogOpen();
	virtual void		ConfigDialogClose();
	virtual void		ConfigDialogActivity( ControlRef hitControl, const UInt32 controlID );
	
	SInt32				GetConfigDialogValue( const OSType sig, const UInt32 controlID );
	void				SetConfigDialogValue( const OSType sig, const UInt32 controlID, const SInt32 value );
	void				SetConfigDialogStaticText( const UInt32 controlID, const Str255 theText );
	void				SetConfigDialogEditText( const UInt32 controlID, const Str255 theText );
	void				SetElementColourFromControl( ControlRef theControl, const UInt32 controlID );
	void				SetConfigDialogControlColour( const UInt32 controlID, const RGBColor& theColour );
	void				EnableConfigDialogControl( const OSType sig, const UInt32 controlID );
	void				DisableConfigDialogControl( const OSType sig, const UInt32 controlID );
	
	void				SwitchConfigTabs( SInt32 newValue );
	void				SwitchLogLin( SInt32 newValue );
	void				SwitchPeakOnOff( SInt32 newValue );
	void				ShowHideUnlitSegments( SInt32 newValue );

	void				SetBarDecayTime( SInt32 newValue );
	void				SetPeakDecayTime( SInt32 newValue );
	void				SetPeakHoldTime( SInt32 newValue );
	void				ShowHideBorder( SInt32 newValue );
	void				ShowHideVUMeters( SInt32 newValue );
	void				SetFlipLeftChannel( SInt32 newValue );
	void				SetFlipRightChannel( SInt32 newValue );
	void				SetLayout( SInt32 newValue );
	void				SetWindowFit( SInt32 newValue );	
	void				SetScalesVisible( SInt32 newValue );
	
	void				SwitchColourPalette( SInt32 newValue );
	void				SwitchColourMode( SInt32 newMode );
	void				SetChannels( SInt32 newValue );
	void				SetFontSize( SInt32 newValue );
	
	void				SavePreferences();
	void				ReadPreferences();
	void				SaveIntegerPreference( CFStringRef key,  const SInt32 theInt );
	void				SaveBooleanPreference( CFStringRef key, const bool theBool );
	SInt32				ReadIntegerPreference( CFStringRef key, const SInt32 missingDefault = 0 );
	bool				ReadBooleanPreference( CFStringRef key, const bool missingDefault = false );
	void				SaveRectPreference( CFStringRef key,  const Rect& theRect );
	void				ReadRectPreference( CFStringRef key, Rect* theRect );
	void				SaveRGBColorPreference( CFStringRef key,  const RGBColor& theColour );
	void				ReadRGBColorPreference( CFStringRef key, RGBColor* theColour );
	
	void				DisplayTrackInfo( const UInt32 forHowLong ){ trackInfoTimer = forHowLong; };
	void				DrawTrackInfo( const bool erase = false );
	void				TrackInfoCtl();
	void				TriggerTrackInfoDisplay();
	
	void				FixupJaguarMenus();
	
	void				SetCmdString( const UInt16 forCmdKey, const SInt16 stateInfo );
	void				DrawCmdString();
	void				ChooseNewRandomColours();

	
	static pascal OSStatus  DialogEventHandler( EventHandlerCallRef inRef, EventRef inEvent, void* userData );

	SInt16				resFile;				// resource file ref
	ITBarDisplay*		display;				// the object that draws the display
	UInt16				curTab;					// which tab shown in options dialog
	UInt32				trackInfoTimer;			// tracks timing of track info
	UInt32				trackFadeTimer;			// fade info out timer
	UInt32				cmdFadeTimer;			// fade cmd feedback timer
	SInt16				paletteIndex;			// which palette in use
	SInt16				paletteID;				// res ID of the palette
	bool				trackInfoEnabled;		// show track info?
	bool				showAlbumInfo;			// include album into
	bool				showUnlit;				// show unlit segments
	bool				randColours;			// change colours at random when track changes
	SInt16				trackTime;				// track info shown for this many seconds
	Rect				configDialogPosition;   // where options dialog is on screen
	UInt16				spectrumChannels;		// number of channels of spectrum data to make
	UInt16				fontSize;				// size of font for track info
	RGBColor			trackInfoColour;		// colour of track info
	CFStringRef			cmdString;				// current cmd string
	Rect				cmdStringRect;			// where it's drawn
};

// sigs and IDs of the controls in our settings dialog:

enum
{
	kSATabsControlSignature			= 'tabz',
	kSALayoutPanelID				= 100,		// first tab user pane sig, ID 1
	kSAMetersPanelID				= 200,		// second tab, ID 2
	kSAColoursPanelID				= 300,		// third tab, ID 3
	kSAAboutPanelID					= 399,
	
	kSASignatureRadio				= 'radi',
	kSASignatureCheckbox			= 'cbox',
	kSASignatureSlider				= 'sldr',
	kSASignatureBevelButton			= 'bevl',
	kSASignaturePopUpMenu			= 'popm',
	kSASignatureStaticText			= 'stat',
	kSASignatureColourPopUp			= 'cpop',
	kSASignatureEditField			= 'edit',
	kSASignatureLittleArrows		= 'arrw',
	kSASignaturePushButton			= 'butn',
	kSASignatureGroupBox			= 'gbox',
	
	kSATabsControlID				= 400,
	
	kSALayoutRadioID				= 1,		// first tab controls
	kSAShowVUCheckID				= 2,
	kSAFlipLeftCheckID				= 3,
	kSABorderCheckID				= 4,
	kSAFitWindowRadioID				= 5,
	kSAFlipRightCheckID				= 6,
	kSAShowTrackInfoCheckID			= 9,
	kSAShowAlbumCheckID				= 10,
	kSAChooseFontSliderID			= 11,
	kSAFreqBandsStaticTextID		= 12,
	kSAMeterScalesCheckID			= 13,
	kSASpectrumChannelsSlider		= 20,
	kSAFreqBandsLabelStaticTextID   = 21,
	kSAKeepTrackInfoVisibleCheck	= 30,
	
	kSALogLinRadioID				= 101,		// second tab controls
	kSAShowPeakCheckID				= 102,
	kSABarDecaySliderID				= 103,
	kSAPeakHoldSliderID				= 104,
	kSAPeakDecaySliderID			= 105,
	kSAUnlitSegmentsCheckID			= 106,
	kSADecayOptionRadioID			= 107,
	kSABarDecayStaticTextID			= 111,
	kSAPeakHoldStaticTextID			= 112,
	kSAPeakDecayStaticTextID		= 113,
	
	kSACModePopUpID					= 201,		// third tab controls
	kSACPalettePopUpID				= 202,
	kSAMeterElementsGroupID			= 209,
	kSABarColourBevelID				= 211,
	kSAAltColourBevelID				= 212,
	kSAPeakColourBevelID			= 213,
	kSAVUBarColourBevelID			= 214,
	kSAVUAltColourBevelID			= 215,
	kSAVUPeakColourBevelID			= 216,
	kSARandomiseColoursCheckID		= 220,
	
	kControlSettingLinear			= 1,
	
	kSAColourPaletteDefault			= 1,
	kSAColourPaletteSystem			= 2,
	kSAColourPalettePastels			= 3,
	kSAColourPaletteFireIce			= 4,
	kSAColourPaletteChrome			= 5,
	kSAColourPaletteGold			= 6
};

enum
{
	kTrackInfoFadeTime				= 2 * 30,		// about 2 seconds
	kCmdInfoFadeTime				= 3 * 30		// 3 sec
};



void	SubstituteString( Str255 theString, const char* markerSeq, const Str255 subString );
void	CopyPString( const Str255 srcString, Str255 destString );


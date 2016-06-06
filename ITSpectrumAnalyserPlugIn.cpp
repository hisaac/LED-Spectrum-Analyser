/*
 *  ITSpectrumAnalyserPlugIn.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Wed Mar 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include	"ITSpectrumAnalyserPlugIn.h"
#include	"CustomHIViewControl.h"
#include	"ITPortSaver.h"

static ControlActionUPP		SACB = NewControlActionUPP( ITSpectrumAnalyserPlugIn::ControlCallback );

ITPlugIn*  NewPlugIn()
{
	return new ITSpectrumAnalyserPlugIn(); //<--- change this to make your subclass of plugin
}


/*------------------------------***  SubstituteString  ***------------------------------*/
/*
access:			global
overrides:		
description: 	insert a substring into a string where the marker sequence is found
ins: 			<theString> original string - modified in place
				<markerSeq> sequence to replace (n.b. C string)
				<subString> string that replaces marker sequence
outs: 			none
notes:			
----------------------------------------------------------------------------------------*/

void	SubstituteString( Str255 theString, const char* markerSeq, const Str255 subString )
{
	StringHandle	temp;
	long			len;
	
	temp = NewString( theString );
	
	if ( temp )
	{
		len = strlen( markerSeq );
		
		Munger((Handle) temp, 1, markerSeq, len, &subString[1], subString[0] );	
		**temp = MIN( 255, GetHandleSize((Handle) temp ) - 1 );
		
		BlockMoveData( *temp, (Ptr) theString, (**temp) + 1 );
		DisposeHandle((Handle) temp );
	}
}


/*---------------------------------***  CopyPString  ***--------------------------------*/
/*
access:			global
overrides:		
description: 	copy one pascal string to another
ins: 			<srcString> string to copy
				<destString> receives copy of srcString
outs: 			none
notes:			
----------------------------------------------------------------------------------------*/

void	CopyPString( const Str255 srcString, Str255 destString )
{
	BlockMoveData( srcString, destString, srcString[0] + 1 );
}




ITSpectrumAnalyserPlugIn::ITSpectrumAnalyserPlugIn()
	: ITPlugIn()
{
	display = NULL;
	curTab = 1;			//default prefs panel
	trackInfoTimer = 0;
	trackFadeTimer = 0;
	cmdFadeTimer = 0;
	resFile = 0;
	paletteIndex = 0;
	paletteID = 0;
	trackInfoEnabled = true;
	showAlbumInfo = false;
	trackTime = 10;
	SetRect( &configDialogPosition, -1, -1, -1, -1 );
	spectrumChannels = 14;
	fontSize = 12;
	trackInfoColour = RGB_WHITE;
	showUnlit = false;
	randColours = false;
	cmdString = NULL;
	SetRect( &cmdStringRect, 0, 0, 0, 0 );
	
	// register custom view object for handling colour pop-ups
	
	HIViewColourPopUpControl::SelfRegister();
}




ITSpectrumAnalyserPlugIn::~ITSpectrumAnalyserPlugIn()
{
	CleanUp();
	
	if ( resFile )
		CloseResFile( resFile );
}


void		ITSpectrumAnalyserPlugIn::GetPlugInName( Str63 outName )
{
	Str63 t = "\pLED Spectrum Analyser";
	BlockMoveData((Ptr) t, (Ptr) outName, t[0] + 1 );
}


void		ITSpectrumAnalyserPlugIn::Initialise()
{
	// open our res fork so we can get stuff from there when required
	
	// the filespec of the plug-in is the FSSpec of the bundle. We need to go into that to
	// find our actual resource file <bundle>/Contents/Resources/ourResFile.rsrc
	FSRef		fsr;
	OSStatus	err;
	char		thePath[1024];
	
	FSpMakeFSRef( &pluginFileSpec, &fsr );
	FSRefMakePath( &fsr, (UInt8*) thePath, 1024 );
	strcat( thePath, "/Contents/Resources/analyser.resources" );
	
	//fprintf( stdout, thePath, "\n" );
	
	err = FSPathMakeRef((UInt8*) thePath, &fsr, NULL );
	
	if ( err == noErr )
	{
		err = FSOpenResourceFile( &fsr, 0, NULL, fsRdPerm, &resFile );
		
		if ( err == noErr )
			UseResFile( resFile );
	}
	
	ReadPreferences();
	
	SInt16 p = ReadIntegerPreference( CFSTR("ITSAValueColourPalette"), 0 );
	SwitchColourPalette( p );
}
	

void		ITSpectrumAnalyserPlugIn::InitGraphics()
{
	ITPortSaver ps( destPort );

	if ( display == NULL )
	{
		display = new ITBarDisplay( destRect, spectrumChannels );
	}

	InvalWindowRect( GetWindowFromPort( destPort ), &destRect );
	
	cmdStringRect = destRect;
	InsetRect( &cmdStringRect, 4, 4 );
	cmdStringRect.bottom = cmdStringRect.top + 15;
}


void		ITSpectrumAnalyserPlugIn::DisposeGraphics()
{
	ITPortSaver ps( destPort );

	if ( display )
	{
		if ( destPort )
		{
			Rect br;
			
			display->GetBounds( &br );
			
			InsetRect( &br, -3, -3 );
			RGBForeColor( &RGB_BLACK );
			PaintRect( &br );
		}
		delete display;
		display = NULL;
	}
}


void		ITSpectrumAnalyserPlugIn::Render()
{
	if ( display )
	{
		//if ( IsFullScreen())
		{
			static SInt32 t = 0;
			
			SInt32 nt = TickCount();
			if ( nt <  ( t + 1 ))
				return;
			else
				t = nt;
		}
		
		if ( renderData )
			display->Update( renderData->spectrumData, IsFullScreen());
		else
			display->Update( NULL, IsFullScreen());	
	}
	
	TrackInfoCtl();
}


void		ITSpectrumAnalyserPlugIn::Idle()
{
	ITPortSaver ps( destPort );
	
	if ( display )
		display->Update( NULL, IsFullScreen());
		
	TrackInfoCtl();
}


void		ITSpectrumAnalyserPlugIn::Update()
{
	ITPortSaver ps( destPort );

	PaintRect( &destRect );
	
	if ( display )
	{
		display->SetForceRepaint( true );
		display->Update( NULL );
	}
	
	if ( trackInfoTimer && trackInfoEnabled )
		DrawTrackInfo();
}


void		ITSpectrumAnalyserPlugIn::TrackInfoChanged( ITTrackInfo* newTrackInfo )
{
	if ( newTrackInfo )
		ITPlugIn::TrackInfoChanged( newTrackInfo );
	
	TriggerTrackInfoDisplay();
}


void		ITSpectrumAnalyserPlugIn::StreamInfoChanged( ITStreamInfo* newStreamInfo )
{
	ITPlugIn::StreamInfoChanged( newStreamInfo );
	
	// copy track info to track info record
	
	if ( newStreamInfo && trackInfoEnabled )
	{
		if ( newStreamInfo->streamTitle[0] > 0 )
		{
			CopyPString( newStreamInfo->streamTitle, trackInfo.artist );
			TriggerTrackInfoDisplay();
		}
	}
}


void		ITSpectrumAnalyserPlugIn::TriggerTrackInfoDisplay()
{
	if ( IsValidPort( destPort ))
	{
		ITPortSaver ps( destPort );
	
		trackFadeTimer = 0;
		
		if ( trackInfoEnabled )
		{
			trackInfoTimer = TickCount() + ( trackTime * 60 );
			DrawTrackInfo( false );
		}
		else
		{
			trackInfoTimer = 0;
			DrawTrackInfo( true );
		}	
		
		if ( randColours && ITBarDisplay::GetColourMode() != kAnimatedColours )
			ChooseNewRandomColours();
	}
}


void		ITSpectrumAnalyserPlugIn::CleanUp()
{
	if ( configDialog )
	{
		EventRef	evt;
		OSStatus	err;
		
		err = CreateEvent( NULL, kEventClassWindow, kEventWindowClose, 0, kEventAttributeNone, &evt );
		if ( err == noErr )
		{
			SendEventToWindow( evt, configDialog );
			ReleaseEvent( evt );
		}
	}
	
	SavePreferences();
	DisposeGraphics();
}


Boolean		ITSpectrumAnalyserPlugIn::DoKeyDownEvent( const UInt16 theKey, const EventModifiers modifiers )
{
	Boolean handledIt = false;
	SInt16  si = 0;
	
	ITPortSaver ps( destPort );

	switch( theKey )
	{
		case 'b':
		case 'B':
			if ( display )
			{
				ShowHideBorder( ! display->IsBorderEnabled());
				si = display->IsBorderEnabled();
				handledIt = true;
			}
			break;
		
		case 'v':
		case 'V':
			if ( display )
			{
				ShowHideVUMeters( ! display->VUMetersVisible());
				si = display->VUMetersVisible();
				handledIt = true;
			}
			break;
			
		case 'p':
		case 'P':
			if ( display )
			{
				SwitchPeakOnOff( ! display->IsPeakEnabled());
				si = display->IsPeakEnabled();
				handledIt = true;
			}
			break;
			
		case 'n':
		case 'N':
			if ( display )
			{
				SwitchLogLin(( display->GetMeterType() == kLinear )? 2 : 1 );
				si = display->GetMeterType() == kLinear;
				handledIt = true;
			}
			break;
			
		case 's':
		case 'S':
			SetLayout(( ITBarDisplay::GetLayout() & 3 ) == kLayoutSideBySide? 2 : 1 );
			si = ( ITBarDisplay::GetLayout() & 3 ) == kLayoutSideBySide;
			handledIt = true;
			break;
			
		case 'x':
		case 'X':
			SetLayout( 3 );
			handledIt = true;
			break;
			
		case 'f':
		case 'F':
			SetWindowFit(( ITBarDisplay::GetLayout() & kLayoutFixedSize ) == 0? 2 : 1 );
			si = ( ITBarDisplay::GetLayout() & kLayoutFixedSize ) == 0;
			handledIt = true;
			break;
			
		case 'r':
		case 'R':
			SetFlipRightChannel( ITBarDisplay::IsRightChannelFlipped()? 0 : 1 );
			si = ITBarDisplay::IsRightChannelFlipped();
			handledIt = true;
			break;
			
		case 'l':
		case 'L':
			SetFlipLeftChannel( ITBarDisplay::IsLeftChannelFlipped()? 0 : 1 );
			si = ITBarDisplay::IsLeftChannelFlipped();
			handledIt = true;
			break;
			
		case 't':
		case 'T':
			trackInfoEnabled = true;
			SetConfigDialogValue( kSASignatureCheckbox, kSAShowTrackInfoCheckID, trackInfoEnabled );
			SetConfigDialogValue( kSASignatureCheckbox, kSAKeepTrackInfoVisibleCheck, trackTime >= 10000 );
			TrackInfoChanged( NULL );
			si = trackInfoEnabled;
			handledIt = true;
			break;
			
		case '.':
		case '>':
			SetChannels( MIN( 31, spectrumChannels + 1 ));
			si = spectrumChannels;
			handledIt = true;
			break;
			
		case ',':
		case '<':
			SetChannels( MAX( 10, spectrumChannels - 1 ));
			si = spectrumChannels;
			handledIt = true;
			break;
			
		case 'i':
		case 'I':
			SetFontSize( MIN( 48, fontSize + 1 ));
			si = fontSize;
			handledIt = true;
			break;
			
		case 'd':
		case 'D':
			SetFontSize( MAX( 9, fontSize - 1 ));
			si = fontSize;
			handledIt = true;
			break;
			
		case 'a':
		case 'A':
			if ( trackInfoEnabled )
			{
				showAlbumInfo = !showAlbumInfo;
				SetConfigDialogValue( kSASignatureCheckbox, kSAShowAlbumCheckID, showAlbumInfo );
				TrackInfoChanged( NULL );
				si = showAlbumInfo;
				handledIt = true;
			}
			break;
			
		case 'u':
		case 'U':
			ShowHideUnlitSegments( !showUnlit );
			si = showUnlit;
			handledIt = true;
			break;
			
		case 'm':
		case 'M':
			if (( ITBarDisplay::GetLayout() & 3 ) != kLayoutAnalogueVUMeters )
			{
				SetScalesVisible( !ITBarDisplay::IsScalesVisible());
				si = ITBarDisplay::IsScalesVisible();
				handledIt = true;
			}
			break;
			
		case 'e':
		case 'E':
			ITBarGraph::expDecay = !ITBarGraph::expDecay;
			SetConfigDialogValue( kSASignatureRadio, kSADecayOptionRadioID, ITBarGraph::expDecay? 1 : 2 );
			si = ITBarGraph::expDecay;
			handledIt = true;
			break;
			
		case '1':
		case '2':
		case '3':
		case '4':
			if ( configDialog )
				SwitchConfigTabs( theKey - '1' + 1 );
			break;
	}
	
	if ( handledIt )
		SetCmdString( theKey, si );
		
	return handledIt;
}



void		ITSpectrumAnalyserPlugIn::NewConfigDialog()
{
	// default is to load dialog from NIB in bundle

	IBNibRef 		nibRef;
	//we have to find our bundle to load the nib inside of it
	
	CFBundleRef iTunesXPlugin;
	
	iTunesXPlugin = CFBundleGetBundleWithIdentifier( CFSTR( "LEDSpectrumAnalyser" ));
	
	if ( iTunesXPlugin )
	{	
		CreateNibReferenceWithCFBundle( iTunesXPlugin, CFSTR( "SettingsDialog" ), &nibRef );
		
		if ( nibRef )
		{
			UseResFile( resFile );
			CreateWindowFromNib( nibRef, CFSTR( "PluginSettings" ), &configDialog );
			DisposeNibReference( nibRef );
			
			// if window was created, set up callback to handle its events. At this stage we are only interested
			// in control hit events
			
			if ( configDialog )
			{
				static EventTypeSpec eventSpec[] = { kEventClassControl, kEventControlHit,
													 kEventClassWindow, kEventWindowClose,
													 kEventClassTextInput, kEventTextInputUnicodeForKeyEvent };
				
				InstallWindowEventHandler( configDialog, NewEventHandlerUPP( DialogEventHandler ), 3, eventSpec, this, NULL );
			}
		}
	}
	else
		SysBeep( 1 );
}


void		ITSpectrumAnalyserPlugIn::ConfigDialogOpen()
{
	// dialog was opened - initialise it to current state
	
	RenderVisualData* saveren = renderData;
	renderData = NULL;
	
	SwitchConfigTabs( curTab );
	
	//on 10.2, clear checkmarks from pop-up menus
	
	FixupJaguarMenus();
	
	SetConfigDialogValue( kSASignatureCheckbox, kSAFlipLeftCheckID, ITBarDisplay::IsLeftChannelFlipped()? 1 : 0 );
	SetConfigDialogValue( kSASignatureCheckbox, kSAFlipRightCheckID, ITBarDisplay::IsRightChannelFlipped()? 1 : 0 );
	SetConfigDialogValue( kSASignatureCheckbox, kSAShowTrackInfoCheckID, trackInfoEnabled );
	SetConfigDialogValue( kSASignatureCheckbox, kSAShowAlbumCheckID, showAlbumInfo );
	SetConfigDialogValue( kSASignatureCheckbox, kSAMeterScalesCheckID, ITBarDisplay::IsScalesVisible());
	SetConfigDialogValue( kSASignatureCheckbox, kSARandomiseColoursCheckID, randColours );

	SetChannels( spectrumChannels );
	ShowHideUnlitSegments( showUnlit );
	
	if ( display )
	{
		ShowHideBorder( display->IsBorderEnabled());
		ShowHideVUMeters( display->VUMetersVisible());
		SwitchPeakOnOff( display->IsPeakEnabled());
		SwitchLogLin( display->GetMeterType());
		SetLayout(( ITBarDisplay::GetLayout() & 3 ) + 1 );
		SetWindowFit(( ITBarDisplay::GetLayout() & kLayoutFixedSize ) == 0? 1 : 2 );
		
		SetBarDecayTime( display->GetDecayTime( kMainBar ));
		SetPeakDecayTime( display->GetDecayTime( kPeakDecay ));
		SetPeakHoldTime( display->GetDecayTime( kPeakHold ));
	}
	
	SetConfigDialogValue( kSASignatureRadio, kSADecayOptionRadioID, ITBarGraph::expDecay? 1 : 2 );
	
	SetConfigDialogControlColour( kSABarColourBevelID, ITBarDisplay::GetFixedColour( kBarColour ));
	SetConfigDialogControlColour( kSAAltColourBevelID, ITBarDisplay::GetFixedColour( kAltBarColour ));
	SetConfigDialogControlColour( kSAPeakColourBevelID, ITBarDisplay::GetFixedColour( kPeakColour ));
	SetConfigDialogControlColour( kSAVUBarColourBevelID, ITBarDisplay::GetFixedColour( kVUBarColour ));
	SetConfigDialogControlColour( kSAVUAltColourBevelID, ITBarDisplay::GetFixedColour( kVUAltBarColour ));
	SetConfigDialogControlColour( kSAVUPeakColourBevelID, ITBarDisplay::GetFixedColour( kVUPeakColour ));
	
	SInt16 p = paletteIndex;
	if ( p == 0 )
		p = 1;  // protect against prefs fault
	SwitchColourPalette( p );
	SwitchColourMode( ITBarDisplay::GetColourMode());
	
	// restore dialog to its previous position:
	
	if (( configDialogPosition.left != -1 ) && ( configDialogPosition.top != -1 ))
	{
		MoveWindow( configDialog, configDialogPosition.left, configDialogPosition.top, true );
	}
	
	// install live feedback callback on sliders
	
	ControlID   cid;
	ControlRef  theControl;
	
	cid.signature = kSASignatureSlider;
	cid.id = kSASpectrumChannelsSlider;
	GetControlByID( configDialog, &cid, &theControl );
	
	if ( theControl )
	{
		SetControlAction( theControl, SACB );
		SetControlReference( theControl, (SInt32) this );
	}
	
	cid.id = kSABarDecaySliderID;
	GetControlByID( configDialog, &cid, &theControl );
	
	if ( theControl )
	{
		SetControlAction( theControl, SACB );
		SetControlReference( theControl, (SInt32) this );
	}
	
	cid.id = kSAPeakHoldSliderID;
	GetControlByID( configDialog, &cid, &theControl );
	
	if ( theControl )
	{
		SetControlAction( theControl, SACB );
		SetControlReference( theControl, (SInt32) this );
	}
	
	cid.id = kSAPeakDecaySliderID;
	GetControlByID( configDialog, &cid, &theControl );
	
	if ( theControl )
	{
		SetControlAction( theControl, SACB );
		SetControlReference( theControl, (SInt32) this );
	}
	
	cid.id = kSAChooseFontSliderID;
	GetControlByID( configDialog, &cid, &theControl );
	
	if ( theControl )
	{
		SetControlAction( theControl, SACB );
		SetControlReference( theControl, (SInt32) this );
	}
	
	SetConfigDialogValue( kSASignatureSlider, kSAChooseFontSliderID, fontSize );
	
	if ( trackInfoEnabled )
	{
		EnableConfigDialogControl( kSASignatureCheckbox, kSAShowAlbumCheckID );
		EnableConfigDialogControl( kSASignatureSlider, kSAChooseFontSliderID );
	}
	else
	{
		DisableConfigDialogControl( kSASignatureCheckbox, kSAShowAlbumCheckID );
		DisableConfigDialogControl( kSASignatureSlider, kSAChooseFontSliderID );
	}
	
	renderData = saveren;
}


void			ITSpectrumAnalyserPlugIn::ConfigDialogClose()
{
	// dialog is closing. Since prefs are "live", all we need to do is to remember the position of the window
	// and mark the local window ref as NULL (important)
	
	GetWindowBounds( configDialog, kWindowGlobalPortRgn, &configDialogPosition );
	configDialog = NULL;
}


SInt32			ITSpectrumAnalyserPlugIn::GetConfigDialogValue( const OSType sig, const UInt32 controlID )
{
	ControlID   cid;
	ControlRef  theControl;
	SInt32		v = 0;
	
	cid.signature = sig;
	cid.id = controlID;
	GetControlByID( configDialog, &cid, &theControl );
	
	if ( theControl )
		v = GetControl32BitValue( theControl );
		
	return v;
}


void			ITSpectrumAnalyserPlugIn::SetConfigDialogValue( const OSType sig, const UInt32 controlID, const SInt32 value )
{
	if ( configDialog )
	{
		ITPortSaver ps( GetWindowPort( configDialog ));
		
		if ( sig == kSASignatureEditField ||
			 sig == kSASignatureStaticText )
		{
			// convert number to string first
			
			Str255 str;
			
			NumToString( value, str );
			
			if ( sig == kSASignatureEditField )
				SetConfigDialogEditText( controlID, str );
			else
				SetConfigDialogStaticText( controlID, str );
		}
		else
		{
			ControlID   cid;
			ControlRef  theControl;
		
			cid.signature = sig;
			cid.id = controlID;
			GetControlByID( configDialog, &cid, &theControl );
			
			if ( theControl )
				SetControl32BitValue( theControl, value );
		}
	}
}


void			ITSpectrumAnalyserPlugIn::SetConfigDialogStaticText( const UInt32 controlID, const Str255 theText )
{
	if ( configDialog )
	{
		ITPortSaver ps( GetWindowPort( configDialog ));
		
		ControlID   cid;
		ControlRef  theControl;
		
		cid.signature = kSASignatureStaticText;
		cid.id = controlID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			SetControlData( theControl, kControlEntireControl, kControlStaticTextTextTag, theText[0], &theText[1] );
			HIViewSetNeedsDisplay( theControl, true );  // for the benefit of 10.2
		}
	}
}


void			ITSpectrumAnalyserPlugIn::SetConfigDialogEditText( const UInt32 controlID, const Str255 theText )
{
	if ( configDialog )
	{
		ITPortSaver ps( GetWindowPort( configDialog ));
		
		ControlID   cid;
		ControlRef  theControl;
		
		cid.signature = kSASignatureEditField;
		cid.id = controlID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			SetControlData( theControl, kControlEntireControl, kControlEditTextTextTag, theText[0], &theText[1] );
		}
	}
}


void			ITSpectrumAnalyserPlugIn::SetElementColourFromControl( ControlRef theControl, const UInt32 controlID )
{
	HIViewColourPopUpControl*  popView;
	
	popView = (HIViewColourPopUpControl*) GetControlReference( theControl );
	
	if ( popView && display )
	{
		RGBColor	rgb;
		
		rgb = popView->GetCurrentColour();
	
		switch( controlID )
		{
			case kSABarColourBevelID:
				ITBarDisplay::SetFixedColour( rgb, kBarColour );
				break;
				
			case kSAAltColourBevelID:
				ITBarDisplay::SetFixedColour( rgb, kAltBarColour );
				break;
				
			case kSAPeakColourBevelID:
				ITBarDisplay::SetFixedColour( rgb, kPeakColour );
				break;
				
			case kSAVUBarColourBevelID:
				ITBarDisplay::SetFixedColour( rgb, kVUBarColour );
				break;
				
			case kSAVUAltColourBevelID:
				ITBarDisplay::SetFixedColour( rgb, kVUAltBarColour );
				break;
				
			case kSAVUPeakColourBevelID:
				ITBarDisplay::SetFixedColour( rgb, kVUPeakColour );
				break;
		}
		
		ITPortSaver ps( destPort );
		display->UpdateColours();
	}
}


void			ITSpectrumAnalyserPlugIn::SetConfigDialogControlColour( const UInt32 controlID, const RGBColor& theColour )
{
	if ( configDialog )
	{
		ITPortSaver ps( GetWindowPort( configDialog ));
		
		ControlID   cid;
		ControlRef  theControl;
		
		cid.signature = kSASignatureColourPopUp;
		cid.id = controlID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			HIViewColourPopUpControl*  popView;
		
			popView = (HIViewColourPopUpControl*) GetControlReference( theControl );
		
			if ( popView )
				popView->UpdateWithNewColour( theColour );
		}
	}
}


void			ITSpectrumAnalyserPlugIn::EnableConfigDialogControl( const OSType sig, const UInt32 controlID )
{
	if ( configDialog )
	{
		ITPortSaver ps( GetWindowPort( configDialog ));
		
		ControlID   cid;
		ControlRef  theControl;
		
		cid.signature = sig;
		cid.id = controlID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
			EnableControl( theControl );
	}
}


void			ITSpectrumAnalyserPlugIn::DisableConfigDialogControl( const OSType sig, const UInt32 controlID )
{
	if ( configDialog )
	{
		ITPortSaver ps( GetWindowPort( configDialog ));
		
		ControlID   cid;
		ControlRef  theControl;
		
		cid.signature = sig;
		cid.id = controlID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
			DisableControl( theControl );
	}
}




void			ITSpectrumAnalyserPlugIn::ConfigDialogActivity( ControlRef hitControl, const UInt32 controlID )
{
	SInt32  newValue = 0;
	
	if ( hitControl )
		newValue = GetControl32BitValue( hitControl );
	
	switch( controlID )
	{
		case kSATabsControlID:
			SwitchConfigTabs( newValue );
			break;
			
		case kSALogLinRadioID:
			SwitchLogLin( newValue );
			break;
			
		case kSAShowPeakCheckID:
			SwitchPeakOnOff( newValue );
			break;
			
		case kSAUnlitSegmentsCheckID:
			ShowHideUnlitSegments( newValue );
			break;
			
		case kSABarDecaySliderID:
			SetBarDecayTime( newValue );
			break;
	
		case kSAPeakHoldSliderID:
			SetPeakHoldTime( newValue );
			break;
	
		case kSAPeakDecaySliderID:
			SetPeakDecayTime( newValue );
			break;
			
		case kSAShowVUCheckID:
			ShowHideVUMeters( newValue );
			break;
	
		case kSABorderCheckID:
			ShowHideBorder( newValue );
			break;
			
		case kSAFlipLeftCheckID:
			SetFlipLeftChannel( newValue );
			break;
			
		case kSAFlipRightCheckID:
			SetFlipRightChannel( newValue );
			break;
			
		case kSALayoutRadioID:
			SetLayout( newValue );
			break;
			
		case kSAFitWindowRadioID:
			SetWindowFit( newValue );
			break;
			
		case kSABarColourBevelID:
		case kSAAltColourBevelID:
		case kSAPeakColourBevelID:
		case kSAVUBarColourBevelID:
		case kSAVUAltColourBevelID:
		case kSAVUPeakColourBevelID:
			SetElementColourFromControl( hitControl, controlID );
			break;
			
		case kSACPalettePopUpID:
			SwitchColourPalette( newValue );
			break;
			
		case kSAShowTrackInfoCheckID:
			trackInfoEnabled = newValue;
			trackTime = ReadIntegerPreference( CFSTR("ITSAValueTrackInfoSeconds"), 10 );
			TrackInfoChanged( NULL );
			if ( trackInfoEnabled )
			{
				EnableConfigDialogControl( kSASignatureCheckbox, kSAShowAlbumCheckID );
				EnableConfigDialogControl( kSASignatureSlider, kSAChooseFontSliderID );
				EnableConfigDialogControl( kSASignatureCheckbox, kSAKeepTrackInfoVisibleCheck );
			}
			else
			{
				DisableConfigDialogControl( kSASignatureCheckbox, kSAShowAlbumCheckID );
				DisableConfigDialogControl( kSASignatureSlider, kSAChooseFontSliderID );
				DisableConfigDialogControl( kSASignatureCheckbox, kSAKeepTrackInfoVisibleCheck );
			}
			break;
			
		case kSAShowAlbumCheckID:
			showAlbumInfo = newValue;
			TrackInfoChanged( NULL );
			break;
			
		case kSAKeepTrackInfoVisibleCheck:
			if ( newValue )
				trackTime = 10000;  // stay up a loooong time
			else
				trackTime = ReadIntegerPreference( CFSTR("ITSAValueTrackInfoSeconds"), 10 );
			TrackInfoChanged( NULL );
			break;
			
		case kSACModePopUpID:
			SwitchColourMode( newValue );
			break;
			
		case kSASpectrumChannelsSlider:
			SetChannels( newValue );
			break;
			
		case kSAChooseFontSliderID:
			SetFontSize( newValue );
			break;
			
		case kSADecayOptionRadioID:
			ITBarGraph::expDecay = ( newValue == 1 );
			break;
			
		case kSAMeterScalesCheckID:
			SetScalesVisible( newValue );
			break;
			
		case kSARandomiseColoursCheckID:
			randColours = newValue;
			break;
			
		default:
			break;
	}
}


void			ITSpectrumAnalyserPlugIn::SwitchConfigTabs( SInt32 newValue )
{
	if ( configDialog )
	{
		//ITPortSaver ps( GetWindowPort( configDialog ));
		
		SInt16  paneIDs[] = { kSALayoutPanelID, kSAMetersPanelID, kSAColoursPanelID, kSAAboutPanelID };
		
		for( int i = 0; i < 4; i++ )
		{
			ControlRef		uPane;
			ControlID		uPaneID;
			
			uPaneID.signature = 'LEDA';
			uPaneID.id = paneIDs[i];
			
			GetControlByID( configDialog, &uPaneID, &uPane );
		
			if ( i == ( newValue - 1 ))
				SetControlVisibility( uPane, true, true );
			else
				SetControlVisibility( uPane, false, true );

		}
		
		curTab = newValue;
		SetConfigDialogValue( kSATabsControlSignature, kSATabsControlID, curTab );
		
		// for 10.2, need to force tab to be redrawn
		
		long	sysVersion;
		OSErr   err;
		
		 err = Gestalt( gestaltSystemVersion, &sysVersion );
		
		if ( sysVersion < 0x00001030 )
		{
			ControlRef  theControl;
			ControlID   cid;
			
			cid.signature = kSATabsControlSignature;
			cid.id = kSATabsControlID;
			
			if ( GetControlByID( configDialog, &cid, &theControl ) == noErr )
				HIViewSetNeedsDisplay( theControl, true );
		}
	}
}


void			ITSpectrumAnalyserPlugIn::SwitchLogLin( SInt32 newValue )
{
	if ( display )
	{
		ITPortSaver ps( destPort );
		
		display->SetMeterType( newValue );
		display->InvalMeterAreas();
	}
	SetConfigDialogValue( kSASignatureRadio, kSALogLinRadioID, newValue);
}


void			ITSpectrumAnalyserPlugIn::SwitchPeakOnOff( SInt32 newValue )
{
	ITPortSaver ps( destPort );

	if ( display )
		display->SetPeakEnabled( newValue );
		
	SetConfigDialogValue( kSASignatureCheckbox, kSAShowPeakCheckID, newValue );
}


void			ITSpectrumAnalyserPlugIn::ShowHideUnlitSegments( SInt32 newValue )
{
	showUnlit = ( newValue == 0 )? false : true;
	
	if ( newValue )
		ITBarGraph::backgroundRGB = RGB_VERY_DARK_GRAY;
	else
		ITBarGraph::backgroundRGB = RGB_BLACK;
		
	Update();
	SetConfigDialogValue( kSASignatureCheckbox, kSAUnlitSegmentsCheckID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SetBarDecayTime( SInt32 newValue )
{
	if ( display )
		display->SetDecayTime( kMainBar, newValue );
	
	Str63  vs,  s = "\p#1 ms";
	
	NumToString(16 * newValue, vs );
	SubstituteString( s, "#1", vs );
	SetConfigDialogStaticText( kSABarDecayStaticTextID, s );
	
	SetConfigDialogValue( kSASignatureSlider, kSABarDecaySliderID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SetPeakDecayTime( SInt32 newValue )
{
	if ( display )
		display->SetDecayTime( kPeakDecay, newValue );

	Str63  vs,  s = "\p#2 ms";
	
	NumToString( 16 * newValue, vs );
	SubstituteString( s, "#2", vs );
	SetConfigDialogStaticText( kSAPeakDecayStaticTextID, s );

	SetConfigDialogValue( kSASignatureSlider, kSAPeakDecaySliderID, newValue );
}

void			ITSpectrumAnalyserPlugIn::SetPeakHoldTime( SInt32 newValue )
{
	if ( display )
		display->SetDecayTime( kPeakHold, newValue );

	Str63  vs,  s = "\p#3 ms";
	
	NumToString( newValue * 16, vs );
	SubstituteString( s, "#3", vs );
	SetConfigDialogStaticText( kSAPeakHoldStaticTextID, s );

	SetConfigDialogValue( kSASignatureSlider, kSAPeakHoldSliderID, newValue );
}



void			ITSpectrumAnalyserPlugIn::ShowHideBorder( SInt32 newValue )
{
	if ( display )
	{
		ITPortSaver ps( destPort );

		display->EnableBorder((bool) newValue );
		
		Rect r;
		
		display->GetBounds( &r );
		InsetRect( &r, -3, -3 );
		InvalWindowRect( GetWindowFromPort( destPort ), &r );
	}
	
	SetConfigDialogValue( kSASignatureCheckbox, kSABorderCheckID, newValue );
}


void			ITSpectrumAnalyserPlugIn::ShowHideVUMeters( SInt32 newValue )
{
	bool vis = ITBarDisplay::VUMetersVisible();
	
	if ( vis != newValue )
	{
		ITBarDisplay::EnableVUMeters( newValue );
		
		DisposeGraphics();
		InitGraphics();
	}
	SetConfigDialogValue( kSASignatureCheckbox, kSAShowVUCheckID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SetFlipLeftChannel( SInt32 newValue )
{
	ITPortSaver ps( destPort );
	
	if ( display )
		display->SetChannelFlip( kLeftChannel, newValue );
	SetConfigDialogValue( kSASignatureCheckbox, kSAFlipLeftCheckID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SetFlipRightChannel( SInt32 newValue )
{
	ITPortSaver ps( destPort );

	if ( display )
		display->SetChannelFlip( kRightChannel, newValue );
	SetConfigDialogValue( kSASignatureCheckbox, kSAFlipRightCheckID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SetLayout( SInt32 newValue )
{
	Layout lays[] = { kLayoutSideBySide, kLayoutBackToBack, kLayoutAnalogueVUMeters };
	Layout oldLayout, newLayout;
	
	newLayout = lays[newValue - 1];
	oldLayout = ITBarDisplay::GetLayout() & 3;
	
	if ( newLayout != oldLayout )
	{
		oldLayout = ITBarDisplay::GetLayout() & ~3;
		
		ITBarDisplay::SetLayout( newLayout | oldLayout );
		
		switch ( newLayout )
		{
			case kLayoutSideBySide:
				ITBarDisplay::SetSpectrumSegments( 24 );
				break;
				
			case kLayoutBackToBack:
				ITBarDisplay::SetSpectrumSegments( 48 );
				break;
				
			case kLayoutAnalogueVUMeters:
				ITBarDisplay::SetSpectrumSegments( 10 );
				SetScalesVisible( false );
				break;
		}
	
		DisposeGraphics();
		InitGraphics();
		
	}
	SetConfigDialogValue( kSASignatureRadio, kSALayoutRadioID, newValue );
	
	switch ( newLayout )
	{
		case kLayoutSideBySide:
			EnableConfigDialogControl( kSASignatureSlider, kSASpectrumChannelsSlider );
			EnableConfigDialogControl( kSASignatureStaticText, kSAFreqBandsStaticTextID );
			EnableConfigDialogControl( kSASignatureStaticText, kSAFreqBandsLabelStaticTextID );
			EnableConfigDialogControl( kSASignatureCheckbox, kSAMeterScalesCheckID );
			EnableConfigDialogControl( kSASignatureCheckbox, kSAFlipLeftCheckID );
			EnableConfigDialogControl( kSASignatureCheckbox, kSAFlipRightCheckID );
			break;
			
		case kLayoutBackToBack:
			EnableConfigDialogControl( kSASignatureSlider, kSASpectrumChannelsSlider );
			EnableConfigDialogControl( kSASignatureStaticText, kSAFreqBandsStaticTextID );
			EnableConfigDialogControl( kSASignatureStaticText, kSAFreqBandsLabelStaticTextID );
			EnableConfigDialogControl( kSASignatureCheckbox, kSAMeterScalesCheckID );
			EnableConfigDialogControl( kSASignatureCheckbox, kSAFlipLeftCheckID );
			EnableConfigDialogControl( kSASignatureCheckbox, kSAFlipRightCheckID );
			break;
			
		case kLayoutAnalogueVUMeters:
			DisableConfigDialogControl( kSASignatureSlider, kSASpectrumChannelsSlider );
			DisableConfigDialogControl( kSASignatureStaticText, kSAFreqBandsStaticTextID );
			DisableConfigDialogControl( kSASignatureStaticText, kSAFreqBandsLabelStaticTextID );
			DisableConfigDialogControl( kSASignatureCheckbox, kSAMeterScalesCheckID );
			DisableConfigDialogControl( kSASignatureCheckbox, kSAFlipLeftCheckID );
			DisableConfigDialogControl( kSASignatureCheckbox, kSAFlipRightCheckID );
			break;
	}
}


void			ITSpectrumAnalyserPlugIn::SetWindowFit( SInt32 newValue )
{
	// newvalue can be 1, 2 or 3, corresponding to fit to window/large/medium sized respectively.
	
	Layout newlay, lay = ITBarDisplay::GetLayout();
	
	newlay = lay;
	
	switch( newValue )
	{
		case 1:
			newlay &= ~( kLayoutFixedSize | kLayoutFixedSizeLarger );
			break;
			
		case 2:
			newlay |= ( kLayoutFixedSize | kLayoutFixedSizeLarger );
			break;
		
		case 3:
			newlay |= kLayoutFixedSize;
			newlay &= ~kLayoutFixedSizeLarger;
			break;
	}
		
	if ( lay != newlay )
	{
		ITBarDisplay::SetLayout( newlay );
	
		DisposeGraphics();
		InitGraphics();
	}
	SetConfigDialogValue( kSASignatureRadio, kSAFitWindowRadioID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SetScalesVisible( SInt32 newValue )
{
	if ( newValue != ITBarDisplay::IsScalesVisible())
	{
		ITBarDisplay::ShowHideScales( newValue );
		
		DisposeGraphics();
		InitGraphics();
	}
	
	SetConfigDialogValue( kSASignatureCheckbox, kSAMeterScalesCheckID, newValue );
}



void			ITSpectrumAnalyserPlugIn::SwitchColourPalette( SInt32 newValue )
{
	// instruct display and config dialog to change palettes
	
	SInt16  palID[] = { 0, 1081, 8, 0, 128, 129, 130, 131, 1032 };
	
	if ( newValue != paletteIndex )
	{
		paletteID = palID[ newValue ];
		paletteIndex = newValue;
		
		SInt16 saveRes = CurResFile();
		UseResFile( resFile );
		ITBarDisplay::SetPaletteID( paletteID );
		UseResFile( saveRes );
	}	
	// update all the dialog colour pop-ups so they are using the required palette
	
	if ( configDialog )
	{
		ControlID					cid;
		ControlRef					theControl;
		HIViewColourPopUpControl*   hic;
		SInt16						saveRes;
		
		// ensure tables can be loaded from resource file
		
		saveRes = CurResFile();
		UseResFile( resFile );
		
		cid.signature = kSASignatureColourPopUp;
		cid.id = kSABarColourBevelID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			hic = (HIViewColourPopUpControl*) GetControlReference( theControl );
			if ( hic )
				hic->SetColourTableID( paletteID );
		}
		cid.id = kSAAltColourBevelID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			hic = (HIViewColourPopUpControl*) GetControlReference( theControl );
			if ( hic )
				hic->SetColourTableID( paletteID );
		}
		cid.id = kSAPeakColourBevelID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			hic = (HIViewColourPopUpControl*) GetControlReference( theControl );
			if ( hic )
				hic->SetColourTableID( paletteID );
		}
		cid.id = kSAVUBarColourBevelID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			hic = (HIViewColourPopUpControl*) GetControlReference( theControl );
			if ( hic )
				hic->SetColourTableID( paletteID );
		}
		cid.id = kSAVUAltColourBevelID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			hic = (HIViewColourPopUpControl*) GetControlReference( theControl );
			if ( hic )
				hic->SetColourTableID( paletteID );
		}
		cid.id = kSAVUPeakColourBevelID;
		GetControlByID( configDialog, &cid, &theControl );
		
		if ( theControl )
		{
			hic = (HIViewColourPopUpControl*) GetControlReference( theControl );
			if ( hic )
				hic->SetColourTableID( paletteID );
		}
		
		UseResFile( saveRes );
	}
	
	SetConfigDialogValue( kSASignaturePopUpMenu, kSACPalettePopUpID, newValue );
}


void			ITSpectrumAnalyserPlugIn::SwitchColourMode( SInt32 newMode )
{
	ITBarDisplay::SetColourMode( newMode );
	
	if ( display )
		display->UpdateColours();
		
	switch( newMode )
	{
		case kFixedColours:
			EnableConfigDialogControl( kSASignatureGroupBox, kSAMeterElementsGroupID );
			
			EnableConfigDialogControl( kSASignatureColourPopUp, kSABarColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAPeakColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAVUBarColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAVUPeakColourBevelID );
			DisableConfigDialogControl( kSASignatureColourPopUp, kSAAltColourBevelID );
			DisableConfigDialogControl( kSASignatureColourPopUp, kSAVUAltColourBevelID );
			break;
			
		case kGraduatedColours:
		case kGraduatedToMax:
			EnableConfigDialogControl( kSASignatureGroupBox, kSAMeterElementsGroupID );
			
			EnableConfigDialogControl( kSASignatureColourPopUp, kSABarColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAPeakColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAVUBarColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAVUPeakColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAAltColourBevelID );
			EnableConfigDialogControl( kSASignatureColourPopUp, kSAVUAltColourBevelID );
			break;
			
		case kAnimatedColours:
			DisableConfigDialogControl( kSASignatureGroupBox, kSAMeterElementsGroupID );

			SInt16 saveRes = CurResFile();
			UseResFile( resFile );
			ITBarDisplay::SetPaletteID( paletteID );
			UseResFile( saveRes );
			break;
	}
	
	SetConfigDialogValue( kSASignaturePopUpMenu, kSACModePopUpID, newMode );
}





void			ITSpectrumAnalyserPlugIn::SetChannels( SInt32 newValue )
{
	if ( destPort )
	{
		newValue = MAX( 10, MIN( 31, newValue ));
		
		if ( newValue != spectrumChannels )
		{
			spectrumChannels = newValue;
		
			ITPortSaver ps( destPort );

			DisposeGraphics();
			InitGraphics();
			
			Render();
			QDFlushPortBuffer( destPort, NULL );
		}
		
		SetConfigDialogValue( kSASignatureSlider, kSASpectrumChannelsSlider, newValue );
		SetConfigDialogValue( kSASignatureStaticText, kSAFreqBandsStaticTextID, newValue );
	}
}


void			ITSpectrumAnalyserPlugIn::SetFontSize( SInt32 newValue )
{
	if ( trackInfoEnabled )
	{
		if ( newValue != fontSize )
		{
			fontSize = newValue;
			TrackInfoChanged( NULL );
		}
		SetConfigDialogValue( kSASignatureSlider, kSAChooseFontSliderID, newValue );
	}
}



void			ITSpectrumAnalyserPlugIn::SavePreferences()
{
	// save all settings to prefs
	
	SaveIntegerPreference( CFSTR("ITSAValueBarDecayRate"), ITBarDisplay::GetDecayTime( kMainBar )); 
	SaveIntegerPreference( CFSTR("ITSAValuePeakDecayRate"), ITBarDisplay::GetDecayTime( kPeakDecay )); 
	SaveIntegerPreference( CFSTR("ITSAValuePeakHoldTime"), ITBarDisplay::GetDecayTime( kPeakHold )); 
	SaveIntegerPreference( CFSTR("ITSAValueMeterType"), ITBarDisplay::GetMeterType());
	SaveIntegerPreference( CFSTR("ITSAValueDisplayLayout"), ITBarDisplay::GetLayout());
	SaveBooleanPreference( CFSTR("ITSAValueShowBorder"), ITBarDisplay::IsBorderEnabled());
	SaveBooleanPreference( CFSTR("ITSAValueShowVUMeters"), ITBarDisplay::VUMetersVisible());
	SaveBooleanPreference( CFSTR("ITSAValueShowPeakIndicator"), ITBarDisplay::IsPeakEnabled());
	SaveBooleanPreference( CFSTR("ITSAValueShowUnlitSegments"), showUnlit );
	SaveBooleanPreference( CFSTR("ITSAValueExponentialDecay"), ITBarGraph::expDecay );
	SaveBooleanPreference( CFSTR("ITSAValueLeftChannelFlip"), ITBarDisplay::IsLeftChannelFlipped());
	SaveBooleanPreference( CFSTR("ITSAValueRightChannelFlip"), ITBarDisplay::IsRightChannelFlipped());
	SaveBooleanPreference( CFSTR("ITSAValueShowMeterScales"), ITBarDisplay::IsScalesVisible());
	SaveBooleanPreference( CFSTR("ITSAValueRandomiseColours"), randColours );
	SaveRectPreference( CFSTR("ITSAValueSettingsDialogPosition"), configDialogPosition );
	
	SaveIntegerPreference( CFSTR("ITSAValueSpectrumChannels"), spectrumChannels );
	SaveIntegerPreference( CFSTR("ITSAValueSpectrumSegments"), ITBarDisplay::GetSpectrumSegments());
	SaveIntegerPreference( CFSTR("ITSAValueVUSegments"), ITBarDisplay::GetVUSegments());
	
	SaveRGBColorPreference( CFSTR("ITSAValueSpectrumBarColour"), ITBarDisplay::GetFixedColour( kBarColour ));
	SaveRGBColorPreference( CFSTR("ITSAValueSpectrumAltColour"), ITBarDisplay::GetFixedColour( kAltBarColour ));
	SaveRGBColorPreference( CFSTR("ITSAValueSpectrumPeakColour"), ITBarDisplay::GetFixedColour( kPeakColour ));
	SaveRGBColorPreference( CFSTR("ITSAValueVUBarColour"), ITBarDisplay::GetFixedColour( kVUBarColour ));
	SaveRGBColorPreference( CFSTR("ITSAValueVUAltColour"), ITBarDisplay::GetFixedColour( kVUAltBarColour ));
	SaveRGBColorPreference( CFSTR("ITSAValueVUPeakColour"), ITBarDisplay::GetFixedColour( kVUPeakColour ));
	SaveIntegerPreference( CFSTR("ITSAValueColourMode"), ITBarDisplay::GetColourMode());
	SaveIntegerPreference( CFSTR("ITSAValueColourPalette"), (paletteIndex == 0)? 1 : paletteIndex );
	SaveBooleanPreference( CFSTR("ITSAValueShowTrackInfo"), trackInfoEnabled );
	SaveBooleanPreference( CFSTR("ITSAValueShowAlbumInfo"), showAlbumInfo );
	//SaveIntegerPreference( CFSTR("ITSAValueTrackInfoSeconds"), trackTime );
	SaveIntegerPreference( CFSTR("ITSAValueTrackInfoFontSize"), fontSize );
	
	SaveRGBColorPreference( CFSTR("ITSAValueTrackInfoColour"), trackInfoColour );
	
	CFPreferencesAppSynchronize( kCFPreferencesCurrentApplication );
}


void			ITSpectrumAnalyserPlugIn::ReadPreferences()
{
	// restore all settings from prefs
	
	ITBarDisplay::SetDecayTime( kMainBar, ReadIntegerPreference( CFSTR("ITSAValueBarDecayRate"), 40 ));
	ITBarDisplay::SetDecayTime( kPeakDecay, ReadIntegerPreference( CFSTR("ITSAValuePeakDecayRate"), 40));
	ITBarDisplay::SetDecayTime( kPeakHold, ReadIntegerPreference( CFSTR("ITSAValuePeakHoldTime"), 40));
	ITBarDisplay::SetLayout( ReadIntegerPreference( CFSTR("ITSAValueDisplayLayout"), kLayoutSideBySide | kLayoutFixedSize ));
	ITBarDisplay::SetMeterType( ReadIntegerPreference( CFSTR("ITSAValueMeterType"), kLogarithmic ));
	ITBarDisplay::EnableVUMeters( ReadBooleanPreference( CFSTR("ITSAValueShowVUMeters"), true ));
	ITBarDisplay::SetPeakEnabled( ReadBooleanPreference( CFSTR("ITSAValueShowPeakIndicator"), true ));
	ITBarDisplay::SetLeftFlip( ReadBooleanPreference( CFSTR("ITSAValueLeftChannelFlip"), false ));
	ITBarDisplay::SetRightFlip( ReadBooleanPreference( CFSTR("ITSAValueRightChannelFlip"), false ));
	ITBarDisplay::SetBorder( ReadBooleanPreference( CFSTR("ITSAValueShowBorder"), true ));
	
	ShowHideUnlitSegments( showUnlit = ReadBooleanPreference( CFSTR("ITSAValueShowUnlitSegments"), true ));
	ITBarGraph::expDecay = ReadBooleanPreference( CFSTR("ITSAValueExponentialDecay"), false );
	ITBarDisplay::ShowHideScales( ReadBooleanPreference( CFSTR("ITSAValueShowMeterScales"), false ));

	ReadRectPreference( CFSTR("ITSAValueSettingsDialogPosition"), &configDialogPosition );
	
	// thes prefs have no interface, but can be edited in the plist
	
	spectrumChannels = ReadIntegerPreference( CFSTR("ITSAValueSpectrumChannels"), 18 );
	ITBarDisplay::SetSpectrumSegments( ReadIntegerPreference( CFSTR("ITSAValueSpectrumSegments"), 24 ));
	//ITBarDisplay::SetVUSegments( ReadIntegerPreference( CFSTR("ITSAValueVUSegments"), 54 ));
	
	RGBColor	tc;
	
	tc = ITBarDisplay::GetFixedColour( kBarColour );
	ReadRGBColorPreference( CFSTR("ITSAValueSpectrumBarColour"), &tc );
	ITBarDisplay::SetFixedColour( tc, kBarColour );

	tc = ITBarDisplay::GetFixedColour( kAltBarColour );
	ReadRGBColorPreference( CFSTR("ITSAValueSpectrumAltColour"), &tc );
	ITBarDisplay::SetFixedColour( tc, kAltBarColour );

	tc = ITBarDisplay::GetFixedColour( kPeakColour );
	ReadRGBColorPreference( CFSTR("ITSAValueSpectrumPeakColour"), &tc );
	ITBarDisplay::SetFixedColour( tc, kPeakColour );

	tc = ITBarDisplay::GetFixedColour( kVUBarColour );
	ReadRGBColorPreference( CFSTR("ITSAValueVUBarColour"), &tc );
	ITBarDisplay::SetFixedColour( tc, kVUBarColour );

	tc = ITBarDisplay::GetFixedColour( kVUPeakColour );
	ReadRGBColorPreference( CFSTR("ITSAValueVUPeakColour"), &tc );
	ITBarDisplay::SetFixedColour( tc, kVUPeakColour );

	tc = ITBarDisplay::GetFixedColour( kVUAltBarColour );
	ReadRGBColorPreference( CFSTR("ITSAValueVUAltColour"), &tc );
	ITBarDisplay::SetFixedColour( tc, kVUAltBarColour );

	ITBarDisplay::SetColourMode( ReadIntegerPreference( CFSTR("ITSAValueColourMode"), kGraduatedColours ));
	trackInfoEnabled = ReadBooleanPreference( CFSTR("ITSAValueShowTrackInfo"), true );
	trackTime = ReadIntegerPreference( CFSTR("ITSAValueTrackInfoSeconds"), 10 );
	showAlbumInfo = ReadBooleanPreference( CFSTR("ITSAValueShowAlbumInfo"), false );
	fontSize = ReadIntegerPreference( CFSTR("ITSAValueTrackInfoFontSize"), 12 );
	
	ReadRGBColorPreference( CFSTR("ITSAValueTrackInfoColour"), &trackInfoColour );
	randColours = ReadBooleanPreference( CFSTR("ITSAValueRandomiseColours"), false );
}


void			ITSpectrumAnalyserPlugIn::SaveIntegerPreference( CFStringRef key,  const SInt32 theInt )
{
	CFNumberRef	value = CFNumberCreate( NULL, kCFNumberSInt32Type, &theInt );
	CFPreferencesSetAppValue( key, value, kCFPreferencesCurrentApplication );
	CFRelease( value );
}


void			ITSpectrumAnalyserPlugIn::SaveBooleanPreference( CFStringRef key, const bool theBool )
{
	CFBooleanRef	value = theBool? kCFBooleanTrue : kCFBooleanFalse;
	CFPreferencesSetAppValue( key, value, kCFPreferencesCurrentApplication );
}


SInt32			ITSpectrumAnalyserPlugIn::ReadIntegerPreference( CFStringRef key, const SInt32 missingDefault )
{
	CFNumberRef value;
	SInt32		temp = 0;
	
	value = (CFNumberRef) CFPreferencesCopyAppValue( key, kCFPreferencesCurrentApplication );
	
	if ( value )
	{
		CFNumberGetValue( value, kCFNumberSInt32Type, &temp );
		CFRelease( value );
	}
	else
		temp = missingDefault;
	
	return temp;
}


bool			ITSpectrumAnalyserPlugIn::ReadBooleanPreference( CFStringRef key, const bool missingDefault )
{
	CFBooleanRef	value = (CFBooleanRef) CFPreferencesCopyAppValue( key, kCFPreferencesCurrentApplication );
	
	if ( value )
		return CFBooleanGetValue( value );
	else
		return missingDefault;
}


void			ITSpectrumAnalyserPlugIn::SaveRectPreference( CFStringRef key,  const Rect& theRect )
{
	CFMutableStringRef		modkey;
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.left"));
	SaveIntegerPreference( modkey, theRect.left );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.top"));
	SaveIntegerPreference( modkey, theRect.top );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.right"));
	SaveIntegerPreference( modkey, theRect.right );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.bottom"));
	SaveIntegerPreference( modkey, theRect.bottom );
	CFRelease( modkey );
}


void			ITSpectrumAnalyserPlugIn::ReadRectPreference( CFStringRef key, Rect* theRect )
{
	CFMutableStringRef		modkey;
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.left"));
	theRect->left = ReadIntegerPreference( modkey, theRect->left );
	CFRelease( modkey );
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.top"));
	theRect->top = ReadIntegerPreference( modkey, theRect->top );
	CFRelease( modkey );
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.right"));
	theRect->right = ReadIntegerPreference( modkey, theRect->right );
	CFRelease( modkey );
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rect.bottom"));
	theRect->bottom = ReadIntegerPreference( modkey, theRect->bottom );
	CFRelease( modkey );
}


void			ITSpectrumAnalyserPlugIn::SaveRGBColorPreference( CFStringRef key,  const RGBColor& theColour )
{
	CFMutableStringRef		modkey;
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rgb.red"));
	SaveIntegerPreference( modkey, theColour.red );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rgb.green"));
	SaveIntegerPreference( modkey, theColour.green );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rgb.blue"));
	SaveIntegerPreference( modkey, theColour.blue );
	CFRelease( modkey );
}


void			ITSpectrumAnalyserPlugIn::ReadRGBColorPreference( CFStringRef key, RGBColor* theColour )
{
	CFMutableStringRef		modkey;
	
	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rgb.red"));
	theColour->red = ReadIntegerPreference( modkey, theColour->red );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rgb.green"));
	theColour->green = ReadIntegerPreference( modkey, theColour->green );
	CFRelease( modkey );

	modkey = CFStringCreateMutableCopy( NULL, 0, key );
	CFStringAppend( modkey, CFSTR(".rgb.blue"));
	theColour->blue = ReadIntegerPreference( modkey, theColour->blue );
	CFRelease( modkey );
}




void			ITSpectrumAnalyserPlugIn::DrawTrackInfo( const bool erase )
{
	if ( destPort )
	{
		CFMutableStringRef  str;
		ThemeDrawState		state = kThemeStateActive;
		Rect				textBox;

		ITPortSaver ps( destPort );
		
		RGBForeColor( &trackInfoColour );
		RGBBackColor( &RGB_BLACK );
		
		// if track fade timer > 0, calculate the fade colour and draw using that
		
		if ( ! IsFullScreen())
		{
			if (( trackInfoTimer == 0 ) && ( trackFadeTimer >= 0 ))
			{
				RGBColor	tc;
				
				tc = trackInfoColour;
				
				tc.red = ( tc.red * trackFadeTimer )/ kTrackInfoFadeTime;
				tc.green = ( tc.green * trackFadeTimer )/ kTrackInfoFadeTime;
				tc.blue = ( tc.blue * trackFadeTimer )/ kTrackInfoFadeTime;

				RGBForeColor( &tc );
			}
		}
		
		TextFont( 0 );
		TextSize( fontSize );
		TextFace( bold );
		
		if ( display )
			display->GetBounds( &textBox );
			
		textBox.left = destRect.left;
		textBox.right = destRect.right;
		textBox.top = textBox.bottom;
		textBox.bottom = destRect.bottom;

		InsetRect( &textBox, 3, 3 );
		EraseRect( &textBox );
		
		if ( ! erase )
		{
			str = CFStringCreateMutable( NULL, 0 );
			
			CFStringEncoding	enc = CFStringGetSystemEncoding();

			
			CFStringAppendPascalString( str, trackInfo.name, enc );
			CFStringAppend( str, CFSTR(" - "));
			CFStringAppendPascalString( str, trackInfo.artist, enc );
			
			if ( showAlbumInfo && trackInfo.album[0] > 0 )
			{
				CFStringAppend( str, CFSTR(" - "));
				CFStringAppendPascalString( str, trackInfo.album, enc );
			}
			
			// we might need two lines with a large font size, so we muct measure the text so that
			// we can position it centrally within the available area
			
			Point   ioBounds;
			SInt16  baseLine;
			
			ioBounds.h = textBox.right - textBox.left;
			GetThemeTextDimensions( str, kThemeCurrentPortFont, state, true, &ioBounds, &baseLine );
		
			// ioBounds.v has height of text. Make this into a new rect and centre it in textBox
			
			Rect tr = textBox;
			tr.bottom = tr.top + ioBounds.v;
			CentreRects( &textBox, &tr );
			
			// but make sure no bigger than original:
			
			SectRect( &tr, &textBox, &tr );
			
			DrawThemeTextBox( str, kThemeCurrentPortFont, state, true, &tr, teJustCenter, NULL );
			CFRelease( str );
		}
	}
}


void			ITSpectrumAnalyserPlugIn::TrackInfoCtl()
{
	if ( trackInfoEnabled )
	{
		if (( TickCount() > trackInfoTimer ) && ( trackInfoTimer != 0 ))
		{
			// display time is up, start to fade the text, but not in FS mode, as it's not buffered
			
			trackInfoTimer = 0;
			
			if ( IsFullScreen())
				DrawTrackInfo( true );
			else	
				trackFadeTimer = kTrackInfoFadeTime;	// 2 seconds approx
		}
		else
		{
			static UInt32   t = 0;
			
			if (( TickCount() > t ) && ( trackFadeTimer > 0 ))
			{
				t = TickCount() + 2;
			
				--trackFadeTimer;
				DrawTrackInfo();
			}
		}
	}
	
	if ( cmdString && ( cmdFadeTimer > 0 ))
	{
		static UInt32 t = 0;
		
		if ( TickCount() > t )
		{
			t = TickCount() + 2;
			
			DrawCmdString();
			--cmdFadeTimer;
		}
	}
}



void			ITSpectrumAnalyserPlugIn::FixupJaguarMenus()
{
	if ( configDialog )
	{
		ControlRef  theControl;
		ControlID   sid;
		
		sid.signature = kSASignaturePopUpMenu;
		sid.id = kSACPalettePopUpID;
		
		GetControlByID( configDialog, &sid, &theControl );
		
		if ( theControl )
		{
			MenuRef		mh;

			mh = GetControlPopupMenuHandle( theControl );
			
			if ( mh )
			{
				UInt16 n = CountMenuItems( mh );
			
				while( n )
					CheckMenuItem( mh, n--, false );
			}
		}
		
		sid.id = kSACModePopUpID;
		
		GetControlByID( configDialog, &sid, &theControl );
		
		if ( theControl )
		{
			MenuRef		mh;

			mh = GetControlPopupMenuHandle( theControl );
			
			if ( mh )
			{
				UInt16 n = CountMenuItems( mh );
			
				while( n )
					CheckMenuItem( mh, n--, false );
			}
		}
	}
}


static const char* cmdstrs[] = {  "track info OFF","track info ON",
								  "track info OFF", "track info ON (stays on)",
								  "album OFF","album ON",
								  "border OFF","border ON",
								  "left channel normal","left channel flipped",
								  "right channel normal","right channel flipped",
								  "VU meters OFF","VU meters ON",
								  "scales OFF","scales ON",
								  "log response", "linear response",
								  "linear decay", "exponential decay",
								  "peak indicators OFF","peak indicators ON",
								  "unlit segments OFF","unlit segments ON",
								  "back-to-back layout ON","side-by-side layout ON",
								  "fixed size ON", "fit to window ON",
								  "analogue meters ON",
								  "rainbow lorikeet", "crimson rosella" };
								  
static const char* cmdvstrs[] = { "font size: ", "channels: " };
								 
static const char cmdk[] = { 't', 'j', 'a', 'b', 'l', 'r', 'v', 'm', 'n', 'e', 'p', 'u', 's', 'f', 'x' };
static const char cmdv[] = { 'i', 'd', ',', '.' };								


void			ITSpectrumAnalyserPlugIn::SetCmdString( const UInt16 forCmdKey, const SInt16 stateInfo )
{
	// look up command for given keyboard shortcut
	
	if ( cmdString )
	{
		CFRelease( cmdString );
		cmdString = NULL;
	}
	
	// use fixed strings for now - later we can localize this if necessary
	
	if ( forCmdKey != 0 )
	{
		UniChar		c;
		UInt16		lc;
		
		if ( forCmdKey >= 'a' )
			lc =  forCmdKey;
		else
			lc =  forCmdKey - 'A' + 'a';
		
		c = (UniChar)( lc - 'a' + 'A' );	
		
		CFMutableStringRef  cfs = CFStringCreateMutable( NULL, 0 );
		
		CFStringAppendCharacters( cfs, &c, 1 );
		CFStringAppendCString( cfs, " - ", kCFStringEncodingMacRoman );

		for( int i = 0; i < 16; i++ )
		{
			if ( lc == cmdk[i] )
			{
				// found the key, which string is it?
				
				
				// look up string:
				
				int idx = ( i * 2 ) + stateInfo;
				
				CFStringAppendCString( cfs, cmdstrs[ idx ], kCFStringEncodingMacRoman );
				
				cmdString = cfs;
				cmdFadeTimer = kCmdInfoFadeTime; // 3 seconds
				return;
			}
		}
		
		// some keys need other treatment, try second table
		
		if ( forCmdKey == '<' || forCmdKey == ',' )
			lc = ',';
			
		if ( forCmdKey == '>' || forCmdKey == '.' )
			lc = '.';
		
		for ( int i = 0; i < 4; i++ )
		{
			if ( lc == cmdv[i] )
			{
				int		idx = ( i / 2 );
				Str15   nps;
				
				CFStringAppendCString( cfs, cmdvstrs[idx], kCFStringEncodingMacRoman );
				NumToString( stateInfo, nps );
				CFStringAppendPascalString( cfs, nps, kCFStringEncodingMacRoman );

				cmdString = cfs;
				cmdFadeTimer = kCmdInfoFadeTime; // 3 seconds
				return;
			}
		}
		
		// if here, no match, so discard cfs
		
		CFRelease( cfs );
	}
}


void			ITSpectrumAnalyserPlugIn::DrawCmdString()
{
	if ( destPort && cmdString && ( cmdFadeTimer > 0 ))
	{
		ITPortSaver ps( destPort );
		
		RGBColor	tc;
		
		tc = RGB_WHITE;
				
		tc.red = ( tc.red * cmdFadeTimer )/ kCmdInfoFadeTime;
		tc.green = ( tc.green * cmdFadeTimer )/ kCmdInfoFadeTime;
		tc.blue = ( tc.blue * cmdFadeTimer )/ kCmdInfoFadeTime;

		RGBForeColor( &tc );
		RGBBackColor( &RGB_BLACK );
		
		EraseRect( &cmdStringRect );
		DrawThemeTextBox( cmdString, kThemeSmallSystemFont, kThemeStateActive, true, &cmdStringRect, teJustLeft, NULL );
	}
}


void			ITSpectrumAnalyserPlugIn::ChooseNewRandomColours()
{
	// this will pick a set of random colours for the meter elements based on the currently set palette
	
	// what palette is in use?
	
	SInt16		pid = ITBarDisplay::GetPaletteID();
	SInt16		svRes;
	CTabHandle  palette;

	// load that palette, so we can pick colours from it
	
	svRes = CurResFile();
	UseResFile( resFile );
	
	palette = GetCTable( pid );
	
	if ( palette == NULL )
		palette = GetCTable( 8 );
		
	UseResFile( svRes );
	
	// OK, pick 'em
	
	RGBColor	rgb;
	SInt16		elem[] = { kBarColour, kPeakColour, kAltBarColour, kVUBarColour, kVUPeakColour, kVUAltBarColour };
	SInt16		ctrl[] = { kSABarColourBevelID, kSAPeakColourBevelID, kSAAltColourBevelID,
							kSAVUBarColourBevelID, kSAVUPeakColourBevelID, kSAVUAltColourBevelID };
	for( int i = 0; i < 6; i++ )
	{
		UInt16  k = (UInt16) Random() % (*palette)->ctSize;
		
		rgb = (*palette)->ctTable[k].rgb;
		
		ITBarDisplay::SetFixedColour( rgb, elem[i] );
		
		// if config dialog open, update the colour picker buttons immediately
		
		if ( configDialog )
			SetConfigDialogControlColour( ctrl[i], rgb );
	}
	
	display->UpdateColours();
	DisposeCTable( palette );
}



pascal OSStatus ITSpectrumAnalyserPlugIn::DialogEventHandler( EventHandlerCallRef inRef, EventRef inEvent, void* userData )
{
    OSStatus result = eventNotHandledErr;
	ITSpectrumAnalyserPlugIn* it = (ITSpectrumAnalyserPlugIn*) userData;
	
	switch( GetEventClass( inEvent ))
	{
		case kEventClassControl:
		{
			ControlID   controlID;
			ControlRef  control = NULL;
			
			GetEventParameter( inEvent, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &control );
			GetControlID( control, &controlID );
			
			if ( it )
				it->ConfigDialogActivity( control, controlID.id );
				
			result = noErr;
		}
		break;
		
		case kEventClassWindow:
			if ( it )
				it->ConfigDialogClose();
			break;
			
		case kEventClassTextInput:
		{
			EventRef	keyEvt;
			char		theKey;
			
			GetEventParameter( inEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(EventRef), NULL, &keyEvt );
			GetEventParameter( keyEvt, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &theKey );
			
			if ( it->DoKeyDownEvent( theKey, 0 ))
				result = noErr;
		}
		break;
	}


	return result;
}


pascal void		ITSpectrumAnalyserPlugIn::ControlCallback( ControlRef theControl, ControlPartCode partCode )
{
	ITSpectrumAnalyserPlugIn* sapi;
	
	sapi = (ITSpectrumAnalyserPlugIn*) GetControlReference( theControl );
	
	if ( sapi )
	{
		ControlID   controlID;
		OSStatus	err;
		
		err = GetControlID( theControl, &controlID );
		
		if ( err == noErr )
			sapi->ConfigDialogActivity( theControl, controlID.id );
	}
}


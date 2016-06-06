/*
 *  ITPlugIn.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Wed Mar 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "ITPlugIn.h"


static  ITPlugIn*		gPlugIn = NULL;


static const RGBColor RGB_BLACK = { 0, 0, 0 };
static const RGBColor RGB_WHITE = { 0xFFFF, 0xFFFF, 0xFFFF };


extern "C"
{
	OSStatus iTunesPluginMainMachO( OSType message, PluginMessageInfo *messageInfo, void *refCon );
}


OSStatus iTunesPluginMainMachO( OSType message, PluginMessageInfo *messageInfo, void *refCon )
{
	OSStatus		status;
	
	switch( message )
	{
		case kPluginInitMessage:
			gPlugIn = NewPlugIn();
			if ( gPlugIn )
				status = ITPlugIn::RegisterWithApplication( messageInfo );
			else
				status = memFullErr;
			break;
			
		case kPluginCleanupMessage:
			delete gPlugIn;
			status = noErr;
			break;
			
		default:
			status = unimpErr;
			break;
	}
	
	return status;
}





static void MemClear( void* dest, SInt32 length );

static void MemClear( void* dest, SInt32 length )
{
	register unsigned char	*ptr;

	ptr = (unsigned char*) dest;
	
	while (length-- > 0)
		*ptr++ = 0;
}





ITPlugIn::ITPlugIn()
{
	destPort = NULL;
	playing = false;
	renderTimeStampID = 0;
	appCookie = NULL;
	appProc = NULL;
	MemClear( &trackInfo, sizeof( trackInfo ));
	MemClear( &streamInfo, sizeof( streamInfo ));
	SetRect( &destRect, 0, 0, 0, 0 );
	renderData = NULL;
	destOptions = 0;
	configDialog = NULL;
}


ITPlugIn::~ITPlugIn()
{
}


OSStatus			ITPlugIn::RegisterWithApplication( PluginMessageInfo *messageInfo )
{
	OSStatus			status;
	PlayerMessageInfo	playerMessageInfo;
		
	MemClear(&playerMessageInfo.u.registerVisualPluginMessage,sizeof(playerMessageInfo.u.registerVisualPluginMessage));
	gPlugIn->GetPlugInName( playerMessageInfo.u.registerVisualPluginMessage.name );

	SetNumVersion(&playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kTVisualPluginMajorVersion, kTVisualPluginMinorVersion, kTVisualPluginReleaseStage, kTVisualPluginNonFinalRelease );

	playerMessageInfo.u.registerVisualPluginMessage.options					= kVisualWantsIdleMessages | kVisualWantsConfigure;
	playerMessageInfo.u.registerVisualPluginMessage.handler					= (VisualPluginProcPtr) ITunesDispatcher;
	playerMessageInfo.u.registerVisualPluginMessage.registerRefCon			= gPlugIn;
	playerMessageInfo.u.registerVisualPluginMessage.creator					= kTVisualPluginCreator;
	
	playerMessageInfo.u.registerVisualPluginMessage.timeBetweenDataInMS		= 0xFFFFFFFF; // 16 milliseconds = 1 Tick,0xFFFFFFFF = Often as possible.
	playerMessageInfo.u.registerVisualPluginMessage.numWaveformChannels		= 0;
	playerMessageInfo.u.registerVisualPluginMessage.numSpectrumChannels		= 2;
	
	playerMessageInfo.u.registerVisualPluginMessage.minWidth				= 400;
	playerMessageInfo.u.registerVisualPluginMessage.minHeight				= 128;
	playerMessageInfo.u.registerVisualPluginMessage.maxWidth				= 32767;
	playerMessageInfo.u.registerVisualPluginMessage.maxHeight				= 32767;
	playerMessageInfo.u.registerVisualPluginMessage.minFullScreenBitDepth	= 0;
	playerMessageInfo.u.registerVisualPluginMessage.maxFullScreenBitDepth	= 0;
	playerMessageInfo.u.registerVisualPluginMessage.windowAlignmentInBytes	= 0;
	
	status = PlayerRegisterVisualPlugin( messageInfo->u.initMessage.appCookie, messageInfo->u.initMessage.appProc, &playerMessageInfo );
		
	return status;
}



OSStatus		ITPlugIn::ITunesDispatcher( OSType message, VisualPluginMessageInfo *messageInfo, void* refCon )
{
	OSStatus result = noErr;
	
	ITPlugIn* thePlug = (ITPlugIn*) refCon;
	
	// this method calls all the others. You just need to override them, don't arse about with this, it tends to get
	// a bit obfuscated.
	
	switch( message )
	{
		case kVisualPluginInitMessage:
			thePlug->appCookie = messageInfo->u.initMessage.appCookie;
			thePlug->appProc = messageInfo->u.initMessage.appProc;
			result = PlayerGetPluginFileSpec( thePlug->appCookie, thePlug->appProc, &thePlug->pluginFileSpec );
			// we've set up the essential data members - now call the Init method to do any custom initialisation
			thePlug->Initialise();
			break;
			
		case kVisualPluginCleanupMessage:
			thePlug->CleanUp();
			break;
			
		case kVisualPluginEnableMessage:
			thePlug->Enable();
			break;
			
		case kVisualPluginDisableMessage:
			thePlug->Disable();
			break;
			
		case kVisualPluginIdleMessage:
			thePlug->Idle();
			break;
			
		case kVisualPluginConfigureMessage:
			thePlug->DoConfigure();
			break;
			
		case kVisualPluginShowWindowMessage:
			thePlug->destOptions = messageInfo->u.showWindowMessage.options;
			thePlug->WindowChanged( messageInfo->u.showWindowMessage.port, &messageInfo->u.showWindowMessage.drawRect );
			thePlug->WindowShown();
			//thePlug->Render();
			thePlug->TrackInfoChanged( NULL );
			break;
			
		case kVisualPluginHideWindowMessage:
			thePlug->TrackInfoChanged( NULL );
			thePlug->StreamInfoChanged( NULL );
			thePlug->WindowChanged( NULL, NULL );
			thePlug->WindowHidden();
			break;
			
		case kVisualPluginSetWindowMessage:
			thePlug->destOptions = messageInfo->u.setWindowMessage.options;
			thePlug->WindowChanged( messageInfo->u.setWindowMessage.port, &messageInfo->u.setWindowMessage.drawRect );
			//thePlug->Render();
			break;
			
		case kVisualPluginRenderMessage:
			thePlug->renderTimeStampID = messageInfo->u.renderMessage.timeStampID;
			thePlug->renderData = messageInfo->u.renderMessage.renderData;
			//thePlug->RenderPreprocess();
			thePlug->Render();
			break;
			
		case kVisualPluginUpdateMessage:
			thePlug->Update();
			break;
			
		case kVisualPluginPlayMessage:
			thePlug->TrackInfoChanged( messageInfo->u.playMessage.trackInfo );
			thePlug->StreamInfoChanged( messageInfo->u.playMessage.streamInfo );
			thePlug->PlayStarting();
			break;
			
		case kVisualPluginStopMessage:
			thePlug->PlayStopping();
			break;
			
		case kVisualPluginChangeTrackMessage:
			thePlug->TrackInfoChanged( messageInfo->u.changeTrackMessage.trackInfo );
			thePlug->StreamInfoChanged( messageInfo->u.changeTrackMessage.streamInfo );
			break;
			
		case kVisualPluginSetPositionMessage:
			thePlug->TrackPositionChanged();
			break;
			
		case kVisualPluginPauseMessage:
			thePlug->PlayPaused();
			break;
			
		case kVisualPluginUnpauseMessage:
			thePlug->PlayUnpaused();
			break;
			
		case kVisualPluginEventMessage:
		{
			const EventRecord& evt = *messageInfo->u.eventMessage.event;
			result = thePlug->ITEventDispatcher( evt );
		}
		break;
			
		default:
			result = unimpErr;
			break;
	}   
	
	return result;
}

void			ITPlugIn::GetPlugInName( Str63 outName )
{
	Str63 t = "\pVisual Plug-in";
	
	BlockMoveData((Ptr) t, (Ptr) outName, t[0] + 1 );
}


void			ITPlugIn::TrackInfoChanged( ITTrackInfo* newTrackInfo )
{
	if ( newTrackInfo )
		trackInfo = *newTrackInfo;
	else
		MemClear( &trackInfo, sizeof( trackInfo ));
}



void			ITPlugIn::StreamInfoChanged( ITStreamInfo* newStreamInfo )
{
	if ( newStreamInfo )
		streamInfo = *newStreamInfo;
	else
		MemClear( &streamInfo, sizeof( streamInfo ));
}



void			ITPlugIn::PlayStarting()
{
	playing = true;
}


void			ITPlugIn::PlayStopping()
{
	playing = false;
	renderData = NULL;
}


void			ITPlugIn::WindowChanged( CGrafPtr newPort, Rect* newPortRect )
{
	bool			doAllocate = false;
	bool			doDeallocate = false;

	if ( newPort != NULL )
	{
		if ( destPort != NULL )
		{
			if ( !EqualRect( newPortRect, &destRect))
			{
				doDeallocate	= true;
				doAllocate		= true;
			}
		}
		else
		{
			doAllocate = true;
		}
	}
	else
	{
		doDeallocate = true;
	}
	
	destPort = newPort;

	if ( newPortRect != NULL )
		destRect = *newPortRect;

	if ( doDeallocate )
	{
		DisposeGraphics();
	
		if ( configDialog )
		{
			WindowRef   d = configDialog;
			
			ConfigDialogClose();
			DisposeWindow( d );
		}
	}
	
	if ( doAllocate )
		InitGraphics();
}


OSStatus		ITPlugIn::ITEventDispatcher( const EventRecord& theEvent )
{
	EventModifiers  modifiers;
	UInt16			key;
	Point			pt;
	OSStatus		result = noErr;
	
	modifiers = theEvent.modifiers;
	
	switch( theEvent.what )
	{
		case keyDown:
		case autoKey:
			key = theEvent.message & charCodeMask;
			if( ! DoKeyDownEvent( key, modifiers ))
				result = unimpErr;
			break;
			
		case mouseDown:
			pt = theEvent.where;
			DoMouseDownEvent( pt, modifiers );
			result = unimpErr;
			break;
	}
	
	return result;
}


void		ITPlugIn::OpenOrSelectConfigDialog()
{
	if ( configDialog == NULL )
	{
		NewConfigDialog();
		ConfigDialogOpen();
	}
	
	if ( configDialog )
	{
		ShowWindow( configDialog );
		SelectWindow( configDialog );
	}
}


bool		ITPlugIn::IsFullScreen()
{
	return (( destOptions & kWindowIsFullScreen ) != 0 );
}

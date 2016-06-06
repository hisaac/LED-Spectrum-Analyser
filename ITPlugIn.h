/*
 *  ITPlugIn.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Wed Mar 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include "iTunesVisualAPI.h"


// abstract base class for implementing an iTunes plug-in. Easier to use than Apple's sample code, since
// you can just subclass it and override the methods you want to use.

class ITPlugIn
{
public:
	ITPlugIn();
	virtual ~ITPlugIn();
	
	static OSStatus		RegisterWithApplication( PluginMessageInfo *messageInfo );
	static OSStatus		ITunesDispatcher( OSType message, VisualPluginMessageInfo *messageInfo, void *refCon );

	virtual void		GetPlugInName( Str63 outName );
	
protected:
	virtual void		Initialise(){};
	virtual void		CleanUp(){};
	virtual void		InitGraphics(){};
	virtual void		DisposeGraphics(){};
	
	virtual void		Enable(){};
	virtual void		Disable(){};
	
	virtual void		Idle(){};
	virtual void		DoConfigure(){ OpenOrSelectConfigDialog(); };
	virtual void		WindowShown(){};
	virtual void		WindowHidden(){};
	virtual void		WindowChanged( CGrafPtr newPort, Rect* newPortRect );
	
	virtual void		RenderPreprocess(){};
	virtual void		Render(){};
	virtual void		Update(){ Render(); };
	
	virtual void		PlayStarting();
	virtual void		PlayStopping();
	virtual void		PlayPaused(){};
	virtual void		PlayUnpaused(){};
	virtual void		TrackInfoChanged( ITTrackInfo* newTrackInfo );
	virtual void		StreamInfoChanged( ITStreamInfo* newStreamInfo );
	virtual void		TrackPositionChanged(){};
	virtual bool		IsPlaying(){ return playing; };
	bool				IsFullScreen();
	
	virtual OSStatus	ITEventDispatcher( const EventRecord& theEvent );
	
	virtual Boolean		DoKeyDownEvent( const UInt16 theKey, const EventModifiers modifiers ){ return false; };
	virtual void		DoMouseDownEvent( const Point mousePt, const EventModifiers modifiers ){};
	
	// config dialog, if requested
	
	virtual void		OpenOrSelectConfigDialog();
	virtual void		NewConfigDialog(){};
	virtual void		ConfigDialogOpen(){};		// set config dialog UI to match state
	virtual void		ConfigDialogClose(){};		// config dialog dismissed
	virtual void		ConfigDialogActivity( ControlRef hitControl, const UInt32 controlID ){};   // user changed something in config dialog

	// data members
	
	void*				appCookie;
	ITAppProcPtr		appProc;
	ITFileSpec			pluginFileSpec;
	ITTrackInfo			trackInfo;
	ITStreamInfo		streamInfo;
	CGrafPtr			destPort;
	Rect				destRect;
	RenderVisualData*	renderData;
	UInt32				renderTimeStampID;
	Boolean				playing;
	OptionBits			destOptions;
	WindowRef			configDialog;
};



enum
{
	kTVisualPluginCreator			 = 'hook',
	kTVisualPluginMajorVersion		 = 2,
	kTVisualPluginMinorVersion		 = 0,
	kTVisualPluginReleaseStage		 = finalStage,
	kTVisualPluginNonFinalRelease	 = 0
};


// factory function makes the desired kind of plug in

ITPlugIn*  NewPlugIn(); 



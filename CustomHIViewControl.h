/*
 *  CustomHIViewControl.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Fri Mar 05 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>

// HIView object to implement colour popup menu


class HIViewColourPopUpControl
{
public:
	HIViewColourPopUpControl( ControlRef theControl );
	virtual ~HIViewColourPopUpControl();
	
	virtual void	Initialise();
	
	virtual OSStatus    DrawEvent( EventRef inEvent );
	virtual void		Draw( const SInt16 partCode, const bool pressed = false );
	virtual OSStatus	HitTest( EventRef inEvent );
	virtual void		Hit( const SInt16 partCode );
	virtual void		Activate();
	virtual void		Deactivate();
	virtual OSStatus	Track( EventRef inEvent );
	virtual OSStatus	ViewUpdate( EventRef inEvent );
	
	
	virtual bool		IsActive();
	virtual bool		IsEnabled();
	CTabHandle			GetColourTable(){ return theColours; };
	void				SetColourTableID( const SInt16 tableID );
	
	void				UpdateWithNewColour( RGBColor newColour, const bool calcIndex = true );
	RGBColor			GetOriginalColour(){ return origColour; };
	RGBColor			GetCurrentColour(){ return selection; };
	SInt16				GetCurrentItemIndex(){ return index; }
	
	static void			SelfRegister();
	
protected:
	ControlRef		theControl;
	MenuRef			theMenu;
	CTabHandle		theColours;
	RGBColor		origColour;
	RGBColor		selection;
	SInt16			index;
	SInt16			cTabID;
};


OSStatus RegisterCustomHIView( CFStringRef classname, EventHandlerUPP hiCallbackProc );
OSStatus CustomViewHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );



#define		kMZColourPopUpClassName		"com.apptree.maczoop.cpopup"


enum
{
	kCustomPopupColourMenuID		= 10477,
	kDefaultSwatchWidth				= 20,
	kSmallSwatchWidth				= 13,
	k81StandardColoursClutID		= 1081

};


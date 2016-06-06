/*
 *  CustomHIViewControl.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Fri Mar 05 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "CustomHIViewControl.h"

static const RGBColor RGB_BLACK = { 0, 0, 0 };
static const RGBColor RGB_WHITE = { 0xFFFF, 0xFFFF, 0xFFFF };


static pascal void	ColourPopUpProc( short message, MenuHandle theMenu,
							 Rect *menuRect, Point hitPt, short *whichItem );


static void CalcCPMenuSize( MenuHandle theMenu, short chipsH, short chipsV );
static void CalcPopupMenuSize( Point hitPt, short item, Rect *mRect, short chipsH, short chipsV );
static void DrawCPMenu( MenuHandle theMenu, Rect *mBounds, short chipsH, short chipsV, short initItem, CTabHandle colours );
static void FindCPItem( Point mouse, Rect* mRect, short chipsH, short chipsV, MenuTrackingData* trackData );
static void HiliteCPItem( Rect* mRect, short chipsH, short chipsV, HiliteMenuItemData* hiliteData );




HIViewColourPopUpControl::HIViewColourPopUpControl( ControlRef hiControl )
{
	selection = origColour = RGB_BLACK;
	theControl = hiControl;
	theMenu = NULL;
	index = 0;
	cTabID = k81StandardColoursClutID;
	theColours = NULL;
}


HIViewColourPopUpControl::~HIViewColourPopUpControl()
{
	if ( theMenu )
		DisposeMenu( theMenu );
		
	if ( theColours )
		DisposeCTable( theColours );
}


	
void	HIViewColourPopUpControl::Initialise()
{
	// create the custom menu we wish to display
	
	MenuAttributes  attributes = 0;
	MenuDefSpec		inSpec;
	
	inSpec.defType = kMenuDefProcPtr;
	inSpec.u.defProc = NewMenuDefUPP( ColourPopUpProc );
	
	CreateCustomMenu( &inSpec, kCustomPopupColourMenuID, attributes, &theMenu );
	
	// stuff refCon so MDEF can find this object
	
	SetMenuItemRefCon( theMenu, 0, (UInt32) this );
	
	// load colour table
	
	theColours = GetCTable( cTabID );
	
	if ( theColours == NULL )
		theColours = GetCTable( 8 );	// use system colours if specified table unavailable
		
	// stuff control refcon too so user of control is able to interrogate this for colour
	
	SetControlReference( theControl, (SInt32) this );
}


bool	HIViewColourPopUpControl::IsActive()
{
	return IsControlActive( theControl );
}


bool	HIViewColourPopUpControl::IsEnabled()
{
	return IsControlEnabled( theControl );
}


OSStatus	HIViewColourPopUpControl::ViewUpdate( EventRef inEvent )
{
	HIViewSetNeedsDisplay( theControl, true );
	return noErr;
}


OSStatus	HIViewColourPopUpControl::DrawEvent( EventRef inEvent )
{
	OSStatus			err;
	ControlPartCode		part;
	
	
	err = GetEventParameter( inEvent, kEventParamControlPart, typeControlPartCode,
			NULL, sizeof( ControlPartCode ),NULL, &part ); 

	Draw( part );
	return err;
}
	

void		HIViewColourPopUpControl::Draw( const SInt16 partCode, const bool pressed )
{
	Rect				r;
	ThemeDrawingState   saveState;
	
	GetThemeDrawingState( &saveState );
	NormalizeThemeDrawingState();
	
	GetControlBounds( theControl, &r );
	OffsetRect( &r, -r.left, -r.top );

	ThemeButtonDrawInfo info;
	
	//EraseRect( &r );
	r.bottom -= 2;
	r.right -= 2;
	
	ControlPartCode part = GetControlHilite( theControl );

	info.state = ( IsActive() && IsEnabled())? (( part == 1 )? kThemeStatePressed : kThemeStateActive ) : kThemeStateInactive;
	info.value = 0;
	info.adornment = kThemeAdornmentNone;
	
	DrawThemeButton( &r, kThemePopupButton, &info, NULL, NULL, NULL, 0 );
		
	// draw a colour thingy on top. Note there are no metrics for components
	// of a popup button, so I'm having to set a value empirically here. This
	// might not look quite right in Aqua.
	
	InsetRect( &r, 6, 6 );
	r.right -= 20;
	r.bottom++;
	
	DrawThemeEditTextFrame( &r, ( IsActive() && IsEnabled())? kThemeStateActive : kThemeStateInactive );
	
	if ( IsEnabled())
	{
		RGBForeColor( &selection );
		PaintRect( &r );
	}
	SetThemeDrawingState( saveState, true );
}


OSStatus  HIViewColourPopUpControl::HitTest( EventRef inEvent )
{
	OSStatus				err;
	HIRect					bounds;
	HIPoint					where;
	ControlPartCode			part;

	// Extract the mouse location
	err = GetEventParameter( inEvent, kEventParamMouseLocation, typeHIPoint,
			NULL, sizeof( HIPoint ), NULL, &where );

	// Is the mouse in the view?
	err = HIViewGetBounds( theControl, &bounds );
	if ( CGRectContainsPoint( bounds, where ) )
		part = 1;
	else
		part = kControlNoPart;

	// Send back the value of the hit part
	err = SetEventParameter( inEvent, kEventParamControlPart, typeControlPartCode,
			sizeof( ControlPartCode ), &part ); 

	return err;
}


void	HIViewColourPopUpControl::Hit( const SInt16 partCode )
{
	if ( partCode == kControlIndicatorPart )
		Draw( 0, true );
	else
		Draw( 0, false );
	HIViewSetNeedsDisplay( theControl, true );
}


void	HIViewColourPopUpControl::Activate()
{
	Draw( 0 );
}


void	HIViewColourPopUpControl::Deactivate()
{
	Draw( 0 );
}


OSStatus	HIViewColourPopUpControl::Track( EventRef inEvent )
{
	OSStatus				err;
	HIRect					bounds;
	HIPoint					where;
	ControlPartCode			part;

	// Extract the mouse location
	err = GetEventParameter( inEvent, kEventParamMouseLocation, typeHIPoint,
			NULL, sizeof( HIPoint ), NULL, &where );

	// Is the mouse location in the view?
	err = HIViewGetBounds( theControl, &bounds );
	if ( CGRectContainsPoint( bounds, where ) )
		part = 1;
	else
		part = kControlNoPart;
	HiliteControl( theControl, part );
	
	SInt32  mSelect;
	Point   location;
	
	origColour = selection;
	
	err = HIViewConvertRect( &bounds, theControl, NULL );
	
	location.h = (SInt16) bounds.origin.x + 5;
	location.v = (SInt16) bounds.origin.y + 2;
	QDLocalToGlobalPoint( GetWindowPort( GetControlOwner( theControl )), &location );
	
	InsertMenu( theMenu, hierMenu );
	mSelect = PopUpMenuSelect( theMenu, location.v, location.h, index );
		
	// delete and dispose of the menu now we've finished with it 
		
	DeleteMenu( kCustomPopupColourMenuID );
	
	if ( HiWord( mSelect ) == kCustomPopupColourMenuID &&
		 LoWord( mSelect ) != 0 )
		 
	{
		index = LoWord( mSelect );
		part = 1;
	}
	else
		part = kControlNoPart;
	
	// Restore the original highlight
	HiliteControl( theControl, kControlNoPart );

	// Send back the part upon which the mouse was released
	err = SetEventParameter( inEvent, kEventParamControlPart, typeControlPartCode,
			sizeof( ControlPartCode ), &part ); 

	return err;
}


void	HIViewColourPopUpControl::UpdateWithNewColour( RGBColor newColour, const bool calcIndex )
{
	selection = newColour;
	HIViewSetNeedsDisplay( theControl, true );
	
	if ( calcIndex )
	{
		// find best index for this colour in the current palette
		
		CGrafPtr	savePort;
		GDHandle	saveDevice;
		GWorldPtr	temp;
		Rect		tr;
		
		GetGWorld( &savePort, &saveDevice );
		SetRect( &tr, 0, 0, 1, 1 );
		NewGWorld( &temp, 8, &tr, theColours, NULL, 0 );
		SetGWorld( temp, NULL );
		
		index = Color2Index( &newColour ) + 1;
	
		SetGWorld( savePort, saveDevice );
		DisposeGWorld( temp );
	}
}


void	HIViewColourPopUpControl::SetColourTableID( const SInt16 tableID )
{
	if ( tableID != cTabID )
	{
		if ( theColours )
			DisposeCTable( theColours );
			
		theColours = GetCTable( tableID );
		
		if ( theColours )
			cTabID = tableID;
		else
		{
			theColours = GetCTable( 8 );
			cTabID = 8;
		}
		
		// need to recalculate the index so that our current colour is correctly indicated in the
		// new palette (if possible)
		
		UpdateWithNewColour( selection, true );
	}
}



void	HIViewColourPopUpControl::SelfRegister()
{
	EventHandlerUPP		callbackProc = NewEventHandlerUPP( CustomViewHandler );
	RegisterCustomHIView( CFSTR( kMZColourPopUpClassName ), callbackProc );
}



OSStatus RegisterCustomHIView( CFStringRef classname, EventHandlerUPP hiCallbackProc )
{
	OSStatus err;
	
	EventTypeSpec   eventList[] = {{ kEventClassHIObject, kEventHIObjectConstruct },
								   { kEventClassHIObject, kEventHIObjectDestruct },
								   { kEventClassControl, kEventControlInitialize },
								   { kEventClassControl, kEventControlDraw },
								   { kEventClassControl, kEventControlHitTest },
								   { kEventClassControl, kEventControlTrack },
								   { kEventClassControl, kEventControlHiliteChanged },
								   { kEventClassControl, kEventControlEnabledStateChanged }};
	
	//RegisterToolboxObjectClass
	
	err = HIObjectRegisterSubclass( classname, kHIViewClassID,
									0, hiCallbackProc,
									GetEventTypeCount( eventList ), eventList,
									NULL, NULL );

	return err;
}



OSStatus CustomViewHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
    OSStatus                err = eventNotHandledErr;
    UInt32                  eventClass = GetEventClass( inEvent );
    UInt32                  eventKind = GetEventKind( inEvent );
	HIViewColourPopUpControl* hip;

    switch ( eventClass )
    {
        case kEventClassHIObject:
        {
           	ControlRef  hiControl;
			
			switch ( eventKind )
            {
                case kEventHIObjectConstruct:
					err = GetEventParameter( inEvent, kEventParamHIObjectInstance,
                            typeHIObjectRef, NULL, sizeof( HIObjectRef ), 
                            NULL, &hiControl ); 
					
					if ( err == noErr )
					{
						hip = new HIViewColourPopUpControl( hiControl );
					
						// copy instance pointer as HIObjectInstance in event
					
						err = SetEventParameter( inEvent, kEventParamHIObjectInstance,
								typeVoidPtr, sizeof( HIViewColourPopUpControl* ), 
								&hip ); 
								
					}
					break;

                case kEventHIObjectInitialize:
					err = CallNextEventHandler( inCallRef, inEvent );
                    break;

                case kEventHIObjectDestruct:
					hip = (HIViewColourPopUpControl*) inUserData; 
							
					if ( hip )
						delete hip;
					err = noErr;
                    break;
            }
        }
        break;
             
        case kEventClassControl:
        {
			hip = (HIViewColourPopUpControl*) inUserData;
			
			if ( hip )
			{
				switch ( eventKind )
				{
					case kEventControlInitialize:
						hip->Initialise();
						err = noErr;
						break;
	
					case kEventControlDraw:
						hip->DrawEvent( inEvent );
						err = noErr;
						break;
								
					case kEventControlHitTest:
						err = hip->HitTest( inEvent );
						break;
						
					case kEventControlActivate:
						hip->Activate();
						err = noErr;
						break;
						
					case kEventControlDeactivate:
						hip->Deactivate();
						err = noErr;
						break;
						
					case kEventControlTrack:
						err = hip->Track( inEvent );
						break;
						
					case kEventControlHit:
						//hip->Hit();
						break;
						
					case kEventControlHiliteChanged:
					case kEventControlEnabledStateChanged:
						err = hip->ViewUpdate( inEvent );
						break;
				}
			}
        }
        break;
    }
 
    return err;
}



pascal void	ColourPopUpProc( short message, MenuHandle theMenu,
							 Rect *menuRect, Point hitPt, short *whichItem )
{
	HIViewColourPopUpControl*    theView;
	
	SInt16			chipsH,chipsV;
	SInt16			numSwatches = 0;
	CTabHandle		colours = NULL;
	
	if ( theMenu )
	{
	    GetMenuItemRefCon( theMenu, 0, (UInt32*) &theView );	
		
		if ( theView )
		    colours = theView->GetColourTable();
		else
			return;
			
		if ( colours )
			numSwatches = (*colours)->ctSize + 1;
		else
			return;

		switch ( numSwatches )
		{
			case 256:
				chipsH = 16;
				break;
				
			case 100:
				chipsH = 10;
				break;
				
			case 144:
				chipsH = 12;
				break;
			
			default:
				chipsH = 9;
				break;
			
			case 16:
				chipsH = 4;
				break;
			
			case 2:
				chipsH = 2;
				break;
		}
		
		chipsV = numSwatches / chipsH;
		
		switch( message )
		{
			case kMenuDrawMsg:
				DrawCPMenu( theMenu, menuRect, chipsH, chipsV, theView->GetCurrentItemIndex(), colours );
				break;
			
			case kMenuSizeMsg:
				CalcCPMenuSize( theMenu, chipsH, chipsV );
				break;
			
			case kMenuPopUpMsg:
				CalcPopupMenuSize( hitPt, *whichItem, menuRect, chipsH, chipsV );
			    SetMenuWidth( theMenu, menuRect->right - menuRect->left );
			    SetMenuHeight( theMenu, menuRect->bottom - menuRect->top );
				break;
				
		    case kMenuFindItemMsg:
		        FindCPItem( hitPt, menuRect, chipsH, chipsV, (MenuTrackingData*) whichItem );
		        break;
		        
		    case kMenuHiliteItemMsg:
		        HiliteCPItem( menuRect, chipsH, chipsV, (HiliteMenuItemData*) whichItem );
				if ( theView && colours )
				{
					RGBColor	nc;
					SInt16		idx;
					
					idx = ((HiliteMenuItemData*) whichItem )->newItem;
					
					if ( idx > 0 )
						nc = (*colours)->ctTable[ idx - 1 ].rgb;
					else
						nc = theView->GetOriginalColour();
				
					theView->UpdateWithNewColour( nc, false );
				
				}
		        break;
		
			case kMenuThemeSavvyMsg:
				*whichItem = kThemeSavvyMenuResponse;
				break;
			
			default:
				break;
		}
	}
}


void CalcCPMenuSize( MenuHandle theMenu, short chipsH, short chipsV )
{
	/* calculates the size of the menu. This is determined by the size of each colour
		square, which is 16 * 16, with 1 pixel surrounding each */
	
	short swatchWidth, swatchHeight;
	
	if (chipsH < 12)
		swatchWidth = swatchHeight = kDefaultSwatchWidth;
	else
		swatchWidth = swatchHeight = kSmallSwatchWidth;

    SetMenuWidth( theMenu, ( swatchWidth * chipsH ) + 6 );
    SetMenuHeight( theMenu, swatchHeight * chipsV );
}


void CalcPopupMenuSize( Point hitPt, short item, Rect *mRect, short chipsH, short chipsV )
{
	/* calculates rectangle for pop-up menu. NOTE THAT THIS SHOULD be pinned to the frame of the
		monitor that contains the largest area */
		
	Rect 		sRect;
	GDHandle	mainScreen;
	short 		swatchWidth,swatchHeight;
	
	if (chipsH < 12)
		swatchWidth = swatchHeight = kDefaultSwatchWidth;
	else
		swatchWidth = swatchHeight = kSmallSwatchWidth;

	mRect->top = hitPt.h;	/* point order is reversed */
	mRect->left = hitPt.v - 3;
	mRect->right = mRect->left + (swatchWidth * chipsH) + 6;
	mRect->bottom = mRect->top + (swatchHeight * chipsV);
	
	mainScreen = GetMainDevice();
	if (mainScreen)
	{
		sRect = (*(*mainScreen)->gdPMap)->bounds;
		InsetRect(&sRect,4,4);
	}
	if (mRect->right > sRect.right)
		OffsetRect(mRect,-(mRect->right - sRect.right),0); 
	
	if (mRect->bottom > sRect.bottom)
		OffsetRect(mRect,0,-(mRect->bottom - sRect.bottom)); 
}


void DrawCPMenu( MenuHandle theMenu, Rect *mBounds, short chipsH, short chipsV, short initItem, CTabHandle colours )
{
	Rect		swatchRect;
	register	short			v,h;
	short 		swatchWidth,swatchHeight;

	DrawThemeMenuBackground( mBounds, kThemeMenuTypePopUp );
	RGBBackColor( &RGB_WHITE );
	
	if (chipsH < 12)
		swatchWidth = swatchHeight = kDefaultSwatchWidth;
	else
		swatchWidth = swatchHeight = kSmallSwatchWidth;
	
	if ( colours )
	{
		SetRect( &swatchRect, 0, 0, swatchWidth, swatchHeight );
		OffsetRect( &swatchRect, mBounds->left + 3, mBounds->top );
		
		if ( chipsH < 12 )
			InsetRect( &swatchRect, 2, 2 );
		else
			InsetRect( &swatchRect, 1, 1 );
			
		for (v = 0;v < chipsV;v++)
		{
			for (h = 0;h < chipsH;h ++)
			{
				RGBForeColor(&(*colours)->ctTable[v * chipsV + h].rgb);
				PaintRect( &swatchRect );
				
				if (( v * chipsV + h + 1 ) == initItem )
				{
					PenState		penSave;
					Rect			tr = swatchRect;

					GetPenState( &penSave );
					PenMode( patXor );
					PenSize( 3, 3 );
					if ( chipsH < 12 )
						InsetRect( &tr, -3, -3 );
					else
						InsetRect( &tr, -2, -2 );
					RGBForeColor( &RGB_BLACK );
					FrameRect( &tr );
					SetPenState( &penSave );
				}
				
				OffsetRect( &swatchRect, swatchWidth, 0 );
			}
			OffsetRect( &swatchRect, -chipsH * swatchWidth, swatchHeight );
		}
	}
}


void FindCPItem( Point mouse, Rect* mRect, short chipsH, short chipsV, MenuTrackingData* trackData )
{
    // locate the item under the hit Pt and pass its rect back
    
    if ( PtInRect( mouse, mRect ))
    {
		short   sv, sh, swatchWidth, swatchHeight, numSwatches, item;
		Rect    swatchRect;
		
    	if ( chipsH < 12 )
    		swatchWidth = swatchHeight = kDefaultSwatchWidth;
    	else
    		swatchWidth = swatchHeight = kSmallSwatchWidth;

    	numSwatches = chipsH * chipsV;

    	SetRect( &swatchRect, 0, 0, swatchWidth, swatchHeight );
    	OffsetRect( &swatchRect, mRect->left + 3, mRect->top );

		for ( sv = 0; sv < chipsV; sv++ )
		{
			for ( sh = 0; sh < chipsH; sh++ )
			{
				if ( PtInRect( mouse, &swatchRect ))
					goto found;
				else
					OffsetRect( &swatchRect, swatchWidth, 0 );
			}
			OffsetRect( &swatchRect, -chipsH * swatchWidth, swatchHeight );
		}
        trackData->itemSelected = 0;
        trackData->itemUnderMouse = 0;
		return;
	
	found:
		item = ( sv * chipsV + sh ) + 1;
        trackData->itemSelected = item;
        trackData->itemUnderMouse = item;

    }
    else
    {
        trackData->itemSelected = 0;
        trackData->itemUnderMouse = 0;
    }
}


static void HiliteCPItem( Rect* mRect, short chipsH, short chipsV, HiliteMenuItemData* hiliteData )
{
	PenState penSave;
	short   swatchWidth, swatchHeight, numSwatches, oh, ov;
	Rect    swatchRect, oldSwatchRect;
	
	GetPenState( &penSave );
	PenMode( patXor );
	PenSize( 3, 3 );
	RGBForeColor( &RGB_BLACK );
	
	if ( chipsH < 12 )
		swatchWidth = swatchHeight = kDefaultSwatchWidth;
	else
		swatchWidth = swatchHeight = kSmallSwatchWidth;

	numSwatches = chipsH * chipsV;

	SetRect( &swatchRect, 0, 0, swatchWidth, swatchHeight );
	OffsetRect( &swatchRect, mRect->left + 3, mRect->top );
	InsetRect( &swatchRect, -1, -1 );
    oldSwatchRect = swatchRect;
    
	if ( hiliteData->previousItem != 0 )
	{
		/* there really is an old one, so calc the hilite */
		oh = ( hiliteData->previousItem - 1 ) / chipsH;
		ov = ( hiliteData->previousItem - 1 ) % chipsH;
		OffsetRect( &oldSwatchRect, ov * swatchWidth, oh * swatchHeight );
		FrameRect( &oldSwatchRect );
	}

	if ( hiliteData->newItem != 0 )
	{
		/* there really is a new one, so calc the hilite */
		oh = ( hiliteData->newItem - 1 ) / chipsH;
		ov = ( hiliteData->newItem - 1 ) % chipsH;
		OffsetRect( &swatchRect, ov * swatchWidth, oh * swatchHeight );
		FrameRect( &swatchRect );
	}
    
    SetPenState( &penSave );
}


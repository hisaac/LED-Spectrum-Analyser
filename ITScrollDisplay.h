/*
 *  ITScrollDisplay.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Wed May 19 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>

// this class implements the LED scrolling text display area



class   ITScrollDisplay
{
protected:
	static GWorldPtr	sCharacterMatrix;
	static UInt8*		sCharsBitmap;
	Rect				mBounds;
	UInt32				mPosition;
	CFStringRef			mString;
	RGBColor			mDisplayColour;
	

public:
	ITScrollDisplay( const Rect& inBounds );
	virtual ~ITScrollDisplay();
	
	
	virtual void		Update();
	virtual void		SetDisplayString( CFStringRef inString );
	virtual void		Draw();
	
	void				SetDisplayColour( const RGBColor aColour ){ mDisplayColour = aColour; }
	
protected:
	void				Init();
	void				PlotCharacter( const char theChar, const Rect& destRect );
};


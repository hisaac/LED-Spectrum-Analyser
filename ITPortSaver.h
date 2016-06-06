/*
 *  ITPortSaver.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Sat Mar 27 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>

// simple object for automatically saving a port while setting a new one, and putting things
// back when it is destructed. Use as a stack object any place you need to do this


class ITPortSaver
{
public:
	ITPortSaver( CGrafPtr newPort );
	~ITPortSaver();

private:
	CGrafPtr	savePort;
	RGBColor	saveForeColour;
	RGBColor	saveBackColour;
	bool		portWasValid;
}; 
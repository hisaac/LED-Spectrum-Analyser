/*
 *  ITPortSaver.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Sat Mar 27 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "ITPortSaver.h"



ITPortSaver::ITPortSaver( CGrafPtr newPort )
{
	GetPort( &savePort );
	
	if ( IsValidPort( newPort ))
	{
		portWasValid = true;
		SetPort( newPort );
		
		GetForeColor( &saveForeColour );
		GetBackColor( &saveBackColour );
	}
	else
		portWasValid = false;
}




ITPortSaver::~ITPortSaver()
{
	if ( portWasValid )
	{
		RGBForeColor( &saveForeColour );
		RGBBackColor( &saveBackColour );
		SetPort( savePort );
	}
}

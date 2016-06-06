/*
 *  ITAnalogueMeter.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Mon May 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>

#include "ITBarGraph.h"


class ITAnalogueMeter : public ITBarGraph
{
public:
	ITAnalogueMeter( const Rect& theBounds, const UInt16 numSegments, const UInt16 options );
	virtual ~ITAnalogueMeter();
	
	virtual void		Draw( const bool doErase = false );
	virtual void		SetColour( const RGBColor& theColour, const UInt16 which );
	
protected:
	void				InitAnalogueMeter();
	void				CalcNeedleLinePoints( SInt16 inValue, float* x1, float* y1, float* x2, float* y2, float rad = 0 );
	void				CalcMeterLinePoints( SInt16 inValue, float* x1, float* y1, float* x2, float* y2 );
	
	void				MakeArcPath( CGContextRef context, float radius, float width, SInt16 startValue, SInt16 endValue );
	
	static GWorldPtr	sBackImage;
	static UInt16		sBackUsage;
};



extern void	CentreRects( const Rect* refRect, Rect* theRect );
extern void	Scale2Rects( Rect *theRect, const Rect *refRect );

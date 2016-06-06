/*
 *  ITAnalogueMeter.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Mon May 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "ITAnalogueMeter.h"

/*---------------------------------***  Scale2Rects  ***--------------------------------*/
/*
access:			global
overrides:		
description: 	scale a rectangle to fit within reference rect, preserving its original
				aspect ratio
ins: 			<theRect> rectangle to modify
				<refRect> reference rectangle
outs: 			none
notes:			
----------------------------------------------------------------------------------------*/

void	Scale2Rects( Rect *theRect, const Rect *refRect )
{
	Rect	dr = { 0, 0, 0, 0 };
	
	OffsetRect( theRect, -theRect->left, -theRect->top );
	
	float rw, rh, sf, hxs, vxs;
	
	rw = (float) refRect->right  - (float) refRect->left;
	rh = (float) refRect->bottom - (float) refRect->top;
	
	hxs = theRect->right / rw;
	vxs = theRect->bottom / rh;
	
	if ( hxs >= vxs )
	{
		// fit width
		
		dr.right = (short) rw;
		sf = rw / (float) theRect->right;
		dr.bottom = (short)((float) theRect->bottom * sf );	
	}
	else
	{
		// fit height
		
		dr.bottom = (short) rh;
		sf = rh / (float) theRect->bottom;
		dr.right = (short)((float) theRect->right * sf );
	}
	CentreRects( refRect, &dr );
	*theRect = dr;		
}



UInt16		ITAnalogueMeter::sBackUsage = 0;
GWorldPtr   ITAnalogueMeter::sBackImage = NULL;

static bool		quartz = true;

ITAnalogueMeter::ITAnalogueMeter( const Rect& theBounds, const UInt16 numSegments, const UInt16 options )
	: ITBarGraph( theBounds, numSegments, options )
{
	// force aspect ratio of bounds to be something "nice"
	
	Rect	rr;
	
	SetRect( &rr, 0, 0, 200, 100 );
	Scale2Rects( &rr, &theBounds );
	
	bounds = rr;
		
	// create the fixed background graphics, etc.

	InitAnalogueMeter();

	++sBackUsage;
}


ITAnalogueMeter::~ITAnalogueMeter()
{
	--sBackUsage;
	
	if ( sBackUsage == 0 && sBackImage )
	{
		DisposeGWorld( sBackImage );
		sBackImage = NULL;
	}
}




void	ITAnalogueMeter::Draw( const bool doErase )
{
	// draw the new needle position
	

	CGrafPtr		port;
	RgnHandle		saveClip;
	
	saveClip = NewRgn();
	
	GetPort( &port );
	LockPortBits( port );
	
	Rect	r = bounds;
	
	OffsetRect( &r, -r.left, -r.top );
	RGBForeColor( &RGB_BLACK );
	RGBBackColor( &RGB_WHITE );
	
	GetClip( saveClip );
	ClipRect( &bounds );
	
	CopyBits( GetPortBitMapForCopyBits( sBackImage ),
				GetPortBitMapForCopyBits( port ),
					&r, &bounds, srcCopy, NULL );
	
	PenNormal();
	PenSize( 2, 2 );
	RGBForeColor( &barRGB );
	
	float  x1, x2, y1, y2, px1, px2, py1, py2;
	

	CalcNeedleLinePoints( value, &x1, &y1, &x2, &y2 );
	CalcNeedleLinePoints( peakValue, &px1, &py1, &px2, &py2, bounds.bottom - bounds.top - 5 );
	
	if ( quartz )
	{
		CGContextRef		context;
		Rect				portBounds;
		SInt16				pby;
		RgnHandle			clip;
		
		// flip y axis
		
		GetPortBounds( port, &portBounds );
		pby = portBounds.bottom - portBounds.top;
		
		y1 = pby - y1;
		y2 = pby - y2;
		py1 = pby - py1;
		py2 = pby - py2;
		
		Rect	cr = bounds;

		clip = NewRgn();
		InsetRect( &cr, 3, 3 );
		ClipRect( &cr );
		GetClip( clip );
		
		QDBeginCGContext( port, &context );
		
		ClipCGContextToRegion( context, &portBounds, clip );
		
		CGSize		offset;
		
		offset.width = 4;
		offset.height = -2;
		
		CGContextSetShadow( context, offset, 3.0 );

		CGContextBeginPath( context );
		CGContextMoveToPoint( context, x1, y1 );
		CGContextAddLineToPoint( context, x2, y2 );
		
		CGContextSetRGBStrokeColor( context,	(float) barRGB.red / 65535.0,
												(float) barRGB.green / 65535.0,
												(float) barRGB.blue / 65535.0,
												1.0 );
		CGContextSetLineWidth( context, 2.8 );
		CGContextStrokePath( context);
		
		if ( peakEnable )
		{
			offset.width = 2;
			CGContextSetShadow( context, offset, 2.0 );

			CGContextBeginPath( context );
			CGContextMoveToPoint( context, px1, py1 );
			CGContextAddLineToPoint( context, px2, py2 );
			
			CGContextSetRGBStrokeColor( context,	(float) peakRGB.red / 65535.0,
													(float) peakRGB.green / 65535.0,
													(float) peakRGB.blue / 65535.0,
													1.0 );
			CGContextSetLineWidth( context, 1.2 );
			CGContextStrokePath( context);
		}
		
		
		QDEndCGContext( port, &context );
		DisposeRgn( clip );
	}
	else
	{
		MoveTo( x1, y1 );
		LineTo( x2, y2 );
	}
	
	SetClip( saveClip );
	UnlockPortBits( port );
	
	RGBForeColor( &RGB_BLACK );
	PenNormal();
	PenSize( 1, 1 );
	DisposeRgn( saveClip );
}


void	ITAnalogueMeter::SetColour( const RGBColor& theColour, const UInt16 which )
{
	ITBarGraph::SetColour( theColour, which );
	
	if ( sBackImage )
	{
		DisposeGWorld( sBackImage );
		sBackImage = NULL;
	}
	
	InitAnalogueMeter();
}


void	ITAnalogueMeter::InitAnalogueMeter()
{
	if ( sBackImage == NULL )
	{
		QDErr err;
		Rect	r;
		
		r = bounds;
		OffsetRect( &r, -r.left, -r.top );
		
		err = NewGWorld( &sBackImage, 32, &r, NULL, NULL, 0 );
		
		if ( err == noErr )
		{
			// draw the background elements which are the scales, etc.
			
			CGrafPtr	sp;
			GDHandle	sd;
			
			GetGWorld( &sp, &sd );
			SetGWorld( sBackImage, NULL );
			
			// alt bar colour is used for background of analogue meters
			
			RGBBackColor( &altBarRGB );
			EraseRect( &r );
			RGBBackColor( &RGB_WHITE );
			
			// segments is used for the number of scale marks
			// peakRGB is used for scale colour
			
			RGBForeColor( &RGB_BLACK );
			PenNormal();
			PenSize( 2, 2 );
			
			
			float  x1, x2, y1, y2;
			
			if ( quartz )
			{
				CGContextRef	context;
				Rect			portBounds;
				SInt16			pby;
				
				GetPortBounds( sBackImage, &portBounds );
				pby = portBounds.bottom - portBounds.top;
				
				QDBeginCGContext( sBackImage, &context );
				
				CGContextSetLineCap( context, kCGLineCapRound );
				CGContextSetRGBStrokeColor( context, 0, 0, 0, 1.0 );
				
				for( int i = 0; i <= segments; i++ )
				{
					SInt16 v = ( i * fullScaleDeflection ) / segments;
					/*
					if ( ! IsInLogMode())
					{
						v = fullScaleDeflection - roundtol( logMultiplier * log( v ));
					}
					*/
					CalcMeterLinePoints( v, &x1, &y1, &x2, &y2 );
				
					CGContextBeginPath( context );
					CGContextMoveToPoint( context, x1 - bounds.left, pby - ( y1 - bounds.top ));
					CGContextAddLineToPoint( context, x2 - bounds.left, pby - ( y2 - bounds.top ));
					
					CGContextSetLineWidth( context, 1.5 );
					CGContextStrokePath( context);
					CGContextClosePath( context );
				}
				
				CGContextBeginPath( context );
				
				float xx, yy;
				
				xx = ( r.left + r.right ) / 2;
				
				if ( inverted )
					yy = pby - ( r.top - 10 );
				else
					yy = pby - ( r.bottom + 10 );
				
				CGContextAddArc( context, xx, yy, 24.0, 0, pi, 0 );
				CGContextClosePath( context );
				
				CGContextSetLineWidth( context, 1.3 );
				CGContextSetRGBFillColor( context, 0, 0, 0, 0.3 );
				CGContextDrawPath( context, kCGPathFillStroke );
				
				CGContextSetLineWidth( context, 0.4 );
				CGContextBeginPath( context );
				
				if ( inverted )
					yy = pby - ( r.top - 100 );
				else
					yy = pby - ( r.bottom + 100 );
				
				float x, y, rad, a1, a2;
				
				rad = ( r.bottom - r.top ) + 70;
				x = xx - 20;
				y = sqrt(( rad * rad ) - ( x * x ));
				a1 = atan2( y, x );
				a2 = atan2( y, -x );
				
				CGContextAddArc( context, xx, yy, rad, a1, a2, 0 );
				CGContextStrokePath( context);
				
				CGContextBeginPath( context );
				CGContextAddArc( context, xx, yy, rad - 3, a1, a2, 0 );
				CGContextStrokePath( context);
				
				// framed edge with shadow:
				CGSize			offset;
				CGRect			qr;
				CGColorRef		colr;
				CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
				float			components[] = { 0.0, 0.0, 0.0, 1.0 };
				
				offset.width = 3;
				offset.height = -4;
				qr.origin.x = 3;
				qr.origin.y = 3;
				qr.size.width = ( bounds.right - bounds.left ) - 6;
				qr.size.height = ( bounds.bottom - bounds.top ) - 6;
				
				colr = CGColorCreate( space, components );
				
				//CGContextSetShadow( context, offset, 2.0 );
				
				CGContextSetShadowWithColor( context, offset, 5.0, colr );
				
				//CGContextBeginPath( context );
				//CGContextAddRect( context, qr );
				CGContextSetLineWidth( context, 1.0 );
				CGContextSetRGBStrokeColor( context, 0, 0, 0, 0.7 );
				CGContextStrokeRect( context, qr );
				//CGContextStrokePath( context);
				CGContextSetShadowWithColor( context, offset, 0, NULL );
				
				CFRelease( colr );
				CFRelease( space );

				// shadow effect:
				
				CGContextBeginPath( context );
				CGContextMoveToPoint( context, r.left + 1, pby - ( r.bottom - 1 ));
				CGContextAddLineToPoint( context, r.right - 1 , pby - ( r.bottom - 1 ));
				CGContextAddLineToPoint( context, r.right - 1 , pby - ( r.top + 1 ));
				CGContextSetRGBStrokeColor( context, 0, 0, 0, 0.7 );
				CGContextSetLineWidth( context, 2.0 );
				CGContextStrokePath( context);
				
				CGContextBeginPath( context );
				CGContextMoveToPoint( context, r.left + 1, pby - ( r.bottom - 1 ));
				CGContextAddLineToPoint( context, r.left + 1 , pby - ( r.top + 1 ));
				CGContextAddLineToPoint( context, r.right - 1 , pby - ( r.top + 1 ));
				CGContextSetRGBStrokeColor( context, 1, 1, 1, 0.5 );
				CGContextSetLineWidth( context, 1.0 );
				CGContextStrokePath( context);

				QDEndCGContext( sBackImage, &context );
			}
			else
			{
				for( int i = 0; i <= segments; i++ )
				{
					SInt16 v = ( i * fullScaleDeflection ) / segments;
					
					CalcMeterLinePoints( v, &x1, &y1, &x2, &y2 );
				
					MoveTo( x1 - bounds.left, y1 - bounds.top );
					LineTo( x2 - bounds.left, y2 - bounds.top );
				}
				
				PenNormal();
				PenSize( 1, 1 );
				
				r.left = r.right = ( r.left + r.right ) / 2;
				r.top = r.bottom = bounds.bottom - bounds.top + 10;
				
				InsetRect( &r, -20, -20 );
				FrameOval( &r );
			}
			
			r = bounds;
			OffsetRect( &r, -r.left, -r.top );
			InsetRect( &r, 10, ( r.bottom - r.top ) / 3 );
			OffsetRect( &r, 0, (( bounds.bottom + bounds.top ) / 2 ) - bounds.top - r.top );
			DrawThemeTextBox( CFSTR("VU"), kThemeSystemFont, kThemeStateActive, false, &r, teJustCenter, NULL );
			
			SetGWorld( sp, sd );
		}
	}
}


void	ITAnalogueMeter::CalcNeedleLinePoints( SInt16 inValue, float* x1, float* y1, float* x2, float* y2, float rad )
{
	// calculate end points of the needle line from stored current value
	
	float  ox, oy;
	
	// needle always anchored at the same point which is centred horizontally below the bottom edge
	
	ox = *x2 = (float)( bounds.left + bounds.right ) / 2.0;
	
	if ( inverted )
		oy = *y2 = bounds.top - 10;
	else
		oy = *y2 = bounds.bottom + 10;
	
	float  sx1, sy1, sx2, sy2;
	
	// get zero angle point:
	
	CalcMeterLinePoints( 0, &sx1, &sy1, &sx2, &sy2 );
	
	double  theta, radius, a1, a2, x, y;
	
	x = ox - sx1;
	y = oy - sy1;
	a1 = atan2( y, x );
	a2 = atan2( y, -x );
	
	if ( rad == 0 )
		radius = bounds.bottom - bounds.top;
	else
		radius = rad;
		
	// theta is an angular value between a1 and a2
	
	theta = a1 + (( inValue * ( a2 - a1 )) / fullScaleDeflection );
	
	*x1 = ox - ( cos( theta ) * radius );
	*y1 = oy - ( sin( theta ) * radius );
}


void	ITAnalogueMeter::CalcMeterLinePoints( SInt16 inValue, float* x1, float* y1, float* x2, float* y2 )
{
	// calculate end points of scale marks given value relative to full scale deflection
	
	float		ox, oy, markLength;
	Rect		br = bounds;
	
	InsetRect( &br, 5, 5 );
	
	// marks centred at the same point which is centred horizontally well below the bottom edge
	
	ox = ( br.left + br.right ) / 2.0;
	
	if ( inverted )
		oy = br.top - 100;
	else
		oy = br.bottom + 100;
	markLength = ( br.bottom - br.top ) / 10.0;
	
	// other end depends on given value
	
	double  theta, radius, a1, a2, x, y;
	
	radius = (double)( br.bottom - br.top + 95 );
	x = ox - br.left;
	
	if ( radius > x )
		y = sqrt(( radius * radius ) - ( x * x ));
	else
		y = sqrt(( x * x ) - ( radius * radius ));
	// overall angle range depends also on the bounds rect - the most extreme leftwards angle representing zero
	// and the most extreme rightwards angle representing full scale deflection. Before we can calculate the
	// instantaneous angle, we need to work out the limiting angles:
	
	a1 = atan2( y, x );
	a2 = atan2( y, -x );
	
	// theta is an angular value between a1 and a2
	
	theta = a1 + (( inValue * ( a2 - a1 )) / fullScaleDeflection );
	
	// x1, y1 are the outer points on the scale arc
	
	*x1 = ox - ( cos( theta ) * radius );
	*y1 = oy - ( sin( theta ) * radius );
	
	// x2, y2 are the inner points on the scale arc, and are aligned such that the marks drawn
	// would intersect the needle origin, not the scale origin. The calculated end point x1, y1 becomes
	// the effective origin for the new point
	
	// calc new y value; x remains the same
	
	if ( inverted )
	{
		y -= oy - ( br.top - 10 );
		oy = br.top - 10;
	}
	else
	{
		y -= oy - ( br.bottom + 10 );
		oy = br.bottom + 10;
	}
	
	// mark lies along line of radius, but its origin is the first point x1, y1
	
	
	a1 = atan2( y, x );
	a2 = atan2( y, -x );
	
	theta = a1 + (( inValue * ( a2 - a1 )) / fullScaleDeflection );
	
	*x2 = *x1 + ( cos( theta ) * markLength );
	*y2 = *y1 + ( sin( theta ) * markLength );
}



void		ITAnalogueMeter::MakeArcPath( CGContextRef context, float radius, float width, SInt16 startValue, SInt16 endValue )
{
 // to do
}


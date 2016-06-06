/*
 *  ITBarGraph.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Tue Mar 02 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "ITBarGraph.h"
#include "QDMP.h"

const RGBColor		RGB_BLACK = { 0, 0, 0 };
const RGBColor		RGB_WHITE = { 0xFFFF, 0xFFFF, 0xFFFF };
const RGBColor		RGB_BRIGHT_GREEN = { 0, 0xFFFF, 0x4444 };
const RGBColor		RGB_BRIGHT_RED = { 0xFFFF, 0, 0 };
const RGBColor		RGB_ICE_BLUE = { 39321, 39321, 65535 };
const RGBColor		RGB_BRIGHT_BLUE = { 0x5555, 0xAAAA, 0xFFFF };
const RGBColor		RGB_YELLOW = { 0xFFFF, 0xFFFF, 0 };
const RGBColor		RGB_MAGENTA = { 0xFFFF, 0, 0xFFFF };
const RGBColor		RGB_VERY_DARK_GRAY = { 0x1111, 0x1111, 0x1111 };


bool		ITBarGraph::logMode = true;
bool		ITBarGraph::peakEnable = true;
double		ITBarGraph::logMultiplier;
bool		ITBarGraph::expDecay = true;
RGBColor	ITBarGraph::backgroundRGB = RGB_VERY_DARK_GRAY;
UInt16		ITBarGraph::barTimeConstant = 32;			// value is in ticks (1/60th sec)
UInt16		ITBarGraph::peakTimeConstant = 32;
UInt16		ITBarGraph::peakHoldTime = 8;				// bigger number is longer hold time



inline static int ABS( int a );
inline static int MIN( int a, int b );
inline static int MAX( int a, int b );


int ABS( int a )
{
	return ( a < 0 )? -a : a;
}

int MIN( int a, int b )
{
	return ( a < b )? a : b;
}

int MAX( int a, int b )
{
	return ( a > b )? a : b;
}




ITBarGraph::ITBarGraph( const Rect& theBounds, const UInt16 numSegments, const UInt16 options )
{
	segments = numSegments;
	inverted = false;
	if ( options & kInvertOrientation )
		SetInverted( true );
		
	// initialise fixed parameters:

	barRGB = RGB_WHITE;
	peakRGB = RGB_BRIGHT_RED;
	altBarRGB = RGB_YELLOW;
	 
	value = peakValue = 0;
	Avalue = Pvalue = 0;
	fullScaleDeflection = 255;
	segmentSpacing = 2;
	colourMode = kGraduatedColours;

	// compute other params
	
	valuePerSegment = fullScaleDeflection / segments;
	SetBounds( theBounds );
	
	logMultiplier = fullScaleDeflection / log( fullScaleDeflection );
	barDecayTime = peakDecayTime = TickCount();
}


ITBarGraph::~ITBarGraph()
{

}




void			ITBarGraph::SetBounds( const Rect& newBounds )
{
	bounds = newBounds;
	
	isVertical = (( bounds.bottom - bounds.top ) > ( bounds.right - bounds.left ));
	
	if ( isVertical )
	{
		segmentSpacing = ( bounds.bottom - bounds.top ) / ( segments * 2 );
		if ( segmentSpacing < 1 )
			segmentSpacing = 1;
					
		segmentRect.left = bounds.left + 1;
		segmentRect.right = bounds.right - 1;
		
		if ( IsInverted())
		{
			segmentRect.top = bounds.top + segmentSpacing;
			segmentRect.bottom = bounds.top + (( bounds.bottom - bounds.top ) / segments );
		}
		else
		{
			segmentRect.bottom = bounds.bottom - segmentSpacing;
			segmentRect.top = bounds.bottom - (( bounds.bottom - bounds.top ) / segments );
		}
	}
	else
	{
		segmentSpacing = ( bounds.right - bounds.left ) / ( segments * 2 );
		if ( segmentSpacing < 1 )
			segmentSpacing = 1;

		segmentRect.top = bounds.top + 1;
		segmentRect.bottom = bounds.bottom - 1;
		
		if ( IsInverted())
		{
			segmentRect.right = bounds.right - segmentSpacing;
			segmentRect.left = bounds.right - (( bounds.right - bounds.left ) / segments );
		}
		else
		{
			segmentRect.left = bounds.left + segmentSpacing;
			segmentRect.right = bounds.left + (( bounds.right - bounds.left ) / segments );
		}
	}
}



void			ITBarGraph::SetValue( const UInt16 newValue )
{
	UInt32  t = TickCount();

	if ( newValue > value )
	{
		value = Avalue = newValue;
		barDecayTime = t; // reset timing for decay
	}
	else
	{
		if ( value > 0 )
		{
			if ( expDecay )
			{
				double v = Avalue * exp( -(( t - barDecayTime ) / (double) barTimeConstant ));
				value = roundtol( v );
			}
			else
				value = roundtol( Avalue - ( fullScaleDeflection * (( t - barDecayTime ) / (double) barTimeConstant )));
		}
		else
			Avalue = 0;
	}
	
	value = MIN( MAX( 0, value ), fullScaleDeflection );
	
	// update peak too if newValue is greater than current peak
	
	if ( newValue > peakValue )
	{
		peakValue = Pvalue = newValue;
		peakHoldTimeCount = t + peakHoldTime;
	}
	else
	{
		if (( t > peakHoldTimeCount ) && ( peakValue > 0 ))
		{
			if ( expDecay )
			{
				double v = Pvalue * exp( -(( t - peakDecayTime ) / (double) peakTimeConstant ));
				peakValue = roundtol( v );
			}
			else
				peakValue = roundtol( Pvalue - ( fullScaleDeflection * (( t - peakDecayTime ) / (double) peakTimeConstant )));
		
			peakValue = MIN( MAX( peakValue, value ), fullScaleDeflection );
		}
		else
			peakDecayTime = t;
	}
}



void			ITBarGraph::Update( const UInt16 newValue, const bool doErase )
{
	UInt16 v;
	
	if (logMode )
	{
		if ( newValue == 0 )
			v = 0;
		else
			v = roundtol( logMultiplier * log( newValue ));
	}
	else
		v = newValue;
		
	v = MAX( 0, MIN( v, fullScaleDeflection ));
	
	SetValue( v );
	Draw();
}



void			ITBarGraph::Draw( const bool doErase )
{
	CGrafPtr		port;
	PixMapHandle	pMap;
	
	GetPort( &port );
	LockPortBits( port );
	pMap = GetPortPixMap( port );
	switch((*pMap)->pixelSize )
	{
		case 32:
			Redraw32( pMap, doErase );
			break;
			
		case 16:
			Redraw16( pMap, doErase );
			break;
			
		default:
			RedrawQD( doErase );
			break;
	}
	UnlockPortBits( port );
}



void			ITBarGraph::Erase()
{
	RGBBackColor( &backgroundRGB );
	EraseRect( &bounds );
}



void			ITBarGraph::RedrawQD( const bool doErase )
{
	// draw the bargraph to represent the current value and peak value
	
	register Rect		tr = segmentRect;
	register SInt16		segx, segy;
	register UInt16		thresh = ( value * segments ) / fullScaleDeflection;
	register UInt16		i, pk = ( peakValue * segments ) / fullScaleDeflection;
	
	// if we are modulating the colour, calculate the increments
	
	SInt32		rInc, gInc, bInc;
	RGBColor	temp;
	
	if ( colourMode != kFixedColours )
	{
		SInt16  v;
		
		if ( colourMode == kGraduatedToMax )
			v = fullScaleDeflection;
		else
			v = value;
			
		if ( v == 0 )
			v = 1;
		
		rInc = (( altBarRGB.red   - barRGB.red  ) * valuePerSegment ) / v;
		gInc = (( altBarRGB.green - barRGB.green) * valuePerSegment ) / v;
		bInc = (( altBarRGB.blue  - barRGB.blue ) * valuePerSegment ) / v;
		
		temp = barRGB;
	}
	
	if ( thresh > 0 )
		--thresh;
		
	if ( pk > 0 )
		--pk;
	
	segx = ( segmentRect.right - segmentRect.left ) + segmentSpacing;
	segy = ( segmentRect.bottom - segmentRect.top ) + segmentSpacing;
	
	RGBForeColor( &barRGB );
	
	for( i = 0; i < thresh; i++ )
	{
		// is segment lit?
		
		if ( colourMode != kFixedColours )
		{
			temp.red += rInc;
			temp.green += gInc;
			temp.blue += bInc;
			RGBForeColor( &temp );
		}
		
		PaintRect( &tr );
		
		if ( isVertical )
		{
			if ( inverted )
			{
				tr.top += segy;
				tr.bottom += segy;
			}
			else
			{
				tr.top -= segy;
				tr.bottom -= segy;
			}
		}
		else
		{
			if ( inverted )
			{
				tr.left -= segx;
				tr.right -= segx;
			}
			else
			{
				tr.left += segx;
				tr.right += segx;
			}
		}
	}
	
	RGBForeColor( &backgroundRGB );
	
	for( i = thresh; i < segments; i++ )
	{
		// deal with peak indicator
		
		if (( i == pk ) && peakEnable )
			RGBForeColor( &peakRGB );
		else
			RGBForeColor( &backgroundRGB );
	
		PaintRect( &tr );
		
		if ( isVertical )
		{
			if ( inverted )
			{
				tr.top += segy;
				tr.bottom += segy;
			}
			else
			{
				tr.top -= segy;
				tr.bottom -= segy;
			}
		}
		else
		{
			if ( inverted )
			{
				tr.left -= segx;
				tr.right -= segx;
			}
			else
			{
				tr.left += segx;
				tr.right += segx;
			}
		}
	}
}

// as above, but uses QDMP for direct pixel access and thus, speed. This only works on 32-bit depth.

void		ITBarGraph::Redraw32( PixMapHandle portPixMap, const bool doErase )
{
	// draw the bargraph to represent the current value and peak value
	
	register Rect		tr = segmentRect;
	register SInt16		segx, segy;
	register UInt16		thresh = ( value * segments ) / fullScaleDeflection;
	register UInt16		i, pk = ( peakValue * segments ) / fullScaleDeflection;
	
	// if we are modulating the colour, calculate the increments
	
	SInt32		rInc, gInc, bInc;
	RGBColor	temp;
	
	UInt32		tempColourByte;
	
	if ( colourMode != kFixedColours )
	{
		SInt16  v;
		
		if ( colourMode == kGraduatedToMax )
			v = fullScaleDeflection;
		else
			v = value;
			
		if ( v == 0 )
			v = 1;
		
		rInc = (( altBarRGB.red   - barRGB.red  ) * valuePerSegment ) / v;
		gInc = (( altBarRGB.green - barRGB.green) * valuePerSegment ) / v;
		bInc = (( altBarRGB.blue  - barRGB.blue ) * valuePerSegment ) / v;
		
		temp = barRGB;
	}
	
	if ( thresh > 0 )
		--thresh;
		
	if ( pk > 0 )
		--pk;
	
	segx = ( segmentRect.right - segmentRect.left ) + segmentSpacing;
	segy = ( segmentRect.bottom - segmentRect.top ) + segmentSpacing;
	
	tempColourByte = RGBColorToColor32( &barRGB );
	
	for( i = 0; i < thresh; i++ )
	{
		// is segment lit?
		
		if ( colourMode != kFixedColours )
		{
			temp.red += rInc;
			temp.green += gInc;
			temp.blue += bInc;
			tempColourByte = RGBColorToColor32( &temp );
		}
		
		QDMP_Fill_Rect32( portPixMap, tr, tempColourByte );
		
		if ( isVertical )
		{
			if ( inverted )
			{
				tr.top += segy;
				tr.bottom += segy;
			}
			else
			{
				tr.top -= segy;
				tr.bottom -= segy;
			}
		}
		else
		{
			if ( inverted )
			{
				tr.left -= segx;
				tr.right -= segx;
			}
			else
			{
				tr.left += segx;
				tr.right += segx;
			}
		}
	}
	
	tempColourByte = RGBColorToColor32( &backgroundRGB );
	
	for( i = thresh; i < segments; i++ )
	{
		// deal with peak indicator
		
		if (( i == pk ) && peakEnable )
			tempColourByte = RGBColorToColor32( &peakRGB );
		else
			tempColourByte = RGBColorToColor32( &backgroundRGB );
	
		QDMP_Fill_Rect32( portPixMap, tr, tempColourByte );
		
		if ( isVertical )
		{
			if ( inverted )
			{
				tr.top += segy;
				tr.bottom += segy;
			}
			else
			{
				tr.top -= segy;
				tr.bottom -= segy;
			}
		}
		else
		{
			if ( inverted )
			{
				tr.left -= segx;
				tr.right -= segx;
			}
			else
			{
				tr.left += segx;
				tr.right += segx;
			}
		}
	}
}


void		ITBarGraph::Redraw16( PixMapHandle portPixMap, const bool doErase )
{
	// draw the bargraph to represent the current value and peak value
	
	register Rect		tr = segmentRect;
	register SInt16		segx, segy;
	register UInt16		thresh = ( value * segments ) / fullScaleDeflection;
	register UInt16		i, pk = ( peakValue * segments ) / fullScaleDeflection;
	
	// if we are modulating the colour, calculate the increments
	
	SInt32		rInc, gInc, bInc;
	RGBColor	temp;
	
	UInt16		tempColourByte;
	
	if ( colourMode != kFixedColours )
	{
		SInt16  v;
		
		if ( colourMode == kGraduatedToMax )
			v = fullScaleDeflection;
		else
			v = value;
			
		if ( v == 0 )
			v = 1;
		
		rInc = (( altBarRGB.red   - barRGB.red  ) * valuePerSegment ) / v;
		gInc = (( altBarRGB.green - barRGB.green) * valuePerSegment ) / v;
		bInc = (( altBarRGB.blue  - barRGB.blue ) * valuePerSegment ) / v;
		
		temp = barRGB;
	}
	
	if ( thresh > 0 )
		--thresh;
		
	if ( pk > 0 )
		--pk;
	
	segx = ( segmentRect.right - segmentRect.left ) + segmentSpacing;
	segy = ( segmentRect.bottom - segmentRect.top ) + segmentSpacing;
	
	tempColourByte = RGBColorToColor16( &barRGB );
	
	for( i = 0; i < thresh; i++ )
	{
		// is segment lit?
		
		if ( colourMode != kFixedColours )
		{
			temp.red += rInc;
			temp.green += gInc;
			temp.blue += bInc;
			tempColourByte = RGBColorToColor16( &temp );
		}
		
		QDMP_Fill_Rect16( portPixMap, tr, tempColourByte );
		
		if ( isVertical )
		{
			if ( inverted )
			{
				tr.top += segy;
				tr.bottom += segy;
			}
			else
			{
				tr.top -= segy;
				tr.bottom -= segy;
			}
		}
		else
		{
			if ( inverted )
			{
				tr.left -= segx;
				tr.right -= segx;
			}
			else
			{
				tr.left += segx;
				tr.right += segx;
			}
		}
	}
	
	tempColourByte = RGBColorToColor16( &backgroundRGB );
	
	for( i = thresh; i < segments; i++ )
	{
		// deal with peak indicator
		
		if (( i == pk ) && peakEnable )
			tempColourByte = RGBColorToColor16( &peakRGB );
		else
			tempColourByte = RGBColorToColor16( &backgroundRGB );
	
		QDMP_Fill_Rect16( portPixMap, tr, tempColourByte );
		
		if ( isVertical )
		{
			if ( inverted )
			{
				tr.top += segy;
				tr.bottom += segy;
			}
			else
			{
				tr.top -= segy;
				tr.bottom -= segy;
			}
		}
		else
		{
			if ( inverted )
			{
				tr.left -= segx;
				tr.right -= segx;
			}
			else
			{
				tr.left += segx;
				tr.right += segx;
			}
		}
	}
}




void		ITBarGraph::SetColour( const RGBColor& theColour, const UInt16 which )
{
	switch (which )
	{
		case kBackgroundColour:
			backgroundRGB = theColour;
			break;
			
		case kBarColour:
			barRGB = theColour;
			break;
			
		case kPeakColour:
			peakRGB = theColour;
			break;
			
		case kAltBarColour:
			altBarRGB = theColour;
			break;
	}
}



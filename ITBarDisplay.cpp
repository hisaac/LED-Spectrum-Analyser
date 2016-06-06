/*
 *  ITBarDisplay.cpp
 *  iTunesXPlugIn
 *
 *  Created by graham on Tue Mar 02 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include "ITBarDisplay.h"

static const RGBColor   RGB_DARK_BLUE = { 0, 13107, 13107 };
static const RGBColor   RGB_GLOWING_GREEN = { 52428, 65535, 0 };


Layout		ITBarDisplay::layout = kLayoutSideBySide;
bool		ITBarDisplay::border = true;
bool		ITBarDisplay::vuenabled = true;
bool		ITBarDisplay::flipleft = false;
bool		ITBarDisplay::flipright = false;
UInt16		ITBarDisplay::spectrumSegments = 24;
UInt16		ITBarDisplay::vuSegments = 54;
ColourMode  ITBarDisplay::colourMode = kGraduatedColours;
RGBColor	ITBarDisplay::barColour = RGB_DARK_BLUE;
RGBColor	ITBarDisplay::altBarColour = RGB_ICE_BLUE;
RGBColor	ITBarDisplay::peakColour = RGB_GLOWING_GREEN;
RGBColor	ITBarDisplay::vuBarColour = RGB_BRIGHT_RED;
RGBColor	ITBarDisplay::vuAltBarColour = RGB_YELLOW;
RGBColor	ITBarDisplay::vuPeakColour = RGB_GLOWING_GREEN;
RGBColor	ITBarDisplay::scaleColour = RGB_WHITE;
SInt16		ITBarDisplay::paletteID = 0;
bool		ITBarDisplay::showScales = false;


static const RGBColor RGB_DARKMIDGRAY = { 0x6666, 0x6666, 0x6666 };

static CTabHandle   gAnimationTable = NULL;


// these tables map raw spectrum data channels to the display channels, using a geometric
// weighting to give a logarithmic frequency scale. Each table sums to 255, and the number
// of entries in the table corresponds to the number of bands displayed. This gives very
// rapid lookup and calculation of the binning for each channel.
/*
static SInt16 tab_10[] = { 1, 2, 3, 5, 8, 13, 22, 37, 62, 102 };
static SInt16 tab_11[] = { 1, 2, 2, 4, 6, 10, 15, 24, 38, 60, 93 };
static SInt16 tab_12[] = { 1, 1, 2, 3, 5, 8, 11, 17, 25, 38, 57, 87 };
static SInt16 tab_13[] = { 1, 1, 2, 3, 4, 6, 9, 13, 18, 26, 38, 55, 79 };
static SInt16 tab_14[] = { 1, 1, 2, 3, 4, 5, 7, 10, 14, 19, 27, 37, 52, 73 }; 
static SInt16 tab_15[] = { 1, 1, 2, 2, 3, 4, 6, 8, 11, 15, 20, 27, 37, 50, 68 };
static SInt16 tab_16[] = { 1, 1, 2, 2, 3, 4, 5, 7, 9, 12, 16, 21, 27, 36, 47, 62 };
static SInt16 tab_17[] = { 1, 1, 2, 3, 3, 3, 5, 6, 8, 10, 13, 16, 21, 27, 35, 45, 57 };
static SInt16 tab_18[] = { 1, 1, 2, 2, 3, 3, 4, 5, 7, 8, 10, 13, 17, 21, 27, 34, 43, 54 };
static SInt16 tab_19[] = { 1, 1, 2, 2, 2, 3, 4, 5, 6, 7, 9, 11, 14, 17, 21, 26, 33, 41, 50 };
static SInt16 tab_20[] = { 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 8, 9, 12, 14, 17, 21, 26, 32, 39, 48 };
static SInt16 tab_21[] = { 1, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 12, 14, 17, 21, 25, 31, 37, 45 };
static SInt16 tab_22[] = { 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 9, 10, 12, 15, 17, 21, 25, 30, 36, 43 };
static SInt16 tab_23[] = { 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 5, 6, 8, 9, 11, 12, 15, 17, 21, 24, 29, 34, 40 };
static SInt16 tab_24[] = { 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 9, 11, 13, 15, 17, 20, 24, 28, 33, 38 };
static SInt16 tab_25[] = { 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 5, 5, 6, 7, 8, 9, 11, 13, 15, 17, 20, 23, 27, 31, 36 };
static SInt16 tab_26[] = { 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 10, 11, 13, 15, 17, 20, 23, 26, 30, 35 };
static SInt16 tab_27[] = { 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 7, 7, 9, 10, 11, 13, 15, 17, 19, 22, 25, 29, 33 };
static SInt16 tab_28[] = { 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 21, 24, 28, 31 };
static SInt16 tab_29[] = { 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 11, 13, 15, 16, 19, 21, 24, 27, 30 };
static SInt16 tab_30[] = { 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 23, 26, 29 };
static SInt16 tab_31[] = { 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 13, 14, 16, 18, 20, 22, 25, 28 };
static SInt16 tab_32[] = { 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 7, 7, 8, 9, 10, 11, 13, 14, 16, 17, 19, 22, 24, 27 };

// this table collects the individual tables into a list so that the right one can be used
// simply by indexing off the number of channels (-10).

static SInt16* geo_tab[] = { tab_10, tab_11, tab_12, tab_13,
							 tab_14, tab_15, tab_16, tab_17,
							 tab_18, tab_19, tab_20, tab_21,
							 tab_22, tab_23, tab_24, tab_25,
							 tab_26, tab_27, tab_28, tab_29,
							 tab_30, tab_31, tab_32 };
*/
							 
static inline double linch( const UInt16 inLogch, const UInt16 nLogChannels )
{
	// returns the linear channel corresponding to the nth log channel in a set
	
	double dec = ( 3.0 / nLogChannels ) * inLogch;
	
	return (( 20 * ( pow(10, dec) )) - 20 ) / 78.047;
}


static SInt16 logtabs[22][32];

static void BuildLogTables()
{
	for( int n = 10; n < 32; n++ )
	{
		double prev = 0;
		
		for ( int i = 0; i < n; i++ )
		{
			double lin = linch( i + 1, n );
			double diff = lin - prev;
			
			prev = lin;
			
			SInt16  m = lround( diff );
			
			if ( m < 1 )
				m = 1;
				
			logtabs[ n - 10 ][i] = m;
		}
	}
}

/*---------------------------------***  CentreRects  ***--------------------------------*/
/*
access:			global
overrides:		
description: 	centre a rectangle with respct to reference rect
ins: 			<theRect> rectangle to centre
				<refRect> reference rectangle
outs: 			none
notes:			does not change size of the rectangle
----------------------------------------------------------------------------------------*/

void	CentreRects( const Rect* refRect, Rect* theRect )
{
	OffsetRect( theRect, -theRect->left, -theRect->top );
	OffsetRect( theRect, (( refRect->left + refRect->right  ) / 2) - ( theRect->right  / 2 ),
						 (( refRect->top  + refRect->bottom ) / 2) - ( theRect->bottom / 2 ));
}






ITBarDisplay::ITBarDisplay( const Rect& visualBounds, const UInt16 numChannels )
{
	BuildLogTables();
	
	channels = MIN( kMaxChannels, MAX( numChannels, kMinChannels ));
	repaint = true;
	
	for( int i = 0; i < kMaxChannels; i++ )
	{
		left[i] = NULL;
		right[i] = NULL;
	}
	
	leftVU = rightVU = NULL;
	
	Init( visualBounds );
}


ITBarDisplay::~ITBarDisplay()
{
	ITBarGraph* bar;
	
	for( int i = 0; i < kMaxChannels; i++ )
	{
		bar = left[i];
		
		if ( bar )
			delete bar;
			
		left[i] = NULL;
	}
	
	for( int i = 0; i < kMaxChannels; i++ )
	{
		bar = right[i];
		
		if ( bar )
			delete bar;
			
		right[i] = NULL;
	}
	
	if ( leftVU )
		delete leftVU;
		
	if ( rightVU )
		delete rightVU;
}



void			ITBarDisplay::Init( const Rect& visBounds )
{
	Rect		br, rr;
	SInt16		dxl, dyl, dxr, dyr;
	SInt16		barsize;
	UInt16		barflagleft, barflagright;
	
	br = fieldBounds = visBounds;
	
	if (( layout & kLayoutFixedSize ) != 0 )
	{
		if (( layout & kLayoutFixedSizeLarger ) != 0 )
			SetRect( &bounds, 0, 0, 800, 178 );
		else
			SetRect( &bounds, 0, 0, 560, 125 );
		CentreRects( &visBounds, &bounds );
		SectRect( &bounds, &fieldBounds, &bounds );
		OffsetRect( &bounds, -bounds.left, -bounds.top );
		CalcRects( &br );
		CentreRects( &visBounds, &br );
	}
	else
	{
		SetRect( &bounds, 0, 0, fieldBounds.right - fieldBounds.left - 40, ( fieldBounds.bottom - fieldBounds.top ) / 2 );
		CalcRects( &br );
		CentreRects( &visBounds, &br );

		if (( br.bottom - br.top ) < 100 )
		{
			SetRect( &bounds, 0, 0, fieldBounds.right - fieldBounds.left - 10, 100 );
			CalcRects( &br );
		}
	}
	
	bounds = br;
	SectRect( &visBounds, &bounds, &bounds );
	OffsetRect( &slr, bounds.left, bounds.top );
	OffsetRect( &srr, bounds.left, bounds.top );
	OffsetRect( &fslr, bounds.left, bounds.top );
	OffsetRect( &fsrr, bounds.left, bounds.top );
	OffsetRect( &vulr, bounds.left, bounds.top );
	OffsetRect( &vurr, bounds.left, bounds.top );
	OffsetRect( &dbsr, bounds.left, bounds.top );
	SectRect( &slr, &bounds, &slr );
	SectRect( &srr, &bounds, &srr );
	SectRect( &fslr, &bounds, &fslr );
	SectRect( &fsrr, &bounds, &fsrr );
	SectRect( &vulr, &bounds, &vulr );
	SectRect( &vurr, &bounds, &vurr );
	SectRect( &dbsr, &bounds, &dbsr );
	
	barflagleft = barflagright = 0;

	// layout the bars according to the preferred option
	
	if (( layout & 3 ) == kLayoutBackToBack )
	{
		barsize = ( slr.bottom - slr.top ) / channels;
		
		br = slr;
		rr = srr;
		br.top = br.bottom - barsize;
		rr.top = rr.bottom - barsize;

		dxl = dxr = 0;
		dyl = dyr = -barsize;
		barflagleft += kInvertOrientation;
		
	}
	else
	{
		// side by side or analogue layout
		
		barsize = ( slr.right - slr.left ) / channels;
		
		br = slr;
		rr = srr;
		
		br.left = br.right - barsize;
		rr.right = rr.left + barsize;
		dxl = -barsize;
		dxr = barsize;
		dyl = dyr = 0;
	}
	
	if ( IsInAnalogueMode())
	{
		// analogue meters handled differently - only two meters are built and they occupy the full
		// area of the bar display regions. left[0] and right[0] hold pointers to these
		
		br = slr;
		InsetRect( &br, 3, 3 );
		
		left[0] = new ITAnalogueMeter( br, spectrumSegments, 0 );
		left[0]->SetColour( barColour, kBarColour );
		left[0]->SetColour( altBarColour, kAltBarColour );
		left[0]->SetColour( peakColour, kPeakColour );
		left[0]->SetColourMode( colourMode );
		
		rr = srr;
		InsetRect( &rr, 3, 3 );
		
		right[0] = new ITAnalogueMeter( rr, spectrumSegments, 0 );
		right[0]->SetColour( barColour, kBarColour );
		right[0]->SetColour( altBarColour, kAltBarColour );
		right[0]->SetColour( peakColour, kPeakColour );
		right[0]->SetColourMode( colourMode );
	}
	else
	{
		for( int i = 0; i < channels; i++ )
		{
			left[i] = new ITBarGraph( br, spectrumSegments, barflagleft );
			OffsetRect( &br, dxl, dyl );
			// set up colours based on our local settings
			left[i]->SetColour( barColour, kBarColour );
			left[i]->SetColour( altBarColour, kAltBarColour );
			left[i]->SetColour( peakColour, kPeakColour );
			left[i]->SetColourMode( colourMode );
			
			right[i] = new ITBarGraph( rr, spectrumSegments, barflagright );
			OffsetRect( &rr, dxr, dyr );
			right[i]->SetColour( barColour, kBarColour );
			right[i]->SetColour( altBarColour, kAltBarColour );
			right[i]->SetColour( peakColour, kPeakColour );
			right[i]->SetColourMode( colourMode );
		}
	}
	
	if ( vuenabled )
	{
		br = vulr;
		rr = vurr;
		
		leftVU = new ITBarGraph( br, vuSegments, barflagleft | kInvertOrientation );
		rightVU = new ITBarGraph( rr, vuSegments, barflagright );
		
		leftVU->SetColour( vuBarColour, kBarColour );
		leftVU->SetColour( vuAltBarColour, kAltBarColour );
		leftVU->SetColour( vuPeakColour, kPeakColour );
		rightVU->SetColour( vuBarColour, kBarColour );
		rightVU->SetColour( vuAltBarColour, kAltBarColour );
		rightVU->SetColour( vuPeakColour, kPeakColour );
		
		leftVU->SetColourMode( colourMode );
		rightVU->SetColourMode( colourMode );
	}
	
	// ensure channel flips match static flags:
	
	if ( flipleft )
	{
		flipleft = !flipleft;
		SetChannelFlip( kLeftChannel, true );
	}
	if ( flipright )
	{
		flipright = !flipright;
		SetChannelFlip( kRightChannel, true );
	}
	repaint = true;
}


void			ITBarDisplay::CalcRects( Rect* outBoundingRect )
{
	// calculates the layout rects, given the current value of bounds. It returns the rect enclosing the lot,
	// which may exceed bounds by a small amount. Bounds should subsequently adjusted to enclose this.
	
	enum
	{
		kVerticalMeterFixedWidth		= 30,
		kHorizontalMeterFixedHeight		= 12,
		kVUMeterFixedHeight				= 22,
		kVerticalGapFixedWidth			= 8,
		kHorizontalGapFixedHeight		= 4
	};
	
	SetRect( &dbsr, 0, 0, 0, 0 );
	SetRect( &fslr, 0, 0, 0, 0 );
	SetRect( &fsrr, 0, 0, 0, 0 );
	SetRect( &vulr, 0, 0, 0, 0 );
	SetRect( &vurr, 0, 0, 0, 0 );
	
	int h = bounds.bottom - bounds.top;
	
	if (( layout & kLayoutBackToBack ) != 0 )
		h -= ( h % channels );
	else
		h -= ( h % spectrumSegments );
	
	bounds.bottom = bounds.top + h;
	
	// fixed central area:
	
	if ( showScales )
	{
		bounds.bottom += kHorizontalMeterFixedHeight;
		bounds.right += kVerticalMeterFixedWidth;
		SetRect( &dbsr, 0, 0, kVerticalMeterFixedWidth, bounds.bottom - bounds.top );
	}
	else
		SetRect( &dbsr, 0, 0, kVerticalGapFixedWidth, h );
		
	if ( vuenabled )
		bounds.bottom += kVUMeterFixedHeight;
	
	// find out the size for the spectrum areas - this is done so that a whole number of channels will
	// fit precisely
	
	int sw, w = ( bounds.right - bounds.left - dbsr.right ) / 2;
	
	if (( layout & kLayoutBackToBack ) != 0 )
		sw = w - ( w % spectrumSegments );
	else
		sw = w - ( w % channels );
	
	if ( showScales )
	{
		SetRect( &fslr, 0, 0, sw, kHorizontalMeterFixedHeight );
		SetRect( &fsrr, 0, 0, sw, kHorizontalMeterFixedHeight );
	}
	else
	{
		SetRect( &fslr, 0, 0, sw, kHorizontalGapFixedHeight );
		SetRect( &fsrr, 0, 0, sw, kHorizontalGapFixedHeight );
	}
	
	if ( vuenabled )
	{
		SetRect( &vulr, 0, 0, sw + 1, kVUMeterFixedHeight );
		SetRect( &vurr, 0, 0, sw, kVUMeterFixedHeight );
	}
	else
	{
		SetRect( &vulr, 0, 0, sw, 0 );
		SetRect( &vurr, 0, 0, sw, 0 );
	}
		
	SetRect( &slr, 0, 0, sw, h );
	SetRect( &srr, 0, 0, sw, h );
	
	// got all the sizes, now position everything
	
	OffsetRect( &srr, sw + dbsr.right, 0 );
	OffsetRect( &fsrr, sw + dbsr.right, h );
	OffsetRect( &vurr, sw + dbsr.right, fsrr.bottom );
	OffsetRect( &dbsr, sw, 0 );
	OffsetRect( &fslr, 0, h );
	OffsetRect( &vulr, 0, fslr.bottom );
	
	outBoundingRect->top = outBoundingRect->left = 0;
	outBoundingRect->bottom = vulr.bottom;
	outBoundingRect->right = vurr.right;
}


void			ITBarDisplay::Update( const UInt8 spectrumArray[2][512], const bool isFullScreen )
{
	CGrafPtr port;
	
	GetPort( &port );

	if ( repaint )
	{
		Rect br = bounds;
		
		InsetRect( &br, -2, -2 );
		br.top -= 1;

		RGBForeColor( &RGB_BLACK );
		PaintRect( &br );
				
		if ( showScales )
			DrawMeterScales();

		if ( border )
		{
			RGBForeColor( &RGB_DARKMIDGRAY );
			FrameRect( &br );
		}
		repaint = false;
	}

	RGBForeColor( &RGB_BLACK );
	
	// set dirty rect to whole bounds
	
	QDAddRectToDirtyRegion( port, &bounds );
	LockPortBits( port );
	
	// calculate binning required to cover spectrum with number of available channels.
		
	int		slotsperbin; // = 256 / channels;
	int		n, k = 0;
	double  leftvalue, rightvalue;
	double  vuLeft, vuRight;
	
	vuLeft = vuRight = 0;
	
	if ( IsInAnalogueMode())
	{
		if ( spectrumArray )
		{
			n = 256;
			while( n-- )
			{
				vuLeft += spectrumArray[0][k];
				vuRight += spectrumArray[1][k++];
			}
		}

		if ( left[0] )
			left[0]->Update( roundtol( vuLeft / 256.0 ), isFullScreen );
				
		if ( right[0] )
			right[0]->Update( roundtol( vuRight / 256.0 ), isFullScreen );
	}
	else
	{
		for( int i = 0; i < channels; i++ )
		{
			// calculate value by averaging data into bins. This should be
			// done as a geometric series calculated to cover the available band in the
			// exact number of steps, but for speed we use the appropriate precalculated
			// table
			
			slotsperbin = logtabs[channels - 10][i];
			leftvalue = rightvalue = 0;
			
			if ( spectrumArray )
			{
				n = slotsperbin;
				while( n-- )
				{
					leftvalue += spectrumArray[0][k];
					rightvalue += spectrumArray[1][k++];
				}
			}
			
			vuLeft += leftvalue;
			vuRight += rightvalue;	
			
			if ( left[i] )
				left[i]->Update( roundtol( leftvalue / slotsperbin ), isFullScreen );
				
			if ( right[i] )
				right[i]->Update( roundtol( rightvalue / slotsperbin ), isFullScreen );
		}
	}
	
	// show vu levels if enabled
	
	if ( vuenabled )
	{
		if ( leftVU )
			leftVU->Update( roundtol( vuLeft / 256.0 ), isFullScreen );
				
		if ( rightVU )
			rightVU->Update( roundtol( vuRight / 256.0 ), isFullScreen );
	}
	
	UnlockPortBits( port );
	
	if ( colourMode == kAnimatedColours && !IsInAnalogueMode())
		DoColourAnimation();
}


void			ITBarDisplay::UpdateColours()
{
	// copies fixed static colour values to the active bars on demand. Also updates the colour mode flag
	
	for( int i = 0; i < channels; i++ )
	{
		if ( left[i] )
		{
			left[i]->SetColourMode( colourMode );
			
			left[i]->SetColour( barColour, kBarColour );
			left[i]->SetColour( peakColour, kPeakColour );
			left[i]->SetColour( altBarColour, kAltBarColour );
		}	
		
		if ( right[i] )
		{
			right[i]->SetColourMode( colourMode );
			
			right[i]->SetColour( barColour, kBarColour );
			right[i]->SetColour( peakColour, kPeakColour );
			right[i]->SetColour( altBarColour, kAltBarColour );
		}
	}
	
	if ( vuenabled )
	{
		if ( leftVU )
		{
			leftVU->SetColourMode( colourMode );
			leftVU->SetColour( vuBarColour, kBarColour );
			leftVU->SetColour( vuPeakColour, kPeakColour );
			leftVU->SetColour( vuAltBarColour, kAltBarColour );
		}
		if ( rightVU )
		{
			rightVU->SetColourMode( colourMode );
			rightVU->SetColour( vuBarColour, kBarColour );
			rightVU->SetColour( vuPeakColour, kPeakColour );
			rightVU->SetColour( vuAltBarColour, kAltBarColour );
		}
	}
}


void			ITBarDisplay::SetMeterType( const MeterType newType )
{
	switch ( newType )
	{
		case kLinear:
			ITBarGraph::SetLogMode( false );
			break;
			
		case kLogarithmic:
			ITBarGraph::SetLogMode( true );
			break;
	}
}


const MeterType ITBarDisplay::GetMeterType()
{
	if ( ITBarGraph::IsInLogMode())
		return kLogarithmic;
	else
		return kLinear;
}


void			ITBarDisplay::SetPeakEnabled( const bool isEnabled )
{
	ITBarGraph::peakEnable = isEnabled;
}


void			ITBarDisplay::SetDecayTime( const Bar whichBar, const UInt16 newTime )
{
	switch( whichBar )
	{
		case kMainBar:
			ITBarGraph::barTimeConstant = newTime;
			break;
			
		case kPeakHold:
			ITBarGraph::peakHoldTime = newTime;
			break;
			
		case kPeakDecay:
			ITBarGraph::peakTimeConstant = newTime;
			break;
	}
}


const UInt16	ITBarDisplay::GetDecayTime( const Bar whichBar )
{
	UInt16 result = 0;
	
	switch( whichBar )
	{
		case kMainBar:
			result = ITBarGraph::barTimeConstant;
			break;
			
		case kPeakHold:
			result = ITBarGraph::peakHoldTime;
			break;
			
		case kPeakDecay:
			result = ITBarGraph::peakTimeConstant;
			break;
	}
	
	return result;
}


void			ITBarDisplay::EnableBorder( const bool isEnabled )
{
	repaint = true;
	border = isEnabled;
}


void			ITBarDisplay::EnableVUMeters( const bool showThem )
{
	vuenabled = showThem;
}


void			ITBarDisplay::SetChannelFlip( const Channel which, const bool isFlipped )
{
	bool flag;
	
	if (( layout & 3 ) == kLayoutAnalogueVUMeters )
		return;
	
	if ( which == kLeftChannel )
		flag = flipleft;
	else
		flag = flipright;
	
	if ( isFlipped != flag )
	{
		// need to reverse the order of the bars in the appropriate array
		
		ITBarGraph* temp[ kMaxChannels ];
		
		// copy bars to temp:
		
		int k, i;
		
		for ( i = 0; i < channels; i++ )
		{
			if ( which == kLeftChannel )
				temp[i] = left[i];
			else
				temp[i] = right[i];
		}	
		// copy them back in reverse order:
		k = 0;
		
		for( i = channels - 1; i >= 0; i-- )
		{
			if ( which == kLeftChannel )
				left[k++] = temp[i];
			else
				right[k++] = temp[i];
		}
		
		if ( which == kLeftChannel )
			flipleft = isFlipped;
		else
			flipright = isFlipped;
			
		if ( showScales )
		{
			CGrafPtr	port;
			
			GetPort( &port );
			InvalWindowRect( GetWindowFromPort( port ), &fieldBounds );
		}
	}
}


void		ITBarDisplay::SetFixedColour( const RGBColor theColour, const UInt16 which )
{
	switch( which )
	{
		case kBarColour:
			barColour = theColour;
			break;
			
		case kPeakColour:
			peakColour = theColour;
			break;
			
		case kAltBarColour:
			altBarColour = theColour;
			break;
			
		case kVUBarColour:
			vuBarColour = theColour;
			break;
			
		case kVUPeakColour:
			vuPeakColour = theColour;
			break;
			
		case kVUAltBarColour:
			vuAltBarColour = theColour;
			break;
	}
}

RGBColor	ITBarDisplay::GetFixedColour( const UInt16 which )
{
	RGBColor c = RGB_BLACK;
	
	switch( which )
	{
		case kBarColour:
			c = barColour;
			break;
			
		case kPeakColour:
			c = peakColour;
			break;
			
		case kAltBarColour:
			c = altBarColour;
			break;
			
		case kVUBarColour:
			c = vuBarColour;
			break;
			
		case kVUPeakColour:
			c = vuPeakColour;
			break;
			
		case kVUAltBarColour:
			c = vuAltBarColour;
			break;
	}
	return c;
}


void		ITBarDisplay::SetPaletteID( const SInt16 newID )
{
	if ( newID != paletteID )
	{
		if ( gAnimationTable )
			DisposeCTable( gAnimationTable );
			
		paletteID = newID;
		
		gAnimationTable = GetCTable( paletteID );
		
		if ( gAnimationTable == NULL )
		{
			gAnimationTable = GetCTable( 8 );
			paletteID = 8;
		}
	}
}


void		ITBarDisplay::DoColourAnimation()
{
	// change bar colours at intervals
	
	static UInt32		timr = 0;
	
	UInt32 t = TickCount();
	
	if ( t > timr )
	{
		timr = t + kColourAnimationInterval;
		
		// OK, we have work to do
		
		if ( gAnimationTable )
		{
			static SInt16   tabPhase = 0;
			static SInt16   altPhase = 40;
			static SInt16   peakPhase = 22;
			
			SInt16			tabSize, tab, alttab, peaktab;
			
			tabSize = (*gAnimationTable)->ctSize;
			
			for( int i = 0; i < channels; i++ )
			{
				tab = ( tabPhase + i ) % tabSize;
				alttab = ( altPhase + i ) % tabSize;
				peaktab = ( peakPhase + i ) % tabSize;
				
				if ( left[i] )
				{
					left[i]->SetColour((*gAnimationTable)->ctTable[tab].rgb, kBarColour );
					left[i]->SetColour((*gAnimationTable)->ctTable[alttab].rgb, kAltBarColour );
					left[i]->SetColour((*gAnimationTable)->ctTable[peaktab].rgb, kPeakColour );
				}
			
				if ( right[i] )
				{
					right[i]->SetColour((*gAnimationTable)->ctTable[tab].rgb, kBarColour );
					right[i]->SetColour((*gAnimationTable)->ctTable[alttab].rgb, kAltBarColour );
					right[i]->SetColour((*gAnimationTable)->ctTable[peaktab].rgb, kPeakColour );
				}
			}
			
			if ( vuenabled )
			{
				if ( leftVU )
				{
					leftVU->SetColour((*gAnimationTable)->ctTable[tab].rgb, kBarColour );
					leftVU->SetColour((*gAnimationTable)->ctTable[alttab].rgb, kAltBarColour );
					leftVU->SetColour((*gAnimationTable)->ctTable[peaktab].rgb, kPeakColour );
				}
				
				if ( rightVU )
				{
					rightVU->SetColour((*gAnimationTable)->ctTable[tab].rgb, kBarColour );
					rightVU->SetColour((*gAnimationTable)->ctTable[alttab].rgb, kAltBarColour );
					rightVU->SetColour((*gAnimationTable)->ctTable[peaktab].rgb, kPeakColour );
				}
			}
			
			altPhase = ( altPhase + 1 ) % tabSize;
			peakPhase = ( peakPhase + 1 ) % tabSize;
			
			if (( altPhase & 1 ) == 0 )
				tabPhase = ( tabPhase + 1 ) % tabSize;
		}
	}
}

static const char* fs[] = { "60", "125", "250", "500", "1k", "2k", "5k", "10k", "20k" };
static const char* db[] = { "0dB", "-8", "-16", "-24", "-32", "-40", "-48" };



void			ITBarDisplay::DrawMeterScales()
{
	// draw scales within the precalculated rects
	
	Rect			trl, trr;
	SInt16			just = teJustCenter;
	CFStringRef		cs;
	ThemeFontID		fnt;
	bool			sbs = (layout & kLayoutBackToBack ) != 0;
	
	RGBForeColor( &scaleColour );
	
	// on 10.3 or later, use mini font, otherwise use small font
	
	fnt = kThemeMiniSystemFont;
	
	long  sysVers;
	
	OSErr err = Gestalt( gestaltSystemVersion, &sysVers );
	
	if ( err == noErr && sysVers < 0x00001030 )
		fnt = kThemeSmallSystemFont;
	
	if ( sbs )
	{
		float   ofs = ( fslr.right - fslr.left ) / 7.0;
		Str15   ss;
		
		for ( int i = 0; i < 7; i++ )
		{
			SetRect( &trl, 0, 0, 32, 14 );
			trr = trl;
			
			OffsetRect( &trl, fslr.left + rinttol( ofs * (float) i ), fslr.top );
			OffsetRect( &trr, fsrr.right - 32 -rinttol( ofs * (float) i ), fsrr.top );
			
			
			if ( ITBarGraph::IsInLogMode())
				cs = CFStringCreateWithCString( NULL, db[ i ], kCFStringEncodingMacRoman );
			else
			{
				NumToString( 7 - i, ss );
				cs = CFStringCreateWithPascalString( NULL, ss, kCFStringEncodingMacRoman );
			}
			
			DrawThemeTextBox( cs, fnt, kThemeStateActive, false, &trl, just, NULL );
			CFRelease( cs );
			
			if ( ITBarGraph::IsInLogMode())
				cs = CFStringCreateWithCString( NULL, db[ i ], kCFStringEncodingMacRoman );
			else
				cs = CFStringCreateWithPascalString( NULL, ss, kCFStringEncodingMacRoman );
				
			DrawThemeTextBox( cs, fnt, kThemeStateActive, false, &trr, just, NULL );
			CFRelease( cs );
		}
	}
	else
	{
		float ofs = ( fslr.right - fslr.left ) / 9.0;
		
		for ( int i = 0; i < 9; i++ )
		{
			SetRect( &trl, 0, 0, 32, 14 );
			trr = trl;
			
			OffsetRect( &trl, fslr.left + rinttol( ofs * (float) i ), fslr.top );
			OffsetRect( &trr, fsrr.right - 32 -rinttol( ofs * (float) i ), fsrr.top );

			if ( flipleft )
				cs = CFStringCreateWithCString( NULL, fs[ i ], kCFStringEncodingMacRoman );
			else
				cs = CFStringCreateWithCString( NULL, fs[ 8 - i ], kCFStringEncodingMacRoman );
			
			DrawThemeTextBox( cs, fnt, kThemeStateActive, false, &trl, just, NULL );
			CFRelease( cs );

			if ( flipright )
				cs = CFStringCreateWithCString( NULL, fs[ i ], kCFStringEncodingMacRoman );
			else
				cs = CFStringCreateWithCString( NULL, fs[ 8 - i ], kCFStringEncodingMacRoman );
			
			DrawThemeTextBox( cs, fnt, kThemeStateActive, false, &trr, just, NULL );
			CFRelease( cs );
		}
	}
	// fixed labels:
	
	trl = dbsr;
	trl.top = fslr.top;
	if ( sbs )
	{
		if ( ITBarGraph::IsInLogMode())
			DrawThemeTextBox( CFSTR("dB"), fnt, kThemeStateActive, false, &trl, teJustCenter, NULL );
	}
	else
		DrawThemeTextBox( CFSTR("Hz"), fnt, kThemeStateActive, false, &trl, teJustCenter, NULL );
	
	if ( vuenabled )
	{
		trl.top = vulr.top + 4;
		trl.bottom = bounds.bottom;
		DrawThemeTextBox( CFSTR("VU"), fnt, kThemeStateActive, false, &trl, teJustCenter, NULL );
	}
	
	// vertical dB scale:
	if ( ! sbs )
	{
		float o = ( slr.bottom - slr.top - 14 ) / 6.0;
		
		for ( int i = 0; i < 7; i++ )
		{
			trl = dbsr;
			trl.bottom = dbsr.top + 14;
		
			OffsetRect( &trl, 0, rinttol( o * (float) i ));
			
			if ( ITBarGraph::IsInLogMode())
				cs = CFStringCreateWithCString( NULL, db[ i ], kCFStringEncodingMacRoman );
			else
			{
				Str15   s;
				
				NumToString( 7 - i, s );
				cs = CFStringCreateWithPascalString( NULL, s, kCFStringEncodingMacRoman );
			}	
			DrawThemeTextBox( cs, fnt, kThemeStateActive, false, &trl, teJustCenter, NULL );
			CFRelease( cs );
		}
	}
}


void	ITBarDisplay::InvalMeterAreas()
{
	if ( showScales )
	{
		CGrafPtr	port;
		
		GetPort( &port );
		InvalWindowRect( GetWindowFromPort( port ), &dbsr );
		InvalWindowRect( GetWindowFromPort( port ), &fslr );
		InvalWindowRect( GetWindowFromPort( port ), &fsrr );
	}
}



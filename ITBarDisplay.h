/*
 *  ITBarDisplay.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Tue Mar 02 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include "ITAnalogueMeter.h"


enum
{
	kMinChannels				= 1,		// only used for analogue mode - normal min is 10
	kMaxChannels				= 32,
	kColourAnimationInterval	= 20		// in ticks
};

// this class collects a series of bargraphs into a display

enum
{
	kLinear				= 1,
	kLogarithmic		= 2
};

typedef UInt8 MeterType;

enum
{
	kMainBar			= 0,
	kPeakHold			= 1,
	kPeakDecay			= 2
};

typedef UInt8 Bar;

enum
{
	kLeftChannel		= 1,
	kRightChannel		= 2
};

typedef UInt8 Channel;

// different supported layout options:

enum
{
	kLayoutSideBySide			= 0,
	kLayoutBackToBack			= 1,
	kLayoutAnalogueVUMeters		= 2,
	kLayoutFixedSize			= 4,
	kLayoutFixedSizeLarger		= 8
};

typedef UInt16 Layout;




class ITBarDisplay
{
public:
	ITBarDisplay( const Rect& visualBounds, const UInt16 numChannels );
	virtual ~ITBarDisplay();
	
	static Layout   GetLayout(){ return layout; };
	static void		SetLayout( const Layout newLayout ){ layout = newLayout; };
	
	void			Init( const Rect& visBounds );
	void			CalcRects( Rect* outBoundingRect );
	void			Update( const UInt8 spectrumArray[2][512], const bool isFullScreen = false );
	
	void			SetForceRepaint( const bool forceIt ){ repaint = forceIt; };
	void			SetChannelFlip( const Channel which, const bool isFlipped );
	void			EnableBorder( const bool isEnabled );
	void			UpdateColours();
	void			DoColourAnimation();
	void			DrawMeterScales();
	void			InvalMeterAreas();

	
	void			GetBounds( Rect* inRect ){ *inRect = bounds; };

	static void				SetPeakEnabled( const bool isEnabled );
	static const bool		IsPeakEnabled(){ return ITBarGraph::peakEnable; };
	static void				SetMeterType( const MeterType newType );
	static const MeterType  GetMeterType();
	static void				SetDecayTime( const Bar whichBar, const UInt16 newTime );
	static const UInt16		GetDecayTime( const Bar whichBar );
	static const bool		IsBorderEnabled(){ return border; };
	static void				EnableVUMeters( const bool showThem );
	static const bool		VUMetersVisible(){ return vuenabled; };
	 
	static const bool		IsLeftChannelFlipped(){ return flipleft; };
	static const bool		IsRightChannelFlipped(){ return flipright; };
	static void				SetLeftFlip( const bool flip ){ flipleft = flip; };
	static void				SetRightFlip( const bool flip ){ flipright = flip; };
	static void				SetBorder( const bool onoff ){ border = onoff; };
	
	static void				SetSpectrumSegments( const UInt16 segs ){ spectrumSegments = segs; };
	static UInt16			GetSpectrumSegments(){ return spectrumSegments; };
	static void				SetVUSegments( const UInt16 segs ){ vuSegments = segs; };
	static UInt16			GetVUSegments(){ return vuSegments; };
	
	static void				SetColourMode( const ColourMode newMode ){ colourMode = newMode; };
	static ColourMode		GetColourMode(){ return colourMode; };
	static void				SetFixedColour( const RGBColor theColour, const UInt16 which );
	static RGBColor			GetFixedColour( const UInt16 which );
	static void				SetPaletteID( const SInt16 newID );
	static SInt16			GetPaletteID(){ return paletteID; };
	
	static void				ShowHideScales( const bool showIt ){ showScales = showIt; };
	static bool				IsScalesVisible(){ return showScales; };
	static bool				IsInAnalogueMode(){ return (( layout & 3 ) == kLayoutAnalogueVUMeters ); };
	
private:
	UInt16				channels;
	ITBarGraph*			left[kMaxChannels];
	ITBarGraph*			right[kMaxChannels];
	ITBarGraph*			leftVU;
	ITBarGraph*			rightVU;
	Rect				bounds;
	Rect				fieldBounds;
	bool				repaint;
	Rect				slr;			// spectrum area (left)
	Rect				srr;			// spectrum area (right)
	Rect				fslr;			// frequency scale left area
	Rect				fsrr;			// frequency scale right area
	Rect				vulr;			// vu left area
	Rect				vurr;			// vu right area
	Rect				dbsr;			// meter scale central area
	
	static Layout		layout;
	static bool			border;
	static bool			vuenabled;
	static bool			flipleft;
	static bool			flipright;
	static bool			showScales;
	static UInt16		spectrumSegments;
	static UInt16		vuSegments;
	static ColourMode   colourMode;
	static RGBColor		barColour;
	static RGBColor		altBarColour;
	static RGBColor		peakColour;
	static RGBColor		vuBarColour;
	static RGBColor		vuAltBarColour;
	static RGBColor		vuPeakColour;
	static RGBColor		scaleColour;
	static SInt16		paletteID;
};

//misc:

extern void	CentreRects( const Rect* refRect, Rect* theRect );


#define MIN( a, b )		((a) < (b))? (a) : (b)
#define MAX( a, b )		((a) > (b))? (a) : (b)
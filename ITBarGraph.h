/*
 *  ITBarGraph.h
 *  iTunesXPlugIn
 *
 *  Created by graham on Tue Mar 02 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>

enum
{
	kFixedColours				= 1,
	kGraduatedColours			= 2,
	kGraduatedToMax				= 3,
	kAnimatedColours			= 4
};

typedef UInt8 ColourMode;



// this class implements a single bargraph, either vertical or horizontal, according to its
// aspect ratio


class ITBarGraph
{
public:
	ITBarGraph( const Rect& bounds, const UInt16 numSegments = 16, const UInt16 options = 0 );
	virtual ~ITBarGraph();
	
	void			SetBounds( const Rect& newBounds );
	void			SetValue( const UInt16 newValue );
	
	void			SetColourMode( const SInt16 mode ){ colourMode = mode; };
	SInt16			GetColourMode(){ return colourMode; };
	void			SetInverted( const bool isInverted ){ inverted = isInverted; };
	bool			IsInverted(){ return inverted; };
	
	void			Update( const UInt16 newValue, const bool doErase = false );
	void			Erase();
	virtual void	Draw( const bool doErase = false );
	virtual void	SetColour( const RGBColor& theColour, const UInt16 which );
	
	void			RedrawQD( const bool doErase );
	void			Redraw32( PixMapHandle portPixMap, const bool doErase );
	void			Redraw16( PixMapHandle portPixMap, const bool doErase );
	
	
	static void		SetLogMode( const bool inLogMode ){ logMode = inLogMode; };
	static bool		IsInLogMode(){ return logMode; };
	

	static UInt16		peakHoldTime;
	static bool			peakEnable;
	static bool			expDecay;
	static RGBColor		backgroundRGB;
	static UInt16		barTimeConstant;
	static UInt16		peakTimeConstant;


protected:
	SInt16			value;
	SInt16			peakValue;
	SInt16			Avalue;
	SInt16			Pvalue;
	SInt16			colourMode;
	bool			inverted;
	UInt16			segments;
	UInt16			fullScaleDeflection;
	UInt16			valuePerSegment;
	RGBColor		altBarRGB;
	RGBColor		barRGB;
	RGBColor		peakRGB;
	Rect			segmentRect;
	Rect			bounds;
	SInt16			segmentSpacing;
	bool			isVertical;
	UInt32			barDecayTime;
	UInt32			peakDecayTime;
	UInt32			peakHoldTimeCount;
	
	
	static bool		logMode;
	static double   logMultiplier;

};


// mode flags:

enum
{
	kComputedColours	= 1,
	kInvertOrientation  = 4
};

// RGB index

enum
{
	kBackgroundColour   = 1,
	kBarColour			= 2,
	kPeakColour			= 3,
	kAltBarColour		= 4,
	kVUBarColour		= ( kBarColour    | 32 ),
	kVUPeakColour		= ( kPeakColour   | 32 ),
	kVUAltBarColour		= ( kAltBarColour | 32 )
};


extern const RGBColor RGB_BLACK;
extern const RGBColor RGB_WHITE;
extern const RGBColor RGB_BRIGHT_GREEN;
extern const RGBColor RGB_BRIGHT_RED;
extern const RGBColor RGB_ICE_BLUE;
extern const RGBColor RGB_BRIGHT_BLUE;
extern const RGBColor RGB_YELLOW;
extern const RGBColor RGB_MAGENTA;
extern const RGBColor RGB_VERY_DARK_GRAY;

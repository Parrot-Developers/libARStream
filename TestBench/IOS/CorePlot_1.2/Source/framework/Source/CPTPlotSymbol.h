/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/// @file

@class CPTLineStyle;
@class CPTFill;
@class CPTShadow;

/**
 *  @brief Plot symbol types.
 **/
typedef enum _CPTPlotSymbolType {
    CPTPlotSymbolTypeNone,      ///< No symbol.
    CPTPlotSymbolTypeRectangle, ///< Rectangle symbol.
    CPTPlotSymbolTypeEllipse,   ///< Elliptical symbol.
    CPTPlotSymbolTypeDiamond,   ///< Diamond symbol.
    CPTPlotSymbolTypeTriangle,  ///< Triangle symbol.
    CPTPlotSymbolTypeStar,      ///< 5-point star symbol.
    CPTPlotSymbolTypePentagon,  ///< Pentagon symbol.
    CPTPlotSymbolTypeHexagon,   ///< Hexagon symbol.
    CPTPlotSymbolTypeCross,     ///< X symbol.
    CPTPlotSymbolTypePlus,      ///< Plus symbol.
    CPTPlotSymbolTypeDash,      ///< Dash symbol.
    CPTPlotSymbolTypeSnow,      ///< Snowflake symbol.
    CPTPlotSymbolTypeCustom     ///< Custom symbol.
}
CPTPlotSymbolType;

@interface CPTPlotSymbol : NSObject<NSCoding, NSCopying> {
    @private
    CGPoint anchorPoint;
    CGSize size;
    CPTPlotSymbolType symbolType;
    CPTLineStyle *lineStyle;
    CPTFill *fill;
    CGPathRef cachedSymbolPath;
    CGPathRef customSymbolPath;
    BOOL usesEvenOddClipRule;
    CGLayerRef cachedLayer;
    CPTShadow *shadow;
}

@property (nonatomic, readwrite, assign) CGPoint anchorPoint;
@property (nonatomic, readwrite, assign) CGSize size;
@property (nonatomic, readwrite, assign) CPTPlotSymbolType symbolType;
@property (nonatomic, readwrite, retain) CPTLineStyle *lineStyle;
@property (nonatomic, readwrite, retain) CPTFill *fill;
@property (nonatomic, readwrite, copy) CPTShadow *shadow;
@property (nonatomic, readwrite, assign) CGPathRef customSymbolPath;
@property (nonatomic, readwrite, assign) BOOL usesEvenOddClipRule;

/// @name Factory Methods
/// @{
+(CPTPlotSymbol *)plotSymbol;
+(CPTPlotSymbol *)crossPlotSymbol;
+(CPTPlotSymbol *)ellipsePlotSymbol;
+(CPTPlotSymbol *)rectanglePlotSymbol;
+(CPTPlotSymbol *)plusPlotSymbol;
+(CPTPlotSymbol *)starPlotSymbol;
+(CPTPlotSymbol *)diamondPlotSymbol;
+(CPTPlotSymbol *)trianglePlotSymbol;
+(CPTPlotSymbol *)pentagonPlotSymbol;
+(CPTPlotSymbol *)hexagonPlotSymbol;
+(CPTPlotSymbol *)dashPlotSymbol;
+(CPTPlotSymbol *)snowPlotSymbol;
+(CPTPlotSymbol *)customPlotSymbolWithPath:(CGPathRef)aPath;
/// @}

/// @name Drawing
/// @{
-(void)renderInContext:(CGContextRef)context atPoint:(CGPoint)center scale:(CGFloat)scale alignToPixels:(BOOL)alignToPixels;
-(void)renderAsVectorInContext:(CGContextRef)context atPoint:(CGPoint)center scale:(CGFloat)scale;
/// @}

@end

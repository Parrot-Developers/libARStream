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

/**
 *  @brief Line cap types.
 **/
typedef enum _CPTLineCapType {
    CPTLineCapTypeNone,       ///< No line cap.
    CPTLineCapTypeOpenArrow,  ///< Open arrow line cap.
    CPTLineCapTypeSolidArrow, ///< Solid arrow line cap.
    CPTLineCapTypeSweptArrow, ///< Swept arrow line cap.
    CPTLineCapTypeRectangle,  ///< Rectangle line cap.
    CPTLineCapTypeEllipse,    ///< Elliptical line cap.
    CPTLineCapTypeDiamond,    ///< Diamond line cap.
    CPTLineCapTypePentagon,   ///< Pentagon line cap.
    CPTLineCapTypeHexagon,    ///< Hexagon line cap.
    CPTLineCapTypeBar,        ///< Bar line cap.
    CPTLineCapTypeCross,      ///< X line cap.
    CPTLineCapTypeSnow,       ///< Snowflake line cap.
    CPTLineCapTypeCustom      ///< Custom line cap.
}
CPTLineCapType;

@interface CPTLineCap : NSObject<NSCoding, NSCopying> {
    @private
    CGSize size;
    CPTLineCapType lineCapType;
    CPTLineStyle *lineStyle;
    CPTFill *fill;
    CGPathRef cachedLineCapPath;
    CGPathRef customLineCapPath;
    BOOL usesEvenOddClipRule;
}

@property (nonatomic, readwrite, assign) CGSize size;
@property (nonatomic, readwrite, assign) CPTLineCapType lineCapType;
@property (nonatomic, readwrite, retain) CPTLineStyle *lineStyle;
@property (nonatomic, readwrite, retain) CPTFill *fill;
@property (nonatomic, readwrite, assign) CGPathRef customLineCapPath;
@property (nonatomic, readwrite, assign) BOOL usesEvenOddClipRule;

/// @name Factory Methods
/// @{
+(CPTLineCap *)lineCap;
+(CPTLineCap *)openArrowPlotLineCap;
+(CPTLineCap *)solidArrowPlotLineCap;
+(CPTLineCap *)sweptArrowPlotLineCap;
+(CPTLineCap *)rectanglePlotLineCap;
+(CPTLineCap *)ellipsePlotLineCap;
+(CPTLineCap *)diamondPlotLineCap;
+(CPTLineCap *)pentagonPlotLineCap;
+(CPTLineCap *)hexagonPlotLineCap;
+(CPTLineCap *)barPlotLineCap;
+(CPTLineCap *)crossPlotLineCap;
+(CPTLineCap *)snowPlotLineCap;
+(CPTLineCap *)customLineCapWithPath:(CGPathRef)aPath;
/// @}

/// @name Drawing
/// @{
-(void)renderAsVectorInContext:(CGContextRef)context atPoint:(CGPoint)center inDirection:(CGPoint)direction;
/// @}

@end

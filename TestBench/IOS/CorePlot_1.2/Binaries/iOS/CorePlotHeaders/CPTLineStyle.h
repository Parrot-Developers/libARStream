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
@class CPTColor;
@class CPTFill;

@interface CPTLineStyle : NSObject<NSCoding, NSCopying, NSMutableCopying> {
    @private
    CGLineCap lineCap;
//    CGLineDash lineDash; // We should make a struct to keep this information
    CGLineJoin lineJoin;
    CGFloat miterLimit;
    CGFloat lineWidth;
    NSArray *dashPattern;
    CGFloat patternPhase;
//    StrokePattern; // We should make a struct to keep this information
    CPTColor *lineColor;
    CPTFill *lineFill;
}

@property (nonatomic, readonly, assign) CGLineCap lineCap;
@property (nonatomic, readonly, assign) CGLineJoin lineJoin;
@property (nonatomic, readonly, assign) CGFloat miterLimit;
@property (nonatomic, readonly, assign) CGFloat lineWidth;
@property (nonatomic, readonly, retain) NSArray *dashPattern;
@property (nonatomic, readonly, assign) CGFloat patternPhase;
@property (nonatomic, readonly, retain) CPTColor *lineColor;
@property (nonatomic, readonly, retain) CPTFill *lineFill;

/// @name Factory Methods
/// @{
+(id)lineStyle;
/// @}

/// @name Drawing
/// @{
-(void)setLineStyleInContext:(CGContextRef)context;
-(void)strokePathInContext:(CGContextRef)context;
-(void)strokeRect:(CGRect)rect inContext:(CGContextRef)context;
/// @}

@end

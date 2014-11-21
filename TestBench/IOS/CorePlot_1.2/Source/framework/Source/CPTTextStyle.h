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

@class CPTColor;

/**
 *  @brief Enumeration of paragraph alignments.
 **/
typedef enum  _CPTTextAlignment {
    CPTTextAlignmentLeft,   ///< Left alignment
    CPTTextAlignmentCenter, ///< Center alignment
    CPTTextAlignmentRight   ///< Right alignment
}
CPTTextAlignment;

@interface CPTTextStyle : NSObject<NSCoding, NSCopying, NSMutableCopying> {
    @protected
    NSString *fontName;
    CGFloat fontSize;
    CPTColor *color;
    CPTTextAlignment textAlignment;
}

@property (readonly, copy, nonatomic) NSString *fontName;
@property (readonly, assign, nonatomic) CGFloat fontSize;
@property (readonly, copy, nonatomic) CPTColor *color;
@property (readonly, assign, nonatomic) CPTTextAlignment textAlignment;

/// @name Factory Methods
/// @{
+(id)textStyle;
/// @}

@end

/** @category NSString(CPTTextStyleExtensions)
 *  @brief NSString extensions for drawing styled text.
 **/
@interface NSString(CPTTextStyleExtensions)

/// @name Measurement
/// @{
-(CGSize)sizeWithTextStyle:(CPTTextStyle *)style;
/// @}

/// @name Drawing
/// @{
-(void)drawInRect:(CGRect)rect withTextStyle:(CPTTextStyle *)style inContext:(CGContextRef)context;
/// @}

@end

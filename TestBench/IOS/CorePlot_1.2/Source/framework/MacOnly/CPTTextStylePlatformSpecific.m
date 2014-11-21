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
#import "CPTTextStylePlatformSpecific.h"

#import "CPTMutableTextStyle.h"
#import "CPTPlatformSpecificCategories.h"
#import "CPTPlatformSpecificFunctions.h"

@implementation NSString(CPTTextStyleExtensions)

#pragma mark -
#pragma mark Layout

/** @brief Determines the size of text drawn with the given style.
 *  @param style The text style.
 *  @return The size of the text when drawn with the given style.
 **/
-(CGSize)sizeWithTextStyle:(CPTTextStyle *)style
{
    NSFont *theFont = [NSFont fontWithName:style.fontName size:style.fontSize];

    CGSize textSize;

    if ( theFont ) {
        NSDictionary *attributes = [[NSDictionary alloc] initWithObjectsAndKeys:
                                    theFont, NSFontAttributeName,
                                    nil];

        textSize = NSSizeToCGSize([self sizeWithAttributes:attributes]);

        [attributes release];
    }
    else {
        textSize = CGSizeZero;
    }

    return textSize;
}

#pragma mark -
#pragma mark Drawing

/** @brief Draws the text into the given graphics context using the given style.
 *  @param rect The bounding rectangle in which to draw the text.
 *  @param style The text style.
 *  @param context The graphics context to draw into.
 **/
-(void)drawInRect:(CGRect)rect withTextStyle:(CPTTextStyle *)style inContext:(CGContextRef)context
{
    if ( style.color == nil ) {
        return;
    }

    CGColorRef textColor = style.color.cgColor;

    CGContextSetStrokeColorWithColor(context, textColor);
    CGContextSetFillColorWithColor(context, textColor);

    CPTPushCGContext(context);
    NSFont *theFont = [NSFont fontWithName:style.fontName size:style.fontSize];
    if ( theFont ) {
        NSColor *foregroundColor                = style.color.nsColor;
        NSMutableParagraphStyle *paragraphStyle = [[NSMutableParagraphStyle alloc] init];

        switch ( style.textAlignment ) {
            case CPTTextAlignmentLeft:
                paragraphStyle.alignment = NSLeftTextAlignment;
                break;

            case CPTTextAlignmentCenter:
                paragraphStyle.alignment = NSCenterTextAlignment;
                break;

            case CPTTextAlignmentRight:
                paragraphStyle.alignment = NSRightTextAlignment;
                break;

            default:
                break;
        }

        NSDictionary *attributes = [[NSDictionary alloc] initWithObjectsAndKeys:
                                    theFont, NSFontAttributeName,
                                    foregroundColor, NSForegroundColorAttributeName,
                                    paragraphStyle, NSParagraphStyleAttributeName,
                                    nil];
        [self drawInRect:NSRectFromCGRect(rect) withAttributes:attributes];

        [paragraphStyle release];
        [attributes release];
    }
    CPTPopCGContext();
}

@end

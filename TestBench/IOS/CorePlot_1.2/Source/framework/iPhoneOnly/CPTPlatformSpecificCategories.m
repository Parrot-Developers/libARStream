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
#import "CPTPlatformSpecificCategories.h"

@implementation CPTColor(CPTPlatformSpecificColorExtensions)

/** @property uiColor
 *  @brief Gets the color value as a UIColor.
 **/
@dynamic uiColor;

-(UIColor *)uiColor
{
    return [UIColor colorWithCGColor:self.cgColor];
}

@end

#pragma mark -

@implementation CPTLayer(CPTPlatformSpecificLayerExtensions)

/** @brief Gets an image of the layer contents.
 *  @return A native image representation of the layer content.
 **/
-(CPTNativeImage *)imageOfLayer
{
    CGSize boundsSize = self.bounds.size;

    if ( UIGraphicsBeginImageContextWithOptions ) {
        UIGraphicsBeginImageContextWithOptions(boundsSize, self.opaque, (CGFloat)0.0);
    }
    else {
        UIGraphicsBeginImageContext(boundsSize);
    }

    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSaveGState(context);
    CGContextSetAllowsAntialiasing(context, true);

    CGContextTranslateCTM(context, (CGFloat)0.0, boundsSize.height);
    CGContextScaleCTM(context, (CGFloat)1.0, (CGFloat) - 1.0);

    [self layoutAndRenderInContext:context];
    CPTNativeImage *layerImage = UIGraphicsGetImageFromCurrentImageContext();
    CGContextSetAllowsAntialiasing(context, false);

    CGContextRestoreGState(context);
    UIGraphicsEndImageContext();

    return layerImage;
}

@end

#pragma mark -

@implementation NSNumber(CPTPlatformSpecificNumberExtensions)

/** @brief Returns a Boolean value that indicates whether the receiver is less than another given number.
 *  @param other The other number to compare to the receiver.
 *  @return @YES if the receiver is less than other, otherwise @NO.
 **/
-(BOOL)isLessThan:(NSNumber *)other
{
    return [self compare:other] == NSOrderedAscending;
}

/** @brief Returns a Boolean value that indicates whether the receiver is less than or equal to another given number.
 *  @param other The other number to compare to the receiver.
 *  @return @YES if the receiver is less than or equal to other, otherwise @NO.
 **/
-(BOOL)isLessThanOrEqualTo:(NSNumber *)other
{
    return [self compare:other] == NSOrderedSame || [self compare:other] == NSOrderedAscending;
}

/** @brief Returns a Boolean value that indicates whether the receiver is greater than another given number.
 *  @param other The other number to compare to the receiver.
 *  @return @YES if the receiver is greater than other, otherwise @NO.
 **/
-(BOOL)isGreaterThan:(NSNumber *)other
{
    return [self compare:other] == NSOrderedDescending;
}

/** @brief Returns a Boolean value that indicates whether the receiver is greater than or equal to another given number.
 *  @param other The other number to compare to the receiver.
 *  @return @YES if the receiver is greater than or equal to other, otherwise @NO.
 **/
-(BOOL)isGreaterThanOrEqualTo:(NSNumber *)other
{
    return [self compare:other] == NSOrderedSame || [self compare:other] == NSOrderedDescending;
}

@end

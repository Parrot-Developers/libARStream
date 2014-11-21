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
#import "CPTColor.h"
#import "CPTFill.h"
#import "CPTFillTests.h"
#import "CPTGradient.h"
#import "CPTImage.h"
#import "_CPTFillColor.h"
#import "_CPTFillGradient.h"
#import "_CPTFillImage.h"

@interface _CPTFillColor()

@property (nonatomic, readwrite, copy) CPTColor *fillColor;

@end

#pragma mark -

@interface _CPTFillGradient()

@property (nonatomic, readwrite, copy) CPTGradient *fillGradient;

@end

#pragma mark -

@interface _CPTFillImage()

@property (nonatomic, readwrite, copy) CPTImage *fillImage;

@end

#pragma mark -

@implementation CPTFillTests

#pragma mark -
#pragma mark NSCoding Methods

-(void)testKeyedArchivingRoundTripColor
{
    _CPTFillColor *fill = (_CPTFillColor *)[CPTFill fillWithColor:[CPTColor redColor]];

    _CPTFillColor *newFill = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:fill]];

    STAssertEqualObjects(fill.fillColor, newFill.fillColor, @"Fill with color not equal");
}

-(void)testKeyedArchivingRoundTripGradient
{
    _CPTFillGradient *fill = (_CPTFillGradient *)[CPTFill fillWithGradient:[CPTGradient rainbowGradient]];

    _CPTFillGradient *newFill = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:fill]];

    STAssertEqualObjects(fill.fillGradient, newFill.fillGradient, @"Fill with gradient not equal");
}

-(void)testKeyedArchivingRoundTripImage
{
    const size_t width  = 100;
    const size_t height = 100;

    size_t bytesPerRow         = (4 * width + 15) & ~15ul;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGContextRef context       = CGBitmapContextCreate(NULL, width, height, 8, bytesPerRow, colorSpace, kCGImageAlphaNoneSkipLast);
    CGImageRef cgImage         = CGBitmapContextCreateImage(context);

    CPTImage *image = [CPTImage imageWithCGImage:cgImage];

    image.tiled                 = YES;
    image.tileAnchoredToContext = YES;

    CGColorSpaceRelease(colorSpace);
    CGImageRelease(cgImage);

    _CPTFillImage *fill = (_CPTFillImage *)[CPTFill fillWithImage:image];

    _CPTFillImage *newFill = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:fill]];

    STAssertEqualObjects(fill.fillImage, newFill.fillImage, @"Fill with image not equal");
}

@end

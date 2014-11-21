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

@implementation CPTLayer(CPTPlatformSpecificLayerExtensions)

/** @brief Gets an image of the layer contents.
 *  @return A native image representation of the layer content.
 **/
-(CPTNativeImage *)imageOfLayer
{
    CGSize boundsSize = self.bounds.size;

    NSBitmapImageRep *layerImage = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                                           pixelsWide:(NSInteger)boundsSize.width
                                                                           pixelsHigh:(NSInteger)boundsSize.height
                                                                        bitsPerSample:8
                                                                      samplesPerPixel:4
                                                                             hasAlpha:YES
                                                                             isPlanar:NO
                                                                       colorSpaceName:NSCalibratedRGBColorSpace
                                                                          bytesPerRow:(NSInteger)boundsSize.width * 4 bitsPerPixel:32];

    NSGraphicsContext *bitmapContext = [NSGraphicsContext graphicsContextWithBitmapImageRep:layerImage];
    CGContextRef context             = (CGContextRef)[bitmapContext graphicsPort];

    CGContextClearRect( context, CPTRectMake(0.0, 0.0, boundsSize.width, boundsSize.height) );
    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    [self layoutAndRenderInContext:context];
    CGContextFlush(context);

    NSImage *image = [[NSImage alloc] initWithSize:NSSizeFromCGSize(boundsSize)];
    [image addRepresentation:layerImage];
    [layerImage release];

    return [image autorelease];
}

@end

#pragma mark -

@implementation CPTColor(CPTPlatformSpecificColorExtensions)

/** @property nsColor
 *  @brief Gets the color value as an NSColor.
 **/
@dynamic nsColor;

-(NSColor *)nsColor
{
    return [NSColor colorWithCIColor:[CIColor colorWithCGColor:self.cgColor]];
}

@end

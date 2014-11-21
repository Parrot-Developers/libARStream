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
#import "CPTGraphHostingView.h"

#import "CPTGraph.h"

/// @cond
// for MacOS 10.6 SDK compatibility
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#else
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
@interface NSWindow(CPTExtensions)

@property (readonly) CGFloat backingScaleFactor;

-(void)viewDidChangeBackingProperties;

@end
#endif
#endif

/// @endcond

#pragma mark -

/**
 *  @brief A container view for displaying a CPTGraph.
 **/
@implementation CPTGraphHostingView

/** @property CPTGraph *hostedGraph
 *  @brief The CPTGraph hosted inside this view.
 **/
@synthesize hostedGraph;

/** @property NSRect printRect
 *  @brief The bounding rectangle used when printing this view.
 **/
@synthesize printRect;

/// @cond

-(id)initWithFrame:(NSRect)frame
{
    if ( (self = [super initWithFrame:frame]) ) {
        hostedGraph = nil;
        printRect   = NSZeroRect;
        CPTLayer *mainLayer = [(CPTLayer *)[CPTLayer alloc] initWithFrame:NSRectToCGRect(frame)];
        self.layer = mainLayer;
        [mainLayer release];
    }
    return self;
}

-(void)dealloc
{
    [hostedGraph removeFromSuperlayer];
    [hostedGraph release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:self.hostedGraph forKey:@"CPTLayerHostingView.hostedGraph"];
    [coder encodeRect:self.printRect forKey:@"CPTLayerHostingView.printRect"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        CPTLayer *mainLayer = [(CPTLayer *)[CPTLayer alloc] initWithFrame:NSRectToCGRect(self.frame)];
        self.layer = mainLayer;
        [mainLayer release];

        hostedGraph      = nil;
        self.hostedGraph = [coder decodeObjectForKey:@"CPTLayerHostingView.hostedGraph"]; // setup layers
        self.printRect   = [coder decodeRectForKey:@"CPTLayerHostingView.printRect"];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Drawing

/// @cond

-(void)drawRect:(NSRect)dirtyRect
{
    if ( self.hostedGraph ) {
        if ( ![NSGraphicsContext currentContextDrawingToScreen] ) {
            [self viewDidChangeBackingProperties];

            NSGraphicsContext *graphicsContext = [NSGraphicsContext currentContext];

            [graphicsContext saveGraphicsState];

            CGRect destinationRect = NSRectToCGRect(self.printRect);
            CGRect sourceRect      = NSRectToCGRect(self.frame);

            // scale the view isotropically so that it fits on the printed page
            CGFloat widthScale  = ( sourceRect.size.width != CPTFloat(0.0) ) ? destinationRect.size.width / sourceRect.size.width : CPTFloat(1.0);
            CGFloat heightScale = ( sourceRect.size.height != CPTFloat(0.0) ) ? destinationRect.size.height / sourceRect.size.height : CPTFloat(1.0);
            CGFloat scale       = MIN(widthScale, heightScale);

            // position the view so that its centered on the printed page
            CGPoint offset = destinationRect.origin;
            offset.x += ( ( destinationRect.size.width - (sourceRect.size.width * scale) ) / CPTFloat(2.0) );
            offset.y += ( ( destinationRect.size.height - (sourceRect.size.height * scale) ) / CPTFloat(2.0) );

            NSAffineTransform *transform = [NSAffineTransform transform];
            [transform translateXBy:offset.x yBy:offset.y];
            [transform scaleBy:scale];
            [transform concat];

            // render CPTLayers recursively into the graphics context used for printing
            // (thanks to Brad for the tip: http://stackoverflow.com/a/2791305/132867 )
            CGContextRef context = [graphicsContext graphicsPort];
            [self.hostedGraph recursivelyRenderInContext:context];

            [graphicsContext restoreGraphicsState];
        }
    }
}

-(BOOL)knowsPageRange:(NSRangePointer)rangePointer
{
    rangePointer->location = 1;
    rangePointer->length   = 1;

    return YES;
}

-(NSRect)rectForPage:(NSInteger)pageNumber
{
    return self.printRect;
}

/// @endcond

#pragma mark -
#pragma mark Mouse handling

/// @cond

-(BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    return YES;
}

-(void)mouseDown:(NSEvent *)theEvent
{
    CPTGraph *theGraph = self.hostedGraph;

    if ( theGraph ) {
        CGPoint pointOfMouseDown   = NSPointToCGPoint([self convertPoint:[theEvent locationInWindow] fromView:nil]);
        CGPoint pointInHostedGraph = [self.layer convertPoint:pointOfMouseDown toLayer:theGraph];
        [theGraph pointingDeviceDownEvent:theEvent atPoint:pointInHostedGraph];
    }
}

-(void)mouseDragged:(NSEvent *)theEvent
{
    CPTGraph *theGraph = self.hostedGraph;

    if ( theGraph ) {
        CGPoint pointOfMouseDrag   = NSPointToCGPoint([self convertPoint:[theEvent locationInWindow] fromView:nil]);
        CGPoint pointInHostedGraph = [self.layer convertPoint:pointOfMouseDrag toLayer:theGraph];
        [theGraph pointingDeviceDraggedEvent:theEvent atPoint:pointInHostedGraph];
    }
}

-(void)mouseUp:(NSEvent *)theEvent
{
    CPTGraph *theGraph = self.hostedGraph;

    if ( theGraph ) {
        CGPoint pointOfMouseUp     = NSPointToCGPoint([self convertPoint:[theEvent locationInWindow] fromView:nil]);
        CGPoint pointInHostedGraph = [self.layer convertPoint:pointOfMouseUp toLayer:theGraph];
        [theGraph pointingDeviceUpEvent:theEvent atPoint:pointInHostedGraph];
    }
}

/// @endcond

#pragma mark -
#pragma mark HiDPI display support

/// @cond

-(void)viewDidChangeBackingProperties
{
    CPTLayer *myLayer  = (CPTLayer *)self.layer;
    NSWindow *myWindow = self.window;

    // backingScaleFactor property is available in MacOS 10.7 and later
    if ( [myWindow respondsToSelector:@selector(backingScaleFactor)] ) {
        myLayer.contentsScale = myWindow.backingScaleFactor;
    }
    else {
        myLayer.contentsScale = CPTFloat(1.0);
    }
}

/// @endcond

#pragma mark -
#pragma mark Accessors

/// @cond

-(void)setHostedGraph:(CPTGraph *)newGraph
{
    NSParameterAssert( (newGraph == nil) || [newGraph isKindOfClass:[CPTGraph class]] );

    if ( newGraph != hostedGraph ) {
        self.wantsLayer = YES;
        [hostedGraph removeFromSuperlayer];
        hostedGraph.hostingView = nil;
        [hostedGraph release];
        hostedGraph = [newGraph retain];

        if ( hostedGraph ) {
            hostedGraph.hostingView = self;

            [self viewDidChangeBackingProperties];
            [self.layer addSublayer:hostedGraph];
        }
    }
}

/// @endcond

@end

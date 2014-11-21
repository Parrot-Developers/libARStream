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
#import "CPTPlatformSpecificFunctions.h"

/** @brief Node in a linked list of graphics contexts.
**/
typedef struct _CPTContextNode {
    NSGraphicsContext *context;       ///< The graphics context.
    struct _CPTContextNode *nextNode; ///< Pointer to the next node in the list.
}
CPTContextNode;

#pragma mark -
#pragma mark Graphics Context

// linked list to store saved contexts
static CPTContextNode *pushedContexts = NULL;

/** @brief Pushes the current AppKit graphics context onto a stack and replaces it with the given Core Graphics context.
 *  @param newContext The graphics context.
 **/
void CPTPushCGContext(CGContextRef newContext)
{
    if ( newContext ) {
        CPTContextNode *newNode = malloc( sizeof(CPTContextNode) );
        newNode->context = [NSGraphicsContext currentContext];
        [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithGraphicsPort:newContext flipped:NO]];
        newNode->nextNode = pushedContexts;
        pushedContexts    = newNode;
    }
}

/**
 *  @brief Pops the top context off the stack and restores it to the AppKit graphics context.
 **/
void CPTPopCGContext(void)
{
    if ( pushedContexts ) {
        [NSGraphicsContext setCurrentContext:pushedContexts->context];
        CPTContextNode *next = pushedContexts->nextNode;
        free(pushedContexts);
        pushedContexts = next;
    }
}

#pragma mark -
#pragma mark Context

/**
 *  @brief Get the default graphics context
 **/
CGContextRef CPTGetCurrentContext(void)
{
    return [[NSGraphicsContext currentContext] graphicsPort];
}

#pragma mark -
#pragma mark Colors

/** @brief Creates a @ref CGColorRef from an NSColor.
 *
 *  The caller must release the returned @ref CGColorRef. Pattern colors are not supported.
 *
 *  @param nsColor The NSColor.
 *  @return The @ref CGColorRef.
 **/
CGColorRef CPTCreateCGColorFromNSColor(NSColor *nsColor)
{
    NSColor *rgbColor = [nsColor colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
    CGFloat r, g, b, a;

    [rgbColor getRed:&r green:&g blue:&b alpha:&a];
    return CGColorCreateGenericRGB(r, g, b, a);
}

/** @brief Creates a CPTRGBAColor from an NSColor.
 *
 *  Pattern colors are not supported.
 *
 *  @param nsColor The NSColor.
 *  @return The CPTRGBAColor.
 **/
CPTRGBAColor CPTRGBAColorFromNSColor(NSColor *nsColor)
{
    CGFloat red, green, blue, alpha;

    [[nsColor colorUsingColorSpaceName:NSCalibratedRGBColorSpace] getRed:&red green:&green blue:&blue alpha:&alpha];

    CPTRGBAColor rgbColor;
    rgbColor.red   = red;
    rgbColor.green = green;
    rgbColor.blue  = blue;
    rgbColor.alpha = alpha;

    return rgbColor;
}

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
#import "CPTDefinitions.h"
#import "CPTResponder.h"
#import <QuartzCore/QuartzCore.h>

@class CPTGraph;
@class CPTShadow;

/// @name Layout
/// @{

/** @brief Notification sent by all layers when the layer @link CALayer::bounds bounds @endlink change.
 *  @ingroup notification
 **/
extern NSString *const CPTLayerBoundsDidChangeNotification;

/// @}

@interface CPTLayer : CALayer<CPTResponder> {
    @private
    CGFloat paddingLeft;
    CGFloat paddingTop;
    CGFloat paddingRight;
    CGFloat paddingBottom;
    BOOL masksToBorder;
    CPTShadow *shadow;
    BOOL renderingRecursively;
    BOOL useFastRendering;
    __cpt_weak CPTGraph *graph;
    CGPathRef outerBorderPath;
    CGPathRef innerBorderPath;
    id<NSCopying, NSCoding, NSObject> identifier;
}

/// @name Graph
/// @{
@property (nonatomic, readwrite, cpt_weak_property) __cpt_weak CPTGraph *graph;
/// @}

/// @name Padding
/// @{
@property (nonatomic, readwrite) CGFloat paddingLeft;
@property (nonatomic, readwrite) CGFloat paddingTop;
@property (nonatomic, readwrite) CGFloat paddingRight;
@property (nonatomic, readwrite) CGFloat paddingBottom;
/// @}

/// @name Drawing
/// @{
@property (readwrite) CGFloat contentsScale;
@property (nonatomic, readonly, assign) BOOL useFastRendering;
@property (nonatomic, readwrite, copy) CPTShadow *shadow;
@property (nonatomic, readonly) CGSize shadowMargin;
/// @}

/// @name Masking
/// @{
@property (nonatomic, readwrite, assign) BOOL masksToBorder;
@property (nonatomic, readwrite, assign) CGPathRef outerBorderPath;
@property (nonatomic, readwrite, assign) CGPathRef innerBorderPath;
@property (nonatomic, readonly, assign) CGPathRef maskingPath;
@property (nonatomic, readonly, assign) CGPathRef sublayerMaskingPath;
/// @}

/// @name Identification
/// @{
@property (nonatomic, readwrite, copy) id<NSCopying, NSCoding, NSObject> identifier;
/// @}

/// @name Layout
/// @{
@property (readonly) NSSet *sublayersExcludedFromAutomaticLayout;
/// @}

/// @name Initialization
/// @{
-(id)initWithFrame:(CGRect)newFrame;
/// @}

/// @name Drawing
/// @{
-(void)renderAsVectorInContext:(CGContextRef)context;
-(void)recursivelyRenderInContext:(CGContextRef)context;
-(void)layoutAndRenderInContext:(CGContextRef)context;
-(NSData *)dataForPDFRepresentationOfLayer;
/// @}

/// @name Masking
/// @{
-(void)applySublayerMaskToContext:(CGContextRef)context forSublayer:(CPTLayer *)sublayer withOffset:(CGPoint)offset;
-(void)applyMaskToContext:(CGContextRef)context;
/// @}

/// @name Layout
/// @{
-(void)pixelAlign;
-(void)sublayerMarginLeft:(CGFloat *)left top:(CGFloat *)top right:(CGFloat *)right bottom:(CGFloat *)bottom;
/// @}

/// @name Information
/// @{
-(void)logLayers;
/// @}

@end

/// @cond
// for MacOS 10.6 SDK compatibility
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#else
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
@interface CALayer(CPTExtensions)

@property (readwrite) CGFloat contentsScale;

@end
#endif
#endif

/// @endcond

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

@class CPTAnnotationHostLayer;
@class CPTLayer;

@interface CPTAnnotation : NSObject<NSCoding> {
    @private
    __cpt_weak CPTAnnotationHostLayer *annotationHostLayer;
    CPTLayer *contentLayer;
    CGPoint contentAnchorPoint;
    CGPoint displacement;
    CGFloat rotation;
}

@property (nonatomic, readwrite, retain) CPTLayer *contentLayer;
@property (nonatomic, readwrite, cpt_weak_property) __cpt_weak CPTAnnotationHostLayer *annotationHostLayer;
@property (nonatomic, readwrite, assign) CGPoint contentAnchorPoint;
@property (nonatomic, readwrite, assign) CGPoint displacement;
@property (nonatomic, readwrite, assign) CGFloat rotation;

@end

#pragma mark -

/** @category CPTAnnotation(AbstractMethods)
 *  @brief CPTAnnotation abstract methods—must be overridden by subclasses.
 **/
@interface CPTAnnotation(AbstractMethods)

/// @name Layout
/// @{
-(void)positionContentLayer;
/// @}

@end

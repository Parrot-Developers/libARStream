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
#import "CPTAnnotationHostLayer.h"

#import "CPTAnnotation.h"
#import "CPTExceptions.h"

/// @cond
@interface CPTAnnotationHostLayer()

@property (nonatomic, readwrite, retain) NSMutableArray *mutableAnnotations;

@end

/// @endcond

#pragma mark -

/** @brief A container layer for annotations.
 *
 *  Annotations (CPTAnnotation) can be added to and removed from an annotation layer.
 *  The host layer automatically handles the annotation layout.
 **/
@implementation CPTAnnotationHostLayer

/** @property NSArray *annotations
 *  @brief An array of annotations attached to this layer.
 **/
@dynamic annotations;

@synthesize mutableAnnotations;

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTAnnotationHostLayer object with the provided frame rectangle.
 *
 *  This is the designated initializer. The initialized layer will have an empty
 *  @ref annotations array.
 *
 *  @param newFrame The frame rectangle.
 *  @return The initialized CPTAnnotationHostLayer object.
 **/
-(id)initWithFrame:(CGRect)newFrame
{
    if ( (self = [super initWithFrame:newFrame]) ) {
        mutableAnnotations = [[NSMutableArray alloc] init];
    }
    return self;
}

/// @}

/// @cond

-(id)initWithLayer:(id)layer
{
    if ( (self = [super initWithLayer:layer]) ) {
        CPTAnnotationHostLayer *theLayer = (CPTAnnotationHostLayer *)layer;

        mutableAnnotations = [theLayer->mutableAnnotations retain];
    }
    return self;
}

-(void)dealloc
{
    [mutableAnnotations release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:self.mutableAnnotations forKey:@"CPTAnnotationHostLayer.mutableAnnotations"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super initWithCoder:coder]) ) {
        mutableAnnotations = [[coder decodeObjectForKey:@"CPTAnnotationHostLayer.mutableAnnotations"] mutableCopy];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark Annotations

-(NSArray *)annotations
{
    return [[self.mutableAnnotations copy] autorelease];
}

/**
 *  @brief Adds an annotation to the receiver.
 **/
-(void)addAnnotation:(CPTAnnotation *)annotation
{
    if ( annotation ) {
        NSMutableArray *annotationArray = self.mutableAnnotations;
        if ( ![annotationArray containsObject:annotation] ) {
            [annotationArray addObject:annotation];
        }
        annotation.annotationHostLayer = self;
        [annotation positionContentLayer];
    }
}

/**
 *  @brief Removes an annotation from the receiver.
 **/
-(void)removeAnnotation:(CPTAnnotation *)annotation
{
    if ( [self.mutableAnnotations containsObject:annotation] ) {
        annotation.annotationHostLayer = nil;
        [self.mutableAnnotations removeObject:annotation];
    }
    else {
        [NSException raise:CPTException format:@"Tried to remove CPTAnnotation from %@. Host layer was %@.", self, annotation.annotationHostLayer];
    }
}

/**
 *  @brief Removes all annotations from the receiver.
 **/
-(void)removeAllAnnotations
{
    NSMutableArray *allAnnotations = self.mutableAnnotations;

    for ( CPTAnnotation *annotation in allAnnotations ) {
        annotation.annotationHostLayer = nil;
    }
    [allAnnotations removeAllObjects];
}

#pragma mark -
#pragma mark Layout

/// @cond

-(NSSet *)sublayersExcludedFromAutomaticLayout
{
    NSMutableArray *annotations = self.mutableAnnotations;

    if ( annotations.count > 0 ) {
        NSMutableSet *excludedSublayers = [[[super sublayersExcludedFromAutomaticLayout] mutableCopy] autorelease];

        if ( !excludedSublayers ) {
            excludedSublayers = [NSMutableSet set];
        }

        for ( CPTAnnotation *annotation in annotations ) {
            CALayer *content = annotation.contentLayer;
            if ( content ) {
                [excludedSublayers addObject:content];
            }
        }

        return excludedSublayers;
    }
    else {
        return [super sublayersExcludedFromAutomaticLayout];
    }
}

-(void)layoutSublayers
{
    [super layoutSublayers];
    [self.mutableAnnotations makeObjectsPerformSelector:@selector(positionContentLayer)];
}

/// @endcond

@end

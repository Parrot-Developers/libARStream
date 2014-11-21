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
#import "CPTLimitBand.h"

#import "CPTFill.h"
#import "CPTPlotRange.h"

/**
 *  @brief Defines a range and fill used to highlight a band of data.
 **/
@implementation CPTLimitBand

/** @property CPTPlotRange *range
 *  @brief The data range for the band.
 **/
@synthesize range;

/** @property CPTFill *fill
 *  @brief The fill used to draw the band.
 **/
@synthesize fill;

#pragma mark -
#pragma mark Init/Dealloc

/** @brief Creates and returns a new CPTLimitBand instance initialized with the provided range and fill.
 *  @param newRange The range of the band.
 *  @param newFill The fill used to draw the interior of the band.
 *  @return A new CPTLimitBand instance initialized with the provided range and fill.
 **/
+(CPTLimitBand *)limitBandWithRange:(CPTPlotRange *)newRange fill:(CPTFill *)newFill
{
    return [[[CPTLimitBand alloc] initWithRange:newRange fill:newFill] autorelease];
}

/** @brief Initializes a newly allocated CPTLimitBand object with the provided range and fill.
 *  @param newRange The range of the band.
 *  @param newFill The fill used to draw the interior of the band.
 *  @return The initialized CPTLimitBand object.
 **/
-(id)initWithRange:(CPTPlotRange *)newRange fill:(CPTFill *)newFill
{
    if ( (self = [super init]) ) {
        range = [newRange retain];
        fill  = [newFill retain];
    }
    return self;
}

/// @cond

-(void)dealloc
{
    [range release];
    [fill release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCopying Methods

/// @cond

-(id)copyWithZone:(NSZone *)zone
{
    CPTLimitBand *newBand = [[CPTLimitBand allocWithZone:zone] init];

    if ( newBand ) {
        newBand->range = [self->range copyWithZone:zone];
        newBand->fill  = [self->fill copyWithZone:zone];
    }
    return newBand;
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)encoder
{
    if ( [encoder allowsKeyedCoding] ) {
        [encoder encodeObject:range forKey:@"CPTLimitBand.range"];
        [encoder encodeObject:fill forKey:@"CPTLimitBand.fill"];
    }
    else {
        [encoder encodeObject:range];
        [encoder encodeObject:fill];
    }
}

-(id)initWithCoder:(NSCoder *)decoder
{
    CPTPlotRange *newRange;
    CPTFill *newFill;

    if ( [decoder allowsKeyedCoding] ) {
        newRange = [decoder decodeObjectForKey:@"CPTLimitBand.range"];
        newFill  = [decoder decodeObjectForKey:@"CPTLimitBand.fill"];
    }
    else {
        newRange = [decoder decodeObject];
        newFill  = [decoder decodeObject];
    }

    return [self initWithRange:newRange fill:newFill];
}

/// @endcond

#pragma mark -
#pragma mark Description

/// @cond

-(NSString *)description
{
    return [NSString stringWithFormat:@"<%@ with range: %@ and fill: %@>", [super description], self.range, self.fill];
}

/// @endcond

@end

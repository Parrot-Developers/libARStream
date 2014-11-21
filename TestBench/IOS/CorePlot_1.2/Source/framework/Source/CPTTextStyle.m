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
#import "CPTTextStyle.h"

#import "CPTColor.h"
#import "CPTDefinitions.h"
#import "CPTMutableTextStyle.h"
#import "NSCoderExtensions.h"

/// @cond
@interface CPTTextStyle()

@property (readwrite, copy, nonatomic) NSString *fontName;
@property (readwrite, assign, nonatomic) CGFloat fontSize;
@property (readwrite, copy, nonatomic) CPTColor *color;
@property (readwrite, assign, nonatomic) CPTTextAlignment textAlignment;

@end

/// @endcond

/** @brief Immutable wrapper for various text style properties.
 *
 *  If you need to customize properties, you should create a CPTMutableTextStyle.
 **/

@implementation CPTTextStyle

/** @property CGFloat fontSize
 *  @brief The font size. Default is @num{12.0}.
 **/
@synthesize fontSize;

/** @property NSString *fontName
 *  @brief The font name. Default is Helvetica.
 **/
@synthesize fontName;

/** @property CPTColor *color
 *  @brief The current text color. Default is solid black.
 **/
@synthesize color;

/** @property CPTTextAlignment textAlignment
 *  @brief The paragraph alignment for multi-line text. Default is #CPTTextAlignmentLeft.
 **/
@synthesize textAlignment;

#pragma mark -
#pragma mark Factory Methods

/** @brief Creates and returns a new CPTTextStyle instance.
 *  @return A new CPTTextStyle instance.
 **/
+(id)textStyle
{
    return [[[self alloc] init] autorelease];
}

#pragma mark -
#pragma mark Init/Dealloc

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTAnnotation object.
 *
 *  The initialized object will have the following properties:
 *  - @ref fontName = Helvetica
 *  - @ref fontSize = @num{12.0}
 *  - @ref color = opaque black
 *  - @ref textAlignment = #CPTTextAlignmentLeft
 *
 *  @return The initialized object.
 **/
-(id)init
{
    if ( (self = [super init]) ) {
        fontName      = @"Helvetica";
        fontSize      = CPTFloat(12.0);
        color         = [[CPTColor blackColor] retain];
        textAlignment = CPTTextAlignmentLeft;
    }
    return self;
}

/// @}

/// @cond

-(void)dealloc
{
    [fontName release];
    [color release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark NSCoding Methods

/// @cond

-(void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeObject:self.fontName forKey:@"CPTTextStyle.fontName"];
    [coder encodeCGFloat:self.fontSize forKey:@"CPTTextStyle.fontSize"];
    [coder encodeObject:self.color forKey:@"CPTTextStyle.color"];
    [coder encodeInt:self.textAlignment forKey:@"CPTTextStyle.textAlignment"];
}

-(id)initWithCoder:(NSCoder *)coder
{
    if ( (self = [super init]) ) {
        self->fontName      = [[coder decodeObjectForKey:@"CPTTextStyle.fontName"] copy];
        self->fontSize      = [coder decodeCGFloatForKey:@"CPTTextStyle.fontSize"];
        self->color         = [[coder decodeObjectForKey:@"CPTTextStyle.color"] copy];
        self->textAlignment = (CPTTextAlignment)[coder decodeIntForKey : @"CPTTextStyle.textAlignment"];
    }
    return self;
}

/// @endcond

#pragma mark -
#pragma mark NSCopying Methods

/// @cond

-(id)copyWithZone:(NSZone *)zone
{
    CPTTextStyle *newCopy = [[CPTTextStyle allocWithZone:zone] init];

    newCopy->fontName      = [self->fontName copy];
    newCopy->color         = [self->color copy];
    newCopy->fontSize      = self->fontSize;
    newCopy->textAlignment = self->textAlignment;
    return newCopy;
}

/// @endcond

#pragma mark -
#pragma mark NSMutableCopying Methods

/// @cond

-(id)mutableCopyWithZone:(NSZone *)zone
{
    CPTTextStyle *newCopy = [[CPTMutableTextStyle allocWithZone:zone] init];

    newCopy->fontName      = [self->fontName copy];
    newCopy->color         = [self->color copy];
    newCopy->fontSize      = self->fontSize;
    newCopy->textAlignment = self->textAlignment;
    return newCopy;
}

/// @endcond

@end

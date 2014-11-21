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
#import "CPTAnimationOperation.h"

#import "CPTAnimationPeriod.h"

/** @brief Describes all aspects of an animation operation, including the value range, duration, animation curve, property to update, and the delegate.
**/
@implementation CPTAnimationOperation

/** @property CPTAnimationPeriod *period
 *  @brief The start value, end value, and duration of this animation operation.
 **/
@synthesize period;

/** @property CPTAnimationCurve animationCurve
 *  @brief The animation curve used to animate this operation.
 **/
@synthesize animationCurve;

/** @property id boundObject
 *  @brief The object to update for each animation frame.
 **/
@synthesize boundObject;

/** @property SEL boundGetter
 *  @brief The @ref boundObject getter method for the property to update for each animation frame.
 **/
@synthesize boundGetter;

/** @property SEL boundSetter
 *  @brief The @ref boundObject setter method for the property to update for each animation frame.
 **/
@synthesize boundSetter;

/** @property __cpt_weak NSObject<CPTAnimationDelegate> *delegate
 *  @brief The animation delegate
 **/
@synthesize delegate;

/// @name Initialization
/// @{

/** @brief Initializes a newly allocated CPTAnimationOperation object.
 *
 *  This is the designated initializer. The initialized object will have the following properties:
 *  - @ref period = @nil
 *  - @ref animationCurve = #CPTAnimationCurveDefault
 *  - @ref boundObject = @nil
 *  - @ref boundGetter = @NULL
 *  - @ref boundSetter = @NULL
 *  - @ref delegate = @nil
 *
 *  @return The initialized object.
 **/
-(id)init
{
    if ( (self = [super init]) ) {
        period         = nil;
        animationCurve = CPTAnimationCurveDefault;
        boundObject    = nil;
        boundGetter    = NULL;
        boundSetter    = NULL;
        delegate       = nil;
    }

    return self;
}

/// @}

/// @cond

-(void)dealloc
{
    [period release];
    [boundObject release];
    [super dealloc];
}

/// @endcond

#pragma mark -
#pragma mark Description

/// @cond

-(NSString *)description
{
    return [NSString stringWithFormat:@"<%@ animate %@ %@ with period %@>", [super description], self.boundObject, NSStringFromSelector(self.boundGetter), period];
}

/// @endcond

@end

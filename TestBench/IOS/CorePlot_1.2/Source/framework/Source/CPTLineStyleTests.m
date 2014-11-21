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
#import "CPTLineStyle.h"
#import "CPTLineStyleTests.h"

@implementation CPTLineStyleTests

#pragma mark -
#pragma mark NSCoding Methods

-(void)testKeyedArchivingRoundTrip
{
    CPTLineStyle *lineStyle = [CPTLineStyle lineStyle];

    CPTLineStyle *newLineStyle = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:lineStyle]];

    STAssertEquals(newLineStyle.lineCap, lineStyle.lineCap, @"Line cap not equal");
    STAssertEquals(newLineStyle.lineJoin, lineStyle.lineJoin, @"Line join not equal");
    STAssertEquals(newLineStyle.miterLimit, lineStyle.miterLimit, @"Miter limit not equal");
    STAssertEquals(newLineStyle.lineWidth, lineStyle.lineWidth, @"Line width not equal");
    STAssertEqualObjects(newLineStyle.dashPattern, lineStyle.dashPattern, @"Dash pattern not equal");
    STAssertEquals(newLineStyle.patternPhase, lineStyle.patternPhase, @"Pattern phase not equal");
    STAssertEqualObjects(newLineStyle.lineColor, lineStyle.lineColor, @"Line colors not equal");
}

@end

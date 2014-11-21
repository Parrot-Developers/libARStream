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
#import "CPTTimeFormatter.h"
#import "CPTTimeFormatterTests.h"

@implementation CPTTimeFormatterTests

#pragma mark -
#pragma mark NSCoding Methods

-(void)testKeyedArchivingRoundTrip
{
    NSDate *refDate                = [NSDate dateWithNaturalLanguageString:@"12:00 Oct 29, 2009"];
    NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];

    dateFormatter.dateStyle = NSDateFormatterShortStyle;

    CPTTimeFormatter *timeFormatter = [[[CPTTimeFormatter alloc] initWithDateFormatter:dateFormatter] autorelease];
    timeFormatter.referenceDate = refDate;

    CPTTimeFormatter *newTimeFormatter = [NSKeyedUnarchiver unarchiveObjectWithData:[NSKeyedArchiver archivedDataWithRootObject:timeFormatter]];

    STAssertEqualObjects(timeFormatter.dateFormatter.dateFormat, newTimeFormatter.dateFormatter.dateFormat, @"Date formatter not equal");
    STAssertEqualObjects(timeFormatter.referenceDate, newTimeFormatter.referenceDate, @"Reference date not equal");
}

@end

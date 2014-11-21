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
#import "CPTDerivedXYGraph.h"
#import "CPTExceptions.h"
#import "CPTTheme.h"
#import "CPTThemeTests.h"
#import "_CPTDarkGradientTheme.h"
#import "_CPTPlainBlackTheme.h"
#import "_CPTPlainWhiteTheme.h"
#import "_CPTSlateTheme.h"
#import "_CPTStocksTheme.h"

@implementation CPTThemeTests

-(void)testSetGraphClassUsingCPTXYGraphShouldWork
{
    CPTTheme *theme = [[CPTTheme alloc] init];

    [theme setGraphClass:[CPTXYGraph class]];
    STAssertEquals([CPTXYGraph class], theme.graphClass, @"graphClass should be CPTXYGraph");
    [theme release];
}

-(void)testSetGraphUsingDerivedClassShouldWork
{
    CPTTheme *theme = [[CPTTheme alloc] init];

    [theme setGraphClass:[CPTDerivedXYGraph class]];
    STAssertEquals([CPTDerivedXYGraph class], theme.graphClass, @"graphClass should be CPTDerivedXYGraph");
    [theme release];
}

-(void)testSetGraphUsingCPTGraphShouldThrowException
{
    CPTTheme *theme = [[CPTTheme alloc] init];

    @try {
        STAssertThrowsSpecificNamed([theme setGraphClass:[CPTGraph class]], NSException, CPTException, @"Should raise CPTException for wrong kind of class");
    }
    @finally {
        STAssertNil(theme.graphClass, @"graphClass should be nil.");
        [theme release];
    }
}

-(void)testThemeNamedRandomNameShouldReturnNil
{
    CPTTheme *theme = [CPTTheme themeNamed:@"not a theme"];

    STAssertNil(theme, @"Should be nil");
}

-(void)testThemeNamedDarkGradientShouldReturnCPTDarkGradientTheme
{
    CPTTheme *theme = [CPTTheme themeNamed:kCPTDarkGradientTheme];

    STAssertTrue([theme isKindOfClass:[_CPTDarkGradientTheme class]], @"Should be _CPTDarkGradientTheme");
}

-(void)testThemeNamedPlainBlackShouldReturnCPTPlainBlackTheme
{
    CPTTheme *theme = [CPTTheme themeNamed:kCPTPlainBlackTheme];

    STAssertTrue([theme isKindOfClass:[_CPTPlainBlackTheme class]], @"Should be _CPTPlainBlackTheme");
}

-(void)testThemeNamedPlainWhiteShouldReturnCPTPlainWhiteTheme
{
    CPTTheme *theme = [CPTTheme themeNamed:kCPTPlainWhiteTheme];

    STAssertTrue([theme isKindOfClass:[_CPTPlainWhiteTheme class]], @"Should be _CPTPlainWhiteTheme");
}

-(void)testThemeNamedStocksShouldReturnCPTStocksTheme
{
    CPTTheme *theme = [CPTTheme themeNamed:kCPTStocksTheme];

    STAssertTrue([theme isKindOfClass:[_CPTStocksTheme class]], @"Should be _CPTStocksTheme");
}

-(void)testThemeNamedSlateShouldReturnCPTSlateTheme
{
    CPTTheme *theme = [CPTTheme themeNamed:kCPTSlateTheme];

    STAssertTrue([theme isKindOfClass:[_CPTSlateTheme class]], @"Should be _CPTSlateTheme");
}

@end

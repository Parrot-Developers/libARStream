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
#import "CPTAxisLabel.h"
#import "CPTAxisLabelTests.h"
#import "CPTBorderedLayer.h"
#import "CPTMutableTextStyle.h"
#import "CPTUtilities.h"
#import <tgmath.h>

static const double precision = 1.0e-6;

@implementation CPTAxisLabelTests

static CGPoint roundPoint(CGPoint position, CGSize contentSize, CGPoint anchor);

static CGPoint roundPoint(CGPoint position, CGSize contentSize, CGPoint anchor)
{
    CGPoint newPosition = position;

    CGPoint newAnchor = CGPointMake(contentSize.width * anchor.x,
                                    contentSize.height * anchor.y);

    newPosition.x = round( position.x + anchor.x - newAnchor.x - CPTFloat(0.5) ) + newAnchor.x;
    newPosition.y = round( position.y + anchor.y - newAnchor.y - CPTFloat(0.5) ) + newAnchor.y;

    return newPosition;
}

-(void)testPositionRelativeToViewPointRaisesForInvalidDirection
{
    CPTAxisLabel *label;

    @try {
        label = [[CPTAxisLabel alloc] initWithText:@"CPTAxisLabelTests-testPositionRelativeToViewPointRaisesForInvalidDirection" textStyle:[CPTTextStyle textStyle]];

        STAssertThrowsSpecificNamed([label positionRelativeToViewPoint:CGPointZero forCoordinate:CPTCoordinateX inDirection:INT_MAX], NSException, NSInvalidArgumentException, @"Should raise NSInvalidArgumentException for invalid direction (type CPTSign)");
    }
    @finally {
        [label release];
    }
}

-(void)testPositionRelativeToViewPointPositionsForXCoordinate
{
    CPTAxisLabel *label;
    CGFloat start = 100.0;

    @try {
        label = [[CPTAxisLabel alloc] initWithText:@"CPTAxisLabelTests-testPositionRelativeToViewPointPositionsForXCoordinate" textStyle:[CPTTextStyle textStyle]];
        CPTLayer *contentLayer = label.contentLayer;
        CGSize contentSize     = contentLayer.bounds.size;
        label.offset = 20.0;

        CGPoint viewPoint = CGPointMake(start, start);

        contentLayer.anchorPoint = CGPointZero;
        contentLayer.position    = CGPointZero;
        [label positionRelativeToViewPoint:viewPoint
                             forCoordinate:CPTCoordinateX
                               inDirection:CPTSignNone];

        CGPoint newPosition = roundPoint(CGPointMake(start - label.offset, start), contentSize, contentLayer.anchorPoint);

        STAssertEqualsWithAccuracy( contentLayer.position.x, newPosition.x, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy( contentLayer.position.y, newPosition.y, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.x, CPTFloat(1.0), precision, @"Should anchor at (1.0, 0.5)");
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.y, CPTFloat(0.5), precision, @"Should anchor at (1.0, 0.5)");

        contentLayer.anchorPoint = CGPointZero;
        contentLayer.position    = CGPointZero;
        [label positionRelativeToViewPoint:viewPoint
                             forCoordinate:CPTCoordinateX
                               inDirection:CPTSignNegative];

        newPosition = roundPoint(CGPointMake(start - label.offset, start), contentSize, contentLayer.anchorPoint);

        STAssertEqualsWithAccuracy( contentLayer.position.x, newPosition.x, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy( contentLayer.position.y, newPosition.y, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.x, CPTFloat(1.0), precision, @"Should anchor at (1.0, 0.5)");
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.y, CPTFloat(0.5), precision, @"Should anchor at (1.0, 0.5)");

        contentLayer.anchorPoint = CGPointZero;
        contentLayer.position    = CGPointZero;
        [label positionRelativeToViewPoint:viewPoint
                             forCoordinate:CPTCoordinateX
                               inDirection:CPTSignPositive];

        newPosition = roundPoint(CGPointMake(start + label.offset, start), contentSize, contentLayer.anchorPoint);

        STAssertEqualsWithAccuracy( contentLayer.position.x, newPosition.x, precision, @"Should add positive offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy( contentLayer.position.y, newPosition.y, precision, @"Should add positive offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.x, CPTFloat(0.0), precision, @"Should anchor at (0.0, 0.5)");
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.y, CPTFloat(0.5), precision, @"Should anchor at (0.0, 0.5)");
    }
    @finally {
        [label release];
    }
}

-(void)testPositionRelativeToViewPointPositionsForYCoordinate
{
    CPTAxisLabel *label;
    CGFloat start = 100.0;

    @try {
        label = [[CPTAxisLabel alloc] initWithText:@"CPTAxisLabelTests-testPositionRelativeToViewPointPositionsForYCoordinate" textStyle:[CPTTextStyle textStyle]];
        CPTLayer *contentLayer = label.contentLayer;
        CGSize contentSize     = contentLayer.bounds.size;
        label.offset = 20.0;

        CGPoint viewPoint = CGPointMake(start, start);

        contentLayer.anchorPoint = CGPointZero;
        contentLayer.position    = CGPointZero;
        [label positionRelativeToViewPoint:viewPoint
                             forCoordinate:CPTCoordinateY
                               inDirection:CPTSignNone];

        CGPoint newPosition = roundPoint(CGPointMake(start, start - label.offset), contentSize, contentLayer.anchorPoint);

        STAssertEqualsWithAccuracy( contentLayer.position.x, newPosition.x, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy( contentLayer.position.y, newPosition.y, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.x, CPTFloat(0.5), precision, @"Should anchor at (0.5, 1.0)");
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.y, CPTFloat(1.0), precision, @"Should anchor at (0.5, 1.0)");

        contentLayer.anchorPoint = CGPointZero;
        contentLayer.position    = CGPointZero;
        [label positionRelativeToViewPoint:viewPoint
                             forCoordinate:CPTCoordinateY
                               inDirection:CPTSignNegative];

        newPosition = roundPoint(CGPointMake(start, start - label.offset), contentSize, contentLayer.anchorPoint);

        STAssertEqualsWithAccuracy( contentLayer.position.x, newPosition.x, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy( contentLayer.position.y, newPosition.y, precision, @"Should add negative offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.x, CPTFloat(0.5), precision, @"Should anchor at (0.5, 1.0)");
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.y, CPTFloat(1.0), precision, @"Should anchor at (0.5, 1.0)");

        contentLayer.anchorPoint = CGPointZero;
        contentLayer.position    = CGPointZero;
        [label positionRelativeToViewPoint:viewPoint
                             forCoordinate:CPTCoordinateY
                               inDirection:CPTSignPositive];

        newPosition = roundPoint(CGPointMake(start, start + label.offset), contentSize, contentLayer.anchorPoint);

        STAssertEqualsWithAccuracy( contentLayer.position.x, newPosition.x, precision, @"Should add positive offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy( contentLayer.position.y, newPosition.y, precision, @"Should add positive offset, %@ != %@", CPTStringFromPoint(contentLayer.position), CPTStringFromPoint(newPosition) );
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.x, CPTFloat(0.5), precision, @"Should anchor at (0.5, 0.0)");
        STAssertEqualsWithAccuracy(contentLayer.anchorPoint.y, CPTFloat(0.0), precision, @"Should anchor at (0.5, 0.0)");
    }
    @finally {
        [label release];
    }
}

@end

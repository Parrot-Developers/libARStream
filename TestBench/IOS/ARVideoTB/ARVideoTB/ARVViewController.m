//
//  ARVViewController.m
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARVViewController.h"

#import "ARVIDEO_Reader_TestBench.h"
#import "ARVIDEO_Sender_TestBench.h"

#define kARVIDEONumberOfPoints 50
#define kARVIDEODeltaTTimerSec 0.1

@interface ARVViewController ()

@end

@implementation ARVViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Create graph from theme
    latencyGraph = [[CPTXYGraph alloc] initWithFrame:CGRectZero];
    CPTTheme *theme = [CPTTheme themeNamed:kCPTPlainWhiteTheme];
    [latencyGraph applyTheme:theme];
    latencyGraphView.collapsesLayers = NO; // Setting to YES reduces GPU memory usage, but can slow drawing/scrolling
    latencyGraphView.hostedGraph     = latencyGraph;
    
    latencyGraph.paddingLeft   = 1.0;
    latencyGraph.paddingTop    = 1.0;
    latencyGraph.paddingRight  = 1.0;
    latencyGraph.paddingBottom = 1.0;
    
    // Setup plot space
    CPTXYPlotSpace *plotSpace = (CPTXYPlotSpace *)latencyGraph.defaultPlotSpace;
    plotSpace.allowsUserInteraction = NO;
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARVIDEONumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(1000.0)];
    
    // Axes
    CPTXYAxisSet *axisSet = (CPTXYAxisSet *)latencyGraph.axisSet;
    CPTXYAxis *y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromString(@"100");
    y.minorTicksPerInterval       = 1;
    
    // Create an orange plot area
    CPTScatterPlot *boundLinePlot  = [[CPTScatterPlot alloc] init];
    CPTMutableLineStyle *lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor orangeColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = @"Latency Plot";
    boundLinePlot.dataSource    = self;
    [latencyGraph addPlot:boundLinePlot];
    
    // Do an orange gradient
    CPTColor *areaColor1       = [CPTColor colorWithComponentRed:1.0 green:0.3 blue:0.0 alpha:0.8];
    CPTGradient *areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    CPTFill *areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    latencyGraphData = [[NSMutableArray alloc] initWithCapacity:kARVIDEONumberOfPoints];
    for (int i = 0; i < kARVIDEONumberOfPoints; i++)
    {
        [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:0]];
    }
    
    
    // Create graph from theme
    lossFramesGraph = [[CPTXYGraph alloc] initWithFrame:CGRectZero];
    [lossFramesGraph applyTheme:theme];
    lossFramesView.collapsesLayers = NO; // Setting to YES reduces GPU memory usage, but can slow drawing/scrolling
    lossFramesView.hostedGraph     = lossFramesGraph;
    
    lossFramesGraph.paddingLeft   = 1.0;
    lossFramesGraph.paddingTop    = 1.0;
    lossFramesGraph.paddingRight  = 1.0;
    lossFramesGraph.paddingBottom = 1.0;
    
    // Setup plot space
    plotSpace = (CPTXYPlotSpace *)lossFramesGraph.defaultPlotSpace;
    plotSpace.allowsUserInteraction = NO;
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARVIDEONumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(10.0)];
    
    // Axes
    axisSet = (CPTXYAxisSet *)lossFramesGraph.axisSet;
    y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromString(@"1");
    y.minorTicksPerInterval       = 1;
    
    // Create a blue plot area
    boundLinePlot  = [[CPTScatterPlot alloc] init];
    lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor blueColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = @"Loss Plot";
    boundLinePlot.dataSource    = self;
    [lossFramesGraph addPlot:boundLinePlot];
    
    // Do a blue gradient
    areaColor1       = [CPTColor colorWithComponentRed:0.3 green:0.3 blue:1.0 alpha:0.8];
    areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    lossFramesData = [[NSMutableArray alloc] initWithCapacity:kARVIDEONumberOfPoints];
    for (int i = 0; i < kARVIDEONumberOfPoints; i++)
    {
        [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:0]];
    }
}

-(NSUInteger)numberOfRecordsForPlot:(CPTPlot *)plot
{
    NSString *plotId = (NSString *)plot.identifier;
    if ([plotId isEqualToString:@"Loss Plot"])
    {
        return [lossFramesData count];
    }
    else if ([plotId isEqualToString:@"Latency Plot"])
    {
        return [latencyGraphData count];
    }
    else
    {
        return 0;
    }
}

-(NSNumber *)numberForPlot:(CPTPlot *)plot field:(NSUInteger)fieldEnum recordIndex:(NSUInteger)index
{
    NSNumber *retval = nil;
    NSString *plotId = (NSString *)plot.identifier;
    if ([plotId isEqualToString:@"Loss Plot"])
    {
        if (CPTScatterPlotFieldX == fieldEnum)
        {
            retval = [NSNumber numberWithUnsignedInteger:index];
        }
        else
        {
            retval = [lossFramesData objectAtIndex:index];
        }
    }
    else if ([plotId isEqualToString:@"Latency Plot"])
    {
        if (CPTScatterPlotFieldX == fieldEnum)
        {
            retval = [NSNumber numberWithUnsignedInteger:index];
        }
        else
        {
            retval = [latencyGraphData objectAtIndex:index];
        }
    }
    return retval;
}

- (void)viewDidAppear:(BOOL)animated
{
    _running = NO;
    _isSender = NO;
    _refreshTimer = [NSTimer scheduledTimerWithTimeInterval:kARVIDEODeltaTTimerSec target:self selector:@selector(updateStatus) userInfo:nil repeats:YES];
}

- (void)viewDidDisappear:(BOOL)animated
{
    [_refreshTimer invalidate];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void) updateStatus
{
    if (!_running)
    {
        [percentOk setText:@""];
        [latency setText:@""];
    }
    else
    {
        if (_isSender)
        {
            [percentOk setText:[NSString stringWithFormat:@"%f%%", ARVIDEO_Sender_PercentOk]];
        }
        else
        {
            [percentOk setText:[NSString stringWithFormat:@"%f%%", ARVIDEO_Reader_PercentOk]];
            int estLat = ARVIDEO_ReaderTb_GetLatency();
            if (estLat < 0)
            {
                [latency setText:@"Not connected"];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:1000]];
                [latencyGraph reloadData];
            }
            else
            {
                [latency setText:[NSString stringWithFormat:@"%dms", estLat]];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:estLat]];
                [latencyGraph reloadData];

            }
            if ([lossFramesData count] > 0)
                [lossFramesData removeObjectAtIndex:0];
            int missed = ARVIDEO_ReaderTb_GetMissedFrames();
            [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)missed]];
            [lossFramesGraph reloadData];
        }
    }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return NO;
}

- (void)senderGo:(id)sender
{
    [senderButton setEnabled:NO];
    [readerButton setEnabled:NO];
    _running = YES;
    _isSender = YES;
    [self performSelectorInBackground:@selector(runSender) withObject:nil];
}

- (void)runSender
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARVideo_TestBench_iOS";
    char *params[2] = { name, cIp };
    ARVIDEO_Sender_TestBenchMain(2, params);
}

- (void)readerGo:(id)sender
{
    [senderButton setEnabled:NO];
    [readerButton setEnabled:NO];
    _running = YES;
    _isSender = NO;
    [self performSelectorInBackground:@selector(runReader) withObject:nil];
}

- (void)runReader
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARVideo_TestBench_iOS";
    char *params[2] = { name, cIp };
    ARVIDEO_Reader_TestBenchMain(2, params);}

@end

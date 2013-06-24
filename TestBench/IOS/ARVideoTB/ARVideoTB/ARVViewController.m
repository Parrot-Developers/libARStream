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
    
    nbGraphs = 0;
    
    // Create graph from theme
    nbGraphs++;
    latencyGraph = [[CPTXYGraph alloc] initWithFrame:CGRectZero];
    CPTTheme *theme = [CPTTheme themeNamed:kCPTPlainWhiteTheme];
    [latencyGraph applyTheme:theme];
    latencyView.collapsesLayers = NO; // Setting to YES reduces GPU memory usage, but can slow drawing/scrolling
    latencyView.hostedGraph     = latencyGraph;
    
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
    y.minorTicksPerInterval       = 0;
    CPTXYAxis *x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARVIDEONumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
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
    
    // Create graph from theme
    nbGraphs++;
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
    y.minorTicksPerInterval       = 0;
    x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARVIDEONumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
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
    
    // Create graph from theme
    nbGraphs++;
    deltaTGraph = [[CPTXYGraph alloc] initWithFrame:CGRectZero];
    [deltaTGraph applyTheme:theme];
    deltaTView.collapsesLayers = NO; // Setting to YES reduces GPU memory usage, but can slow drawing/scrolling
    deltaTView.hostedGraph     = deltaTGraph;
    
    deltaTGraph.paddingLeft   = 1.0;
    deltaTGraph.paddingTop    = 1.0;
    deltaTGraph.paddingRight  = 1.0;
    deltaTGraph.paddingBottom = 1.0;
    
    // Setup plot space
    plotSpace = (CPTXYPlotSpace *)deltaTGraph.defaultPlotSpace;
    plotSpace.allowsUserInteraction = NO;
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARVIDEONumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(100.f)];
    
    // Axes
    axisSet = (CPTXYAxisSet *)deltaTGraph.axisSet;
    y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromString(@"10");
    y.minorTicksPerInterval       = 0;
    x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARVIDEONumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
    // Create a blue plot area
    boundLinePlot  = [[CPTScatterPlot alloc] init];
    lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor greenColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = @"Delta Plot";
    boundLinePlot.dataSource    = self;
    [deltaTGraph addPlot:boundLinePlot];
    
    // Do a blue gradient
    areaColor1       = [CPTColor colorWithComponentRed:0.3 green:1.0 blue:0.3 alpha:0.8];
    areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    deltaTData = [[NSMutableArray alloc] initWithCapacity:kARVIDEONumberOfPoints];
    
    // Create graph from theme
    nbGraphs++;
    efficiencyGraph = [[CPTXYGraph alloc] initWithFrame:CGRectZero];
    [efficiencyGraph applyTheme:theme];
    efficiencyView.collapsesLayers = NO; // Setting to YES reduces GPU memory usage, but can slow drawing/scrolling
    efficiencyView.hostedGraph     = efficiencyGraph;
    
    efficiencyGraph.paddingLeft   = 1.0;
    efficiencyGraph.paddingTop    = 1.0;
    efficiencyGraph.paddingRight  = 1.0;
    efficiencyGraph.paddingBottom = 1.0;
    
    // Setup plot space
    plotSpace = (CPTXYPlotSpace *)efficiencyGraph.defaultPlotSpace;
    plotSpace.allowsUserInteraction = NO;
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARVIDEONumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(1.f)];
    
    // Axes
    axisSet = (CPTXYAxisSet *)efficiencyGraph.axisSet;
    y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromString(@"0.1");
    y.minorTicksPerInterval       = 0;
    x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARVIDEONumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
    // Create a yellow plot area
    boundLinePlot  = [[CPTScatterPlot alloc] init];
    lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor yellowColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = @"Eff Plot";
    boundLinePlot.dataSource    = self;
    [efficiencyGraph addPlot:boundLinePlot];
    
    // Do a yellow gradient
    areaColor1       = [CPTColor colorWithComponentRed:0.8 green:0.8 blue:0.3 alpha:0.8];
    areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    efficiencyData = [[NSMutableArray alloc] initWithCapacity:kARVIDEONumberOfPoints];
    
    [self resetDataArrays];
}

- (void)resetDataArrays
{
    [lossFramesData removeAllObjects];
    [latencyGraphData removeAllObjects];
    [deltaTData removeAllObjects];
    [efficiencyData removeAllObjects];
    for (int i = 0; i < kARVIDEONumberOfPoints; i++)
    {
        [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:0]];
        [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:0]];
        [deltaTData addObject:[NSNumber numberWithUnsignedInteger:0]];
        [efficiencyData addObject:[NSNumber numberWithUnsignedInteger:0]];
    }
    [self refreshGraphs];
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
    else if ([plotId isEqualToString:@"Delta Plot"])
    {
        return [deltaTData count];
    }
    else if ([plotId isEqualToString:@"Eff Plot"])
    {
        return [efficiencyData count];
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
    else if ([plotId isEqualToString:@"Delta Plot"])
    {
        if (CPTScatterPlotFieldX == fieldEnum)
        {
            retval = [NSNumber numberWithUnsignedInteger:index];
        }
        else
        {
            retval = [deltaTData objectAtIndex:index];
        }
    }
    else if ([plotId isEqualToString:@"Eff Plot"])
    {
        if (CPTScatterPlotFieldX == fieldEnum)
        {
            retval = [NSNumber numberWithUnsignedInteger:index];
        }
        else
        {
            retval = [efficiencyData objectAtIndex:index];
        }
    }
    return retval;
}

- (void)viewDidAppear:(BOOL)animated
{
    _running = NO;
    _isSender = NO;
    
    CGSize contentSize = CGSizeMake(graphScrollView.frame.size.width, 108.f * nbGraphs);
    [graphScrollView setContentSize:contentSize];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void) refreshGraphs
{
    if (! [NSThread isMainThread])
    {
        [self performSelectorOnMainThread:@selector(refreshGraphs) withObject:nil waitUntilDone:NO];
    }
    else
    {
        [latencyGraph reloadData];
        [lossFramesGraph reloadData];
        [deltaTGraph reloadData];
        [efficiencyGraph reloadData];
    }
}

- (void) startTimer
{
    _refreshTimer = [NSTimer timerWithTimeInterval:kARVIDEODeltaTTimerSec target:self selector:@selector(updateStatus) userInfo:nil repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer:_refreshTimer forMode:NSRunLoopCommonModes];
}

- (void) stopTimer
{
    [_refreshTimer invalidate];
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
            int estLat = ARVIDEO_SenderTb_GetLatency();
            if (estLat < 0)
            {
                [latency setText:@"Not connected"];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:1000]];
            }
            else
            {
                [latency setText:[NSString stringWithFormat:@"%dms", estLat]];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:estLat]];
                
            }
            if ([lossFramesData count] > 0)
                [lossFramesData removeObjectAtIndex:0];
            int missed = ARVIDEO_SenderTb_GetMissedFrames();
            [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)missed]];
            
            if ([deltaTData count] > 0)
                [deltaTData removeObjectAtIndex:0];
            int dt = ARVIDEO_SenderTb_GetMeanTimeBetweenFrames();
            [deltaTData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)dt]];
            
            if ([efficiencyData count] > 0)
                [efficiencyData removeObjectAtIndex:0];
            float eff = ARVIDEO_SenderTb_GetEfficiency();
            [efficiencyData addObject:[NSNumber numberWithFloat:eff]];
            
            [self refreshGraphs];
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
            }
            else
            {
                [latency setText:[NSString stringWithFormat:@"%dms", estLat]];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:estLat]];

            }
            if ([lossFramesData count] > 0)
                [lossFramesData removeObjectAtIndex:0];
            int missed = ARVIDEO_ReaderTb_GetMissedFrames();
            [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)missed]];
            
            if ([deltaTData count] > 0)
                [deltaTData removeObjectAtIndex:0];
            int dt = ARVIDEO_ReaderTb_GetMeanTimeBetweenFrames();
            [deltaTData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)dt]];
            
            if ([efficiencyData count] > 0)
                [efficiencyData removeObjectAtIndex:0];
            float eff = ARVIDEO_ReaderTb_GetEfficiency();
            [efficiencyData addObject:[NSNumber numberWithFloat:eff]];
            
            [self refreshGraphs];
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
    [senderButton setHidden:YES];
    [readerButton setHidden:YES];
    [stopButton setHidden:NO];
    [stopButton setTitle:@"STOP - SENDER" forState:UIControlStateNormal];
    
    [self startTimer];
    
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
    [senderButton setHidden:YES];
    [readerButton setHidden:YES];
    [stopButton setHidden:NO];
    [stopButton setTitle:@"STOP - READER" forState:UIControlStateNormal];
    
    [self startTimer];
    
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
    ARVIDEO_Reader_TestBenchMain(2, params);
}

- (void)stop:(id)sender
{
    ARVIDEO_Sender_TestBenchStop();
    ARVIDEO_Reader_TestBenchStop();
    [self resetDataArrays];
    [self stopTimer];
    
    
    [senderButton setHidden:NO];
    [readerButton setHidden:NO];
    [stopButton setHidden:YES];
}

@end

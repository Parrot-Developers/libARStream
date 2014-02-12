//
//  ARSViewController.m
//  ARStreamTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSViewController.h"

#import "ARSTREAM_Reader_TestBench.h"
#import "ARSTREAM_Sender_TestBench.h"
#import "ARSTREAM_TCPReader.h"
#import "ARSTREAM_TCPSender.h"

#define kARSTREAMNumberOfPoints 50
#define kARSTREAMDeltaTTimerSec 0.1

#define kLATENCY_PLOT_NAME @"Latency"
#define kLOSS_PLOT_NAME @"Loss"
#define kDELTAT_PLOT_NAME @"DeltatT"
#define kEFFICIENCY_PLOT_NAME @"Efficiency"

@interface ARSViewController ()

@end

@implementation ARSViewController

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
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARSTREAMNumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(1000.0)];
    
    // Axes
    CPTXYAxisSet *axisSet = (CPTXYAxisSet *)latencyGraph.axisSet;
    CPTXYAxis *y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromFloat(100.0);
    y.minorTicksPerInterval       = 0;
    CPTXYAxis *x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARSTREAMNumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
    // Create an orange plot area
    CPTScatterPlot *boundLinePlot  = [[CPTScatterPlot alloc] init];
    CPTMutableLineStyle *lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor orangeColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = kLATENCY_PLOT_NAME;
    boundLinePlot.dataSource    = self;
    [latencyGraph addPlot:boundLinePlot];
    
    // Do an orange gradient
    CPTColor *areaColor1       = [CPTColor colorWithComponentRed:1.0 green:0.3 blue:0.0 alpha:0.8];
    CPTGradient *areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    CPTFill *areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    latencyGraphData = [[NSMutableArray alloc] initWithCapacity:kARSTREAMNumberOfPoints];
    
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
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARSTREAMNumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(10.0)];
    
    // Axes
    axisSet = (CPTXYAxisSet *)lossFramesGraph.axisSet;
    y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromFloat(1.0);
    y.minorTicksPerInterval       = 0;
    x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARSTREAMNumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
    // Create a blue plot area
    boundLinePlot  = [[CPTScatterPlot alloc] init];
    lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor blueColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = kLOSS_PLOT_NAME;
    boundLinePlot.dataSource    = self;
    [lossFramesGraph addPlot:boundLinePlot];
    
    // Do a blue gradient
    areaColor1       = [CPTColor colorWithComponentRed:0.3 green:0.3 blue:1.0 alpha:0.8];
    areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    lossFramesData = [[NSMutableArray alloc] initWithCapacity:kARSTREAMNumberOfPoints];
    
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
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARSTREAMNumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(100.f)];
    
    // Axes
    axisSet = (CPTXYAxisSet *)deltaTGraph.axisSet;
    y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromFloat(10.0);
    y.minorTicksPerInterval       = 0;
    x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARSTREAMNumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
    // Create a blue plot area
    boundLinePlot  = [[CPTScatterPlot alloc] init];
    lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor greenColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = kDELTAT_PLOT_NAME;
    boundLinePlot.dataSource    = self;
    [deltaTGraph addPlot:boundLinePlot];
    
    // Do a blue gradient
    areaColor1       = [CPTColor colorWithComponentRed:0.3 green:1.0 blue:0.3 alpha:0.8];
    areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    deltaTData = [[NSMutableArray alloc] initWithCapacity:kARSTREAMNumberOfPoints];
    
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
    plotSpace.xRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(kARSTREAMNumberOfPoints * 1.f)];
    plotSpace.yRange                = [CPTPlotRange plotRangeWithLocation:CPTDecimalFromFloat(0.0) length:CPTDecimalFromFloat(1.f)];
    
    // Axes
    axisSet = (CPTXYAxisSet *)efficiencyGraph.axisSet;
    y = axisSet.yAxis;
    y.majorIntervalLength         = CPTDecimalFromFloat(0.1);
    y.minorTicksPerInterval       = 0;
    x = axisSet.xAxis;
    x.majorIntervalLength         = CPTDecimalFromInt(kARSTREAMNumberOfPoints);
    x.minorTicksPerInterval       = 0;
    
    // Create a yellow plot area
    boundLinePlot  = [[CPTScatterPlot alloc] init];
    lineStyle = [CPTMutableLineStyle lineStyle];
    lineStyle.miterLimit        = 1.0f;
    lineStyle.lineWidth         = 3.0f;
    lineStyle.lineColor         = [CPTColor yellowColor];
    boundLinePlot.dataLineStyle = lineStyle;
    boundLinePlot.identifier    = kEFFICIENCY_PLOT_NAME;
    boundLinePlot.dataSource    = self;
    [efficiencyGraph addPlot:boundLinePlot];
    
    // Do a yellow gradient
    areaColor1       = [CPTColor colorWithComponentRed:0.8 green:0.8 blue:0.3 alpha:0.8];
    areaGradient1 = [CPTGradient gradientWithBeginningColor:areaColor1 endingColor:[CPTColor clearColor]];
    areaGradient1.angle = -90.0f;
    areaGradientFill = [CPTFill fillWithGradient:areaGradient1];
    boundLinePlot.areaFill      = areaGradientFill;
    boundLinePlot.areaBaseValue = [[NSDecimalNumber zero] decimalValue];
    
    efficiencyData = [[NSMutableArray alloc] initWithCapacity:kARSTREAMNumberOfPoints];
    
    [self resetDataArrays];
}

- (void)resetDataArrays
{
    [lossFramesData removeAllObjects];
    [latencyGraphData removeAllObjects];
    [deltaTData removeAllObjects];
    [efficiencyData removeAllObjects];
    for (int i = 0; i < kARSTREAMNumberOfPoints; i++)
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
    if ([plotId isEqualToString:kLOSS_PLOT_NAME])
    {
        return [lossFramesData count];
    }
    else if ([plotId isEqualToString:kLATENCY_PLOT_NAME])
    {
        return [latencyGraphData count];
    }
    else if ([plotId isEqualToString:kDELTAT_PLOT_NAME])
    {
        return [deltaTData count];
    }
    else if ([plotId isEqualToString:kEFFICIENCY_PLOT_NAME])
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
    if ([plotId isEqualToString:kLOSS_PLOT_NAME])
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
    else if ([plotId isEqualToString:kLATENCY_PLOT_NAME])
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
    else if ([plotId isEqualToString:kDELTAT_PLOT_NAME])
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
    else if ([plotId isEqualToString:kEFFICIENCY_PLOT_NAME])
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
    _mode = ARS_None;
    
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
    _refreshTimer = [NSTimer timerWithTimeInterval:kARSTREAMDeltaTTimerSec target:self selector:@selector(updateStatus) userInfo:nil repeats:YES];
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
        switch (_mode)
        {
            case ARS_Sender:
            {
                [percentOk setText:[NSString stringWithFormat:@"%d%%", ARSTREAM_SenderTb_GetEstimatedLoss()]];
                int estLat = ARSTREAM_SenderTb_GetLatency();
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
                int missed = ARSTREAM_SenderTb_GetMissedFrames();
                [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)missed]];
                
                if ([deltaTData count] > 0)
                    [deltaTData removeObjectAtIndex:0];
                int dt = ARSTREAM_SenderTb_GetMeanTimeBetweenFrames();
                [deltaTData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)dt]];
                
                if ([efficiencyData count] > 0)
                    [efficiencyData removeObjectAtIndex:0];
                float eff = ARSTREAM_SenderTb_GetEfficiency();
                [efficiencyData addObject:[NSNumber numberWithFloat:eff]];
                
                [self refreshGraphs];
                [logger log:[NSString stringWithFormat:@"%4d; %5.2f; %3d; %4d; %5.3f", estLat, ARSTREAM_Sender_PercentOk, missed, dt, eff]];
            }
                break;
            case ARS_Reader:
            {
                [percentOk setText:[NSString stringWithFormat:@"%d%%", ARSTREAM_ReaderTb_GetEstimatedLoss()]];
                int estLat = ARSTREAM_ReaderTb_GetLatency();
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
                int missed = ARSTREAM_ReaderTb_GetMissedFrames();
                [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)missed]];
                
                if ([deltaTData count] > 0)
                    [deltaTData removeObjectAtIndex:0];
                int dt = ARSTREAM_ReaderTb_GetMeanTimeBetweenFrames();
                [deltaTData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)dt]];
                
                if ([efficiencyData count] > 0)
                    [efficiencyData removeObjectAtIndex:0];
                float eff = ARSTREAM_ReaderTb_GetEfficiency();
                [efficiencyData addObject:[NSNumber numberWithFloat:eff]];
                
                [self refreshGraphs];
                [logger log:[NSString stringWithFormat:@"%4d; %5.2f; %3d; %4d; %5.3f", estLat, ARSTREAM_Reader_PercentOk, missed, dt, eff]];
            }
                break;
            case ARS_TCP_Sender:
                // Do nothing for TCP sender
                [percentOk setText:@""];
                [latency setText:@""];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:0]];
                if ([lossFramesData count] > 0)
                    [lossFramesData removeObjectAtIndex:0];
                [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:0]];
                if ([deltaTData count] > 0)
                    [deltaTData removeObjectAtIndex:0];
                [deltaTData addObject:[NSNumber numberWithUnsignedInteger:0]];
                if ([efficiencyData count] > 0)
                    [efficiencyData removeObjectAtIndex:0];
                [efficiencyData addObject:[NSNumber numberWithFloat:0.0]];
                
                [self refreshGraphs];
                
                break;
            case ARS_TCP_Reader:
                [percentOk setText:@""];
                [latency setText:@""];
                if ([latencyGraphData count] > 0)
                    [latencyGraphData removeObjectAtIndex:0];
                [latencyGraphData addObject:[NSNumber numberWithUnsignedInteger:0]];
                if ([lossFramesData count] > 0)
                    [lossFramesData removeObjectAtIndex:0];
                [lossFramesData addObject:[NSNumber numberWithUnsignedInteger:0]];
                if ([deltaTData count] > 0)
                    [deltaTData removeObjectAtIndex:0];
                int dt = ARSTREAM_TCPReader_GetMeanTimeBetweenFrames ();
                [deltaTData addObject:[NSNumber numberWithUnsignedInteger:(NSUInteger)dt]];
                if ([efficiencyData count] > 0)
                    [efficiencyData removeObjectAtIndex:0];
                [efficiencyData addObject:[NSNumber numberWithFloat:0.0]];
                
                [logger log:[NSString stringWithFormat:@"%4d", dt]];
                
                [self refreshGraphs];
                break;
            default:
                break;
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
    [tcpSenderButton setHidden:YES];
    [tcpReaderButton setHidden:YES];
    [stopButton setHidden:NO];
    [stopButton setTitle:@"STOP - SENDER" forState:UIControlStateNormal];
    
    [self startTimer];
    
    _running = YES;
    _mode = ARS_Sender;
    
    [self performSelectorInBackground:@selector(runSender) withObject:nil];
}

- (void)runSender
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARStream_TestBench_iOS";
    char *params[2] = { name, cIp };
    
    logger = [[ARSLogger alloc] init];
    [logger log:@"Latency (ms); PercentOK (%%); Missed frames; Mean time between frame (ms); Efficiency"];
    ARSTREAM_Sender_TestBenchMain(2, params);
    [logger close];
    logger = nil;
}

- (void)readerGo:(id)sender
{
    [senderButton setHidden:YES];
    [readerButton setHidden:YES];
    [tcpSenderButton setHidden:YES];
    [tcpReaderButton setHidden:YES];
    [stopButton setHidden:NO];
    [stopButton setTitle:@"STOP - READER" forState:UIControlStateNormal];
    
    [self startTimer];
    
    _running = YES;
    _mode = ARS_Reader;
    
    [self performSelectorInBackground:@selector(runReader) withObject:nil];
}

- (void)runReader
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARStream_TestBench_iOS";
    char *params[2] = { name, cIp };
    
    logger = [[ARSLogger alloc] init];
    [logger log:@"Latency (ms); PercentOK (%%); Missed frames; Mean time between frame (ms); Efficiency"];
    ARSTREAM_Reader_TestBenchMain(2, params);
    [logger close];
    logger = nil;
}

- (void)tcpSenderGo:(id)sender
{
    [senderButton setHidden:YES];
    [readerButton setHidden:YES];
    [tcpSenderButton setHidden:YES];
    [tcpReaderButton setHidden:YES];
    [stopButton setHidden:NO];
    [stopButton setTitle:@"STOP - TCPSENDER" forState:UIControlStateNormal];
    
    [self startTimer];
    
    _running = YES;
    _mode = ARS_TCP_Sender;
    
    [self performSelectorInBackground:@selector(runTcpSender) withObject:nil];
}

- (void)runTcpSender
{
    char *name = "ARStream_TestBench_iOS";
    char *params[1] = { name };
    
    ARSTREAM_TCPSender_Main(1, params);
}

- (void)tcpReaderGo:(id)sender
{
    [senderButton setHidden:YES];
    [readerButton setHidden:YES];
    [tcpSenderButton setHidden:YES];
    [tcpReaderButton setHidden:YES];
    [stopButton setHidden:NO];
    [stopButton setTitle:@"STOP - READER" forState:UIControlStateNormal];
    
    [self startTimer];
    
    _running = YES;
    _mode = ARS_TCP_Reader;
    
    [self performSelectorInBackground:@selector(runTcpReader) withObject:nil];
}

- (void)runTcpReader
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARStream_TestBench_iOS";
    char *params[2] = { name, cIp };
    
    logger = [[ARSLogger alloc] init];
    [logger log:@"Mean time between frame (ms)"];
    ARSTREAM_TCPReader_Main(2, params);
    [logger close];
    logger = nil;
}

- (void)stop:(id)sender
{
    ARSTREAM_Sender_TestBenchStop();
    ARSTREAM_Reader_TestBenchStop();
    ARSTREAM_TCPReader_Stop();
    ARSTREAM_TCPSender_Stop();
    [self resetDataArrays];
    [self stopTimer];
    
    
    [senderButton setHidden:NO];
    [readerButton setHidden:NO];
    [tcpSenderButton setHidden:NO];
    [tcpReaderButton setHidden:NO];
    [stopButton setHidden:YES];
}

- (void)deleteLogs:(id)sender
{
    if (logger == nil)
    {
        logger = [[ARSLogger alloc] initWithoutFile];
    }
    [logger deleteAllLogs];
}

@end
//
//  ARSViewController.h
//  ARStreamTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ARSLogger.h"
#import <CorePlot-CocoaTouch.h>

typedef enum {
    ARS_None,
    ARS_Sender,
    ARS_Reader,
    ARS_TCP_Sender,
    ARS_TCP_Reader,
} ARS_MODE;

@interface ARSViewController : UIViewController <UITextFieldDelegate, CPTPlotDataSource> {
    IBOutlet UITextField *ipField;
    IBOutlet UIButton *senderButton;
    IBOutlet UIButton *readerButton;
    IBOutlet UIButton *tcpSenderButton;
    IBOutlet UIButton *tcpReaderButton;
    IBOutlet UIButton *stopButton;
    IBOutlet UIButton *deleteLogsButton;
    IBOutlet UILabel *percentOk;
    IBOutlet UILabel *latency;
    
    IBOutlet UIScrollView *graphScrollView;
    
    IBOutlet CPTGraphHostingView *latencyView;
    IBOutlet CPTGraphHostingView *lossFramesView;
    IBOutlet CPTGraphHostingView *deltaTView;
    IBOutlet CPTGraphHostingView *efficiencyView;
    
    CPTXYGraph *latencyGraph;
    NSMutableArray *latencyGraphData;
    
    CPTXYGraph *lossFramesGraph;
    NSMutableArray *lossFramesData;
    
    CPTXYGraph *deltaTGraph;
    NSMutableArray *deltaTData;
    
    CPTXYGraph *efficiencyGraph;
    NSMutableArray *efficiencyData;
    
    int nbGraphs;
    
    ARSLogger *logger;
}

- (IBAction)senderGo:(id)sender;
- (IBAction)readerGo:(id)sender;
- (IBAction)tcpSenderGo:(id)sender;
- (IBAction)tcpReaderGo:(id)sender;
- (IBAction)stop:(id)sender;
- (IBAction)deleteLogs:(id)sender;

@property (nonatomic) BOOL running;
@property (nonatomic) ARS_MODE mode;
@property (nonatomic, strong) NSTimer *refreshTimer;

@end

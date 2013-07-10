//
//  ARVViewController.h
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "ARVLogger.h"
#import <CorePlot-CocoaTouch.h>

typedef enum {
    ARV_None,
    ARV_Sender,
    ARV_Reader,
    ARV_TCP_Sender,
    ARV_TCP_Reader,
} ARV_MODE;

@interface ARVViewController : UIViewController <UITextFieldDelegate, CPTPlotDataSource> {
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
    
    ARVLogger *logger;
}

- (IBAction)senderGo:(id)sender;
- (IBAction)readerGo:(id)sender;
- (IBAction)tcpSenderGo:(id)sender;
- (IBAction)tcpReaderGo:(id)sender;
- (IBAction)stop:(id)sender;
- (IBAction)deleteLogs:(id)sender;

@property (nonatomic) BOOL running;
@property (nonatomic) ARV_MODE mode;
@property (nonatomic, strong) NSTimer *refreshTimer;

@end

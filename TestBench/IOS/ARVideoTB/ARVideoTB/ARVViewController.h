//
//  ARVViewController.h
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CorePlot-CocoaTouch.h>

@interface ARVViewController : UIViewController <UITextFieldDelegate, CPTPlotDataSource> {
    IBOutlet UITextField *ipField;
    IBOutlet UIButton *senderButton;
    IBOutlet UIButton *readerButton;
    IBOutlet UIButton *stopButton;
    IBOutlet UILabel *percentOk;
    IBOutlet UILabel *latency;
    IBOutlet CPTGraphHostingView *latencyGraphView;
    IBOutlet CPTGraphHostingView *lossFramesView;
    
    CPTXYGraph *latencyGraph;
    NSMutableArray *latencyGraphData;
    
    CPTXYGraph *lossFramesGraph;
    NSMutableArray *lossFramesData;
    
}

- (IBAction)senderGo:(id)sender;
- (IBAction)readerGo:(id)sender;
- (IBAction)stop:(id)sender;

@property (nonatomic) BOOL running;
@property (nonatomic) BOOL isSender;
@property (nonatomic, strong) NSTimer *refreshTimer;

@end

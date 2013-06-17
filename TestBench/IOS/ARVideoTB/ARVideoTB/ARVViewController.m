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

@interface ARVViewController ()

@end

@implementation ARVViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)viewDidAppear:(BOOL)animated
{
    _running = NO;
    _isSender = NO;
    _refreshTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(updateStatus) userInfo:nil repeats:YES];
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
            }
            else
            {
                [latency setText:[NSString stringWithFormat:@"%dms", estLat]];
            }
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

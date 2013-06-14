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

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)senderGo:(id)sender
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARVideo_TestBench_iOS";
    char *params[2] = { name, cIp };
    ARVIDEO_Sender_TestBenchMain(2, params);
}

- (void)readerGo:(id)sender
{
    NSString *ip = ipField.text;
    char *cIp = (char *)[ip UTF8String];
    char *name = "ARVideo_TestBench_iOS";
    char *params[2] = { name, cIp };
    ARVIDEO_Reader_TestBenchMain(2, params);
}

@end

//
//  ARSIPSelectorViewController.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSIPSelectorViewController.h"
#import "ARSDecoderViewController.h"

@interface ARSIPSelectorViewController ()

@end

@implementation ARSIPSelectorViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    NSString *ip = _ipField.text;
    NSLog (@"Prepare for segue : ip = %@", ip);
    id dest = segue.destinationViewController;
    if ([dest isKindOfClass:[ARSDecoderViewController class]])
    {
        ARSDecoderViewController *vc = (ARSDecoderViewController *)dest;
        [vc setIp:ip];
        [vc setStreamHeight:[_heightField.text intValue]];
        [vc setStreamWidth:[_widthField.text intValue]];
    }
}

@end

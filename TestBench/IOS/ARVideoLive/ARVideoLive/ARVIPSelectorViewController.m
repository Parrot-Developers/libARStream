//
//  ARVIPSelectorViewController.m
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARVIPSelectorViewController.h"
#import "ARVDecoderViewController.h"

@interface ARVIPSelectorViewController ()

@end

@implementation ARVIPSelectorViewController

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
    if ([dest isKindOfClass:[ARVDecoderViewController class]])
    {
        ARVDecoderViewController *vc = (ARVDecoderViewController *)dest;
        [vc setIp:ip];
        [vc setVideoHeight:[_heightField.text intValue]];
        [vc setVideoWidth:[_widthField.text intValue]];
    }
}

@end

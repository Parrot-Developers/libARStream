//
//  ARVViewController.h
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ARVViewController : UIViewController <UITextFieldDelegate> {
    IBOutlet UITextField *ipField;
    IBOutlet UIButton *senderButton;
    IBOutlet UIButton *readerButton;
    IBOutlet UILabel *percentOk;
    IBOutlet UILabel *latency;
}

- (IBAction)senderGo:(id)sender;
- (IBAction)readerGo:(id)sender;

@property (nonatomic) BOOL running;
@property (nonatomic) BOOL isSender;
@property (nonatomic, strong) NSTimer *refreshTimer;

@end

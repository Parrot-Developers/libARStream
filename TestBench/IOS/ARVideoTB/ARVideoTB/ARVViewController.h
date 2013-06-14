//
//  ARVViewController.h
//  ARVideoTB
//
//  Created by Nicolas BRULEZ on 14/06/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ARVViewController : UIViewController {
    IBOutlet UITextField *ipField;
    IBOutlet UIButton *senderButton;
    IBOutlet UIButton *readerButton;
}

- (IBAction)senderGo:(id)sender;
- (IBAction)readerGo:(id)sender;

@end

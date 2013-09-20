//
//  ARSIPSelectorViewController.h
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ARSIPSelectorViewController : UIViewController

@property (nonatomic, strong) IBOutlet UITextField *ipField;
@property (nonatomic, strong) IBOutlet UITextField *widthField;
@property (nonatomic, strong) IBOutlet UITextField *heightField;
@property (nonatomic, strong) IBOutlet UIButton *goButton;

@end

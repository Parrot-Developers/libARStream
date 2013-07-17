//
//  ARVDecoderViewController.h
//  ARVideoLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface ARVDecoderViewController : UIViewController

@property (nonatomic, strong) IBOutlet UILabel *netFps;
@property (nonatomic, strong) IBOutlet UILabel *decFps;
@property (nonatomic, strong) IBOutlet UILabel *disFps;

@property (nonatomic, strong) NSString *ip;
@property (nonatomic) int videoWidth;
@property (nonatomic) int videoHeight;

@end

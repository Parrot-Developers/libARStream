//
//  ARSDecoderViewController.h
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 11/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

@interface ARSDecoderViewController : UIViewController

@property (nonatomic, strong) IBOutlet UILabel *netFps;
@property (nonatomic, strong) IBOutlet UILabel *decFps;
@property (nonatomic, strong) IBOutlet UILabel *disFps;

@property (nonatomic, strong) NSString *ip;
@property (nonatomic) int streamWidth;
@property (nonatomic) int streamHeight;

@end

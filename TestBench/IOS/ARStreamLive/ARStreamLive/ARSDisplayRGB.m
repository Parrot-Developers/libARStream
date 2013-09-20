//
//  ARSDisplayRGB.m
//  ARStreamLive
//
//  Created by Nicolas BRULEZ on 17/07/13.
//  Copyright (c) 2013 parrot. All rights reserved.
//

#import "ARSDisplayRGB.h"

@interface ARSDisplayRGB ()

@property (nonatomic, strong) UIImageView *iw;
@property (nonatomic) CGSize resolution;

@property (nonatomic) uint8_t *frameBuffer;

@property (nonatomic) CGColorSpaceRef colorSpace;
@property (nonatomic) CGContextRef gtx;

@property (nonatomic) UIImage *img;

@property (nonatomic) int bpp;

@property (nonatomic) int nbDisplayed;

@end

@implementation ARSDisplayRGB

- (id)initWithFrame:(CGRect)frame andResolution:(CGSize)res
{
    self = [super init];
    if (self)
    {
        _iw = [[UIImageView alloc] initWithFrame:frame];
        _resolution = res;
        _bpp = 4;
        _frameBuffer = malloc (_bpp * res.width * res.height);
        [self initAlpha];

        _colorSpace = CGColorSpaceCreateDeviceRGB();

        _gtx = CGBitmapContextCreate(&_frameBuffer[0], res.width, res.height, 8, res.width * _bpp, _colorSpace, kCGImageAlphaPremultipliedLast);
    }
    return self;
}

- (void)initAlpha
{
    int nbPixels = _resolution.height * _resolution.width;
    for (int i = 0; i < nbPixels; i++)
    {
        int index = _bpp*i + (_bpp-1);
        _frameBuffer[index] = 0xff;
    }
}

- (void)convert565to888:(uint16_t *)data
{
    int nbPixels = _resolution.height * _resolution.width;
    for (int i = 0; i < nbPixels; i++)
    {
        int srcpix = data[i];
        int r = srcpix >> 11;
        int g = (srcpix & 0x07ff) >> 5;
        int b = (srcpix & 0x001f);

        r <<= 3;
        g <<= 2;
        b <<= 3;

        int index = _bpp * i;
        _frameBuffer[index] = r;
        _frameBuffer[index+1] = g;
        _frameBuffer[index+2] = b;
    }
}

- (void)dealloc
{
    CGContextRelease(_gtx);
    CGColorSpaceRelease(_colorSpace);
    free (_frameBuffer);
}

- (UIImageView *)getView
{
    return _iw;
}

- (void)displayFrame:(ARSFrame *)frame
{
    // Convert frame into RGBA 8888
    [self convert565to888:(uint16_t *)[frame data]];

    // Create the image
    CGImageRef toCGImage = CGBitmapContextCreateImage(_gtx);
    _img = [UIImage imageWithCGImage:toCGImage];
    CGImageRelease(toCGImage);
    [_iw setImage:_img];
    [self incrementNbDisplayed];
}

- (void)incrementNbDisplayed
{
    @synchronized (self)
    {
        _nbDisplayed++;
    }
}

- (int)getAndResetNbDisplayed
{
    int ret = 0;
    @synchronized (self)
    {
        ret = _nbDisplayed;
        _nbDisplayed = 0;
    }
    return ret;
}
@end

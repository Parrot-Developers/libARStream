//
//  GLView.h
//  Vp8Streamer
//
//  Created by Frédéric D'Haeyer on 9/11/13.
//  Copyright (c) 2013 Swift Navigation. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "opengl_structures.h"

@interface GLRenderer_RGB : NSObject
@end

@interface GLRenderer_YUV : NSObject
@end

@interface GLView : UIView
- (id) initWithFrame:(CGRect)frame rendererClass:(Class)rendererClass;
- (void) render:(AROpenGLTexture *)texture;
@end
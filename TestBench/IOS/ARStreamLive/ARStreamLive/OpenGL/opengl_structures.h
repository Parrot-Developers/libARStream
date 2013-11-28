//
//  opengl_structures.h
//  FF3
//
//  Created by Nicolas BRULEZ on 03/10/13.
//  Copyright (c) 2013 Parrot SA. All rights reserved.
//

#ifndef FF3_opengl_structures_h
#define FF3_opengl_structures_h

/**
 * Define
 */
typedef struct
{
    float width;
    float height;
} AROpenGLSize;

/**
 * Define
 */
typedef struct
{
	AROpenGLSize image_size;
	AROpenGLSize texture_size;
	float scaleModelX;
	float scaleModelY;
	float scaleTextureX;
	float scaleTextureY;
	int bytesPerPixel;
	int type;
    void *data;
    unsigned int num_picture_decoded;
    int dataSize;
    int nbDisplayed;
} AROpenGLTexture;

#endif

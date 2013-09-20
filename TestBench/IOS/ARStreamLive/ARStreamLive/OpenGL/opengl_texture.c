//
//  opengl_texture.c
//  ARDroneEngine
//
//  Created by Frédéric D'Haeyer on 10/21/11.
//  Copyright 2011 Parrot SA. All rights reserved.
//

#if USE_OPENGL

#include "opengl_texture.h"
#include <string.h>
#include <stdio.h>

typedef struct _InterleavedVertex_
{
    GLfloat position[3]; // Vertex
    GLfloat texcoord[2];  // Texture Coordinates
} InterleavedVertex;

static InterleavedVertex const vertex[] =
{
    { {1.0f, -1.0f, -1.2f }, { 0.0f, 1.0f } },
    { {-1.0f, -1.0f, -1.2f }, { 0.0f, 0.0f } },
    { {1.0f,  1.0f, -1.2f }, { 1.0f, 1.0f } },
    { {-1.0f,  1.0f, -1.2f }, { 1.0f, 0.0f } }
};

static GLushort const indexes[] = { 0, 1, 2, 3 };

void opengl_texture_init(ARDroneOpenGLTexture *texture)
{
    // Generated the texture
    // Note: this must NOT be done before the initialization of OpenGL (this is why it can NOT be done in "InitializeTexture")
    memset(texture, 0, sizeof(ARDroneOpenGLTexture));
//    vp_os_memset(texture, 0, sizeof(ARDroneOpenGLTexture));
}

void opengl_texture_scale_compute(ARDroneOpenGLTexture *texture, ARDroneSize screen_size, ARDroneScaling scaling)
{
	printf("%s sizes %f, %f, %f, %f\n", __FUNCTION__, texture->image_size.width, texture->image_size.height, texture->texture_size.width, texture->texture_size.height);
	switch(scaling)
	{
		case NO_SCALING:
			texture->scaleModelX = texture->image_size.height / screen_size.width;
			texture->scaleModelY = texture->image_size.width / screen_size.height;
			break;
		case FIT_X:
			texture->scaleModelX = (screen_size.height * texture->image_size.height) / (screen_size.width * texture->image_size.width);
			texture->scaleModelY = 1.0f;
			break;
		case FIT_Y:
			texture->scaleModelX = 1.0f;
			texture->scaleModelY = (screen_size.width * texture->image_size.width) / (screen_size.height * texture->image_size.height);
			break;
		default:
			texture->scaleModelX = 1.0f;
			texture->scaleModelY = 1.0f;
			break;
	}
    
	texture->scaleTextureX = texture->image_size.width / (float)texture->texture_size.width;
	texture->scaleTextureY = texture->image_size.height / (float)texture->texture_size.height;
}

void opengl_texture_draw(ARDroneOpenGLTexture* texture, GLuint program)
{
    static int textureId = 0;
    static int prev_num_picture_decoded = 0;
    if(texture->num_picture_decoded > prev_num_picture_decoded)
    {
        texture->nbDisplayed++;
        glActiveTexture(GL_TEXTURE0);
        // Load the texture in the GPUDis
        glTexImage2D(GL_TEXTURE_2D, 0, texture->format, texture->texture_size.width, texture->texture_size.height, 0, texture->format, texture->type, texture->data);
        textureId = (textureId + 1) % 2;
        prev_num_picture_decoded = texture->num_picture_decoded;
    }
    // Draw the background quad
    glDrawElements(GL_TRIANGLE_STRIP, sizeof(indexes)/sizeof(GLushort), GL_UNSIGNED_SHORT, (void*)0);
}

void opengl_texture_destroy(ARDroneOpenGLTexture *texture)
{
    
}

#endif // USE_OPENGL

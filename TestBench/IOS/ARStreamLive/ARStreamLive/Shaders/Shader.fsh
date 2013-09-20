//
//  shader.fsh
//
//  Created by Frédéric D'HAEYER on 24/10/2011
//
varying mediump vec2 v_texcoord;

uniform sampler2D u_texture;

varying lowp vec4 v_color;
uniform bool u_useTexture;

void main()
{
    if (u_useTexture)
        gl_FragColor = texture2D(u_texture, v_texcoord);
    else
        gl_FragColor = v_color;
}

//
//  shader.vsh
//
//  Created by Frédéric D'HAEYER on 24/10/2011
//
//#pragma debug(on)
attribute vec4 a_position;
attribute vec2 a_texcoord;
attribute vec4 a_color;

uniform mat4 u_mvp;
uniform mat2 u_texscale;

varying mediump vec2 v_texcoord;

varying vec4 v_color;

void main()
{
    v_color = a_color; // 5
    gl_Position = u_mvp * a_position;
	v_texcoord = a_texcoord * u_texscale;
}

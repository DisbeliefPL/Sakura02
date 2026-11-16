#pragma once

//@里的内容由cmake自己替换
const char *VERTEX_SHADER_SRC = R"(
attribute vec4 a_Position;
attribute vec2 a_TexCoord;

varying vec2 v_TexCoord;

void main()
{
    gl_Position = a_Position;
    v_TexCoord = a_TexCoord;
}

)";
const char *FRAGMENT_SHADER_SRC = R"(
precision mediump float;

varying vec2 v_TexCoord;
uniform sampler2D u_Texture;

void main()
{
    gl_FragColor = texture2D(u_Texture, v_TexCoord);
}

)";

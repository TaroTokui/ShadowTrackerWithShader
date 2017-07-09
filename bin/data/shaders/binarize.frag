#version 120
uniform sampler2DRect tex0;
uniform float threshold;

void main()
{
    vec2 pos = gl_TexCoord[0].st;
    vec3 src = texture2DRect(tex0, pos).rgb;

    if(src.r < threshold)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }else{
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
}

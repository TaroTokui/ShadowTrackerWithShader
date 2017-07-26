#version 120
uniform sampler2DRect tex0;
uniform sampler2DRect bgTex;
uniform float threshold;

void main()
{
    vec2 pos = gl_TexCoord[0].st;
    vec3 src = texture2DRect(tex0, pos).rgb;
    vec3 bg = texture2DRect(bgTex, pos).rgb;

    float diff = bg.r - src.r;
    float ratio = diff / max(bg.r,0.01);
    if(ratio > threshold)
    {
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }else{
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    // gl_FragColor = vec4(ratio, 0.0, 0.0, 1.0);
}

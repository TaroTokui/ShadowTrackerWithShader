#version 120
uniform float increaseParam;
uniform float decreaseParam;
uniform sampler2DRect tex0;
uniform sampler2DRect preTex;

void main()
{
    vec2 pos = gl_TexCoord[0].st;
    vec3 src = texture2DRect(tex0, pos).rgb;
    vec3 prev = texture2DRect(preTex, pos).rgb;

    float val = 0.0;
    if(src.r > 0.5)
    {
        val = min(prev.r + increaseParam, 1.0);
    }else
    {
        val = max(prev.r - decreaseParam, 0.01);
    }

    gl_FragColor = vec4(val, val, val, 1.0);
}

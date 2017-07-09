#version 120
uniform sampler2DRect tex0;
uniform sampler2DRect preTex;

void main()
{
    vec2 pos = gl_TexCoord[0].st;
    vec3 src = texture2DRect(tex0, pos).rgb;
    vec3 prev = texture2DRect(preTex, pos).rgb;

    float v = max(src.r, prev.r);

    // gl_FragColor = vec4((src+prev)*0.5, 1.0);
    gl_FragColor = vec4(v, v, v, 1.0);
}

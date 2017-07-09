#version 120
uniform sampler2DRect tex0;
uniform int size;
uniform float sigma;

void main(void)
{
    vec2 pos = gl_TexCoord[0].st;

    int i, j, m, n;
    int s0 = (size-1) / 2;
    float weight;
    vec2 offset;
    vec3 col = vec3(0.0);

       
    float sum = 0.0;
    for(j = 0; j < size; j++)
    {
        n = -s0 + j;
        for(i = 0; i < size; i++)
        {
            m = -s0 + i;
            offset = vec2(i, j) ;
            weight = exp(-float(m*m + n*n) / (2.0 * sigma * sigma));
            sum += weight;
            col += texture2DRect(tex0, pos + offset).rgb * weight;            
        }
    }
    col /= sum;

    gl_FragColor = vec4(col, 1.0) ; 
}

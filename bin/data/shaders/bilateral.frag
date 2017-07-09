#version 120
uniform sampler2DRect tex0;

// Original shader by mrharicot
// https://www.shadertoy.com/view/4dfGDH

// Ported to Processing by Raphaël de Courville <twitter: @sableRaph>

#define SIGMA 10.0
#define BSIGMA 0.1
#define MSIZE 7

// uniform sampler2D texture;
uniform vec2 iResolution;

float normpdf(in float x, in float sigma)
{
    return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
    return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}


void main(void)
{
    vec2 pos = gl_TexCoord[0].st;
    vec3 c = texture2DRect( tex0, pos ).rgb;
        
    //declare stuff
    const int kSize = (MSIZE-1)/2;
    float kernel[MSIZE];
    vec3 final_colour = vec3(0.0);
    
    //create the 1-D kernel
    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j)
    {
        kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), SIGMA);
    }
    
    
    vec3 cc;
    float factor;
    float bZ = 1.0/normpdf(0.0, BSIGMA);
    vec2 p;
    //read out the texels
    for (int i=-kSize; i <= kSize; ++i)
    {
        for (int j=-kSize; j <= kSize; ++j)
        {
            p = pos + vec2(i,j);
            cc = texture2DRect(tex0, p ).rgb;
            factor = normpdf3(cc-c, BSIGMA)*bZ*kernel[kSize+j]*kernel[kSize+i];
            Z += factor;
            final_colour += factor*cc;

        }
    }
    
    gl_FragColor = vec4(final_colour/Z, 1.0);

}
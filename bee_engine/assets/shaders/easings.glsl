#extension GL_GOOGLE_include_directive : require

#include "constants.glsl"

float plot(vec2 uv,float pct)
{
    return smoothstep(pct - 0.01,pct, uv.y) - smoothstep(pct ,pct + 0.01,uv.y);
}

float lerp(float a, float b, float f) 
{
    return (a * (1.0 - f)) + (b * f);
}

// Sine
float easeInSine(float x)
{
    return 1.0 - cos((x * PI) / 2.0);
}

float easeOutSine(float x)
{
    return sin((x * PI) / 2.0);
}

float easeInOutSine(float x)
{ 
    return -(cos(PI * x) - 1.0) / 2.0;
}

// Quad
float easeInQuad(float x)
{
    return x * x;
}

float easeOutQuad(float x)
{
    return 1.0 - (1.0-x) * (1.0 -x );
}

float EaseInOutQuad(float x)
{
    //x < 0.5f ? 2 * x* x : 1 - pow(-2 * x + 2,2) /2;
    float inValue = 2.0 * x  *x;
    float outValue = 1.0- pow(-2.0 * x + 2.0,2.0) / 2.0;
    float inStep = step(inValue,0.5) * inValue;
    float outStep = step(0.5 , outValue ) * outValue;
   
    return inStep + outStep;
}

// Cubic
float easeInCubic(float x)
{
    return x * x * x;
}

float easeOutCubic(float x)
{ 
    return 1.0 - pow(1.0 - x,3.0);
}

float easeInOutCubic(float x)
{
    //x < 0.5f ? 4.0f * x * x * x : 1 - Mathf.Pow(-2 *x + 2,3)/2;
    float inValue = 4.0 * x * x * x;
    float outValue = 1.0 -pow(-2.0 * x + 2.0 ,3.0) /2.0;
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Quart
float easeInQuart(float x)
{
    return x * x * x * x;
}

float easeOutQuart(float x)
{
    return 1.0 - pow(1.0 -x, 4.0);
}

float easeInOutQuart(float x)
{
    //x < 0.5 ? 8 * x * x * x * x : 1 - pow(-2 * x + 2, 4) / 2;
    float inValue = 8.0 * x * x * x * x;
    float outValue = 1.0 - pow(-2.0 * x + 2.0, 4.0) / 2.0;
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Quint
float easeInQuint(float x)
{
    return x * x * x * x * x;
}

float easeOutQuint(float x)
{
    return 1.0 - pow(1.0 - x , 5.0);
}

float easeInOutQuint(float x)
{
    // x < 0.5 ? 16 * x * x * x * x * x : 1 - pow(-2 * x + 2, 5) / 2;
    float inValue = 16.0 * x * x * x * x * x;
    float outValue = 1.0 - pow(-2.0 * x + 2.0, 5.0) / 2.0;
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Expo
float easeInExpo(float x)
{
    return pow(2.0, 10.0 * x - 10.0);
}

float easeOutExpo(float x)
{  
    return 1.0 - pow(2.0, -10.0 * x);;
}

float easeInOutExpo(float x)
{
    float inValue = pow(2.0, 20.0 * x - 10.0) / 2.0;
    float outValue = (2.0 - pow(2.0, -20.0 * x + 10.0)) / 2.0;
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Circ
float easeInCirc(float x)
{
    return 1.0 - sqrt(1.0 - pow(x, 2.0));
}

float easeOutCirc(float x)
{
    return sqrt(1.0 - pow(x - 1.0, 2.0));
}

float easeInOutCirc(float x)
{
    //x < 0.5 ? (1 - sqrt(1 - pow(2 * x, 2))) / 2 : (sqrt(1 - pow(-2 * x + 2, 2)) + 1) / 2;
    float inValue = (1.0 - sqrt(1.0 - pow(2.0 * x, 2.0))) / 2.0 * step(x,0.5);
    float outValue = (sqrt(1.0 - pow(-2.0 * x + 2.0, 2.0)) + 1.0) / 2.0 * step(0.5,x);
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Back
const float easings_backC1 = 1.70158;
const float easings_backC2 = 2.5949095;
const float easings_backC3 = 2.70158;

float easeInBack(float x)
{
    return easings_backC3 * x * x * x - easings_backC1 * x * x;
}

float easeOutBack(float x)
{
    return  1.0 - easings_backC3 * pow(x - 1.0, 3.0) + easings_backC1 * pow(x - 1.0, 2.0);
}

float easeInOutBack(float x){
    //x < 0.5 ? (pow(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2: (pow(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
    float inValue = (pow(2.0 * x, 2.0) * ((easings_backC2 + 1.0) * 2.0 * x - easings_backC2)) / 2.0;
    float outValue = (pow(2.0 * x - 2.0, 2.0) * ((easings_backC2 + 1.0) * (x * 2.0 - 2.0) + easings_backC2) + 2.0) / 2.0;
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Elastic
float easeInElastic(float x)
{
    float c4 = (2.0 * PI) / 3.0;
    return -pow(2.0, 10.0 * x - 10.0) * sin((x * 10.0 - 10.75) * c4);
}

float easeOutElastic(float x)
{
    float c4 = (2.0 * PI) / 3.0;
    return pow(2.0, -10.0 * x) * sin((x * 10.0 - 0.75) * c4) + 1.0;;
}

float easeInOutElastic(float x)
{
    //x < 0.5 ? -(pow(2, 20 * x - 10) * sin((20 * x - 11.125) * c5)) / 2: (pow(2, -20 * x + 10) * sin((20 * x - 11.125) * c5)) / 2 + 1;
    float c5 = (2.0 * PI) / 4.5;
    float inValue = ( -(pow(2.0, 20.0 * x - 10.0) * sin((20.0 * x - 11.125) * c5)) / 2.0 )* step(x,0.5);
    float outValue = ((pow(2.0, -20.0 * x + 10.0) * sin((20.0 * x - 11.125) * c5)) / 2.0 + 1.0 )* step(0.5,x);
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

// Bounce
float easeOutBounce(float x)
{
    float n1 = 7.5625;
    float d1 = 2.75;
    float step1 = step(x,1.0/d1);
    float step2 = step(x, 2.0 /d1) - step1;
    float step3 = step(x, 2.5 / d1) - step2 - step1;
    float step4 = 1.0 - step3 - step2 - step1;
     
    float value = x;
    float p0 = (n1 * x * x)* step1 ;
    value = x - (1.5 / d1);
    float p1 = (n1 * value * value + 0.75) * step2 ;
    value = x - (2.25 / d1);
    float p2 = (n1 * value * value + 0.9375) * step3;
    value = x - (2.625  / d1);
    float p3 = (n1 * value * value + 0.984375) * step4;
    return p0 + p1 + p2 + p3;
}

float easeInBounce(float x)
{
    return 1.0 - easeOutBounce(1.0 - x);
}

float easeInOutBounce(float x)
{
    //x < 0.5 ? (1 - easeOutBounce(1 - 2 * x)) / 2: (1 + easeOutBounce(2 * x - 1)) / 2;
    float inValue = (1.0 - easeOutBounce(1.0 - 2.0 * x)) / 2.0 * step(x,0.5);
    float outValue =  (1.0 + easeOutBounce(2.0 * x - 1.0)) / 2.0 * step(0.5,x);
    return step(inValue , 0.5) * inValue + step(0.5,outValue) * outValue;
}

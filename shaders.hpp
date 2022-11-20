#include <stdlib.h>
#include <glatter/glatter.h>
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"out vec2 vPos_nobeat;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"   vPos_nobeat = vec2(aPos.x,aPos.y-0.3) * 1.2;\n"
"}\0";
const char* fragmentShaderSource =
"#version 330 core\n"
"const float draw_width = 0.3;\n"
"const float border_width = 0.2;\n"
"const float opacity_decay = 0.7;\n"
"bool between(float x, float lower_bound,float upper_bound){\n"
"    return x>=lower_bound && x<=upper_bound;"
"}\n"
"bool draw_on_pt_impl(vec2 pt){\n"
"    return (pt.y >= 0 && between(distance(pt, vec2(0.5, 0.0)), 0.5 - draw_width, 0.5)) ||"
"           (pt.y <= 0 && between(pt.y, pow(pt.x - 0.5, 3) + pt.x - 1.125, pow(pt.x - 0.5 + draw_width, 3) + pt.x - 1.125 + draw_width));"
"}\n"
"bool draw_on_pt(vec2 pt){\n"
"    return draw_on_pt_impl(vec2(abs(pt.x),pt.y)); \n"
"}\n"

"vec2 get_meetpoint(float k,float sign_x){\n"
"    float p = 1-sign_x*k;\n"
"    float q = -sign_x*0.625-k/2;\n"
"    float delta = (q*q/4.0) + (p*p*p/27.0);\n"
"    float x = 0.5*sign_x + pow(-q/2.0+sqrt(delta),1.0/3.0) +sign(-q/2.0-sqrt(delta))* pow(abs(-q/2.0-sqrt(delta)),1.0/3.0) ;\n"
"    float y = k * x;\n"
"    return vec2(x,y);"
"}\n"

"float heartbeat(float timestamp){\n"
"    return timestamp < 0.03 ? 1 - 10 * timestamp:\n"
"           timestamp < 0.13 ? mix(0.5, 1.3, (timestamp - 0.03) / 0.10):"
"           1.0 + 0.3 * pow((1.0 - timestamp) / (1 - 0.13), 2.0);"
"}\n"
"in vec2 vPos_nobeat;\n "
"out vec4 FragColor;\n"
"uniform float timestamp;\n"
"uniform bool transparent;\n"
"uniform bool use_standard;\n"
"vec2 vPos,tPoint,scale_center,region_center;\n"
"float rad, opacity, my_timestamp;\n"
"float dist,blockid;\n"
"void main()\n"
"{\n"
"    vPos = vPos_nobeat * 1.3 / heartbeat(timestamp);\n"
//"    vPos = vPos_nobeat;\n"
"    opacity = 0.0;\n"
"    for(float i=-0.5;i<1.0;i=i+1){\n"
"        rad = 0.01 + radians(round(degrees(atan(vPos.y,vPos.x-i)/10))*10);\n"
"        region_center = vec2((i + 0.5*cos(rad)),0.5*sin(rad));\n"
"        scale_center  = vec2((i + (0.5-border_width)*cos(rad)),(0.5-border_width)*sin(rad));\n"
"        my_timestamp = mod(timestamp + 100.0 * sin(30.0*region_center.x +20.0*region_center.y) + 0.7*i,1.0);\n"
"        tPoint = (vPos - mix(scale_center,region_center,my_timestamp)) * 25 / my_timestamp;\n"
"        opacity = (region_center.y >= -0.01 && draw_on_pt(tPoint))?(1.0-(1.0-opacity)*opacity_decay*my_timestamp):opacity;\n"

"        rad = 0.01 + radians(round(degrees(atan(vPos.y,vPos.x)/8))*8);\n"
"        region_center = get_meetpoint(tan(rad),-2*i);\n"
"        scale_center = mix(region_center,vec2(0.0,0.0),0.2/distance(region_center,vec2(0.0,0.0)));\n"
"        my_timestamp = mod(timestamp + 1000 * (region_center.x +region_center.y) + 0.7*i,1.0);\n"
"        tPoint = (vPos - mix(scale_center,region_center,my_timestamp)) * 25 / my_timestamp;\n"
"        opacity = (region_center.y <= 0.01 && draw_on_pt(tPoint))?(1.0-(1.0-opacity)*opacity_decay*my_timestamp):opacity;\n"
"    }\n"
"    blockid = mod(sin(round(100 * vPos.x) + 400 * round(100 * vPos.y) ) * 1000.0 + 107 * timestamp, 107.0);\n"
"    tPoint = vec2(abs(vPos.x),vPos.y);\n"
"    dist = abs(min(distance(vec2(0.0, 0.0), tPoint), tPoint.y >= 0 ? distance(vec2(0.5, 0.0), tPoint) - 0.5: distance(get_meetpoint(tPoint.y / (tPoint.x + 0.001), 1.0), tPoint)));\n"
"    opacity = use_standard ? ((blockid * pow(dist + 0.01,2.0) < 0.02) ? 1.0 : 0.0) : opacity;\n"
"    FragColor = vec4(opacity, 0.0, 0.0, transparent ? opacity : 1.0);\n"
"}\n\0";
inline GLuint compileShaderProgram() {

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status)
		exit(1);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		//char tmp[256];
		//GLsizei retsize;
		//glGetShaderInfoLog(fragmentShader, 256, &retsize, tmp);
		exit(1);
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (!status)
		exit(1);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glUseProgram(shaderProgram);

	return shaderProgram;
};
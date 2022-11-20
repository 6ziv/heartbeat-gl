#include <stdint.h>
#include <math.h>
#ifdef _WIN32
#include <Windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32

#define MY_CLOCKS_PER_SEC 1000
uint64_t my_clock(){return GetTickCount64();}
void my_sleep(uint64_t t){Sleep(t);}
#else
#include <time.h>
#include <unistd.h>
#define MY_CLOCKS_PER_SEC CLOCKS_PER_SEC
uint64_t my_clock(){return clock();}
void my_sleep(uint64_t t){
	usleep(t * 1000000 / (MY_CLOCKS_PER_SEC));
}
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <GLFW/glfw3native.h>
#include "midi.h"
#include "rtmidi/RtMidi.h"
#include <midifile/MidiFile.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <thread>
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"RtMidi.lib")
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

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
GLint transparentLocation;
GLint usestdLocation;
GLuint shaderProgram;
bool transparent = false;
void soundThreadProc() {
    std::string str(reinterpret_cast<const char*>(&bin2c_some_music_mid[0]), sizeof(bin2c_some_music_mid));
    std::istringstream iss(str, std::ios::binary);

    smf::MidiFile midifile;
    midifile.readSmf(iss);
    midifile.doTimeAnalysis();
    //midifile.linkNotePairs();
    //midifile.joinTracks();

    std::vector<std::pair<double, std::vector<uint8_t>>> messages;


    for (size_t i = 0; i < midifile.size(); i++) {
        const auto& track = midifile[i];
        for (size_t eventid = 0; eventid < track.size(); eventid++) {
            const auto& midi_ev = track[eventid];
            if (!midi_ev.isMeta()) {
                messages.emplace_back((MY_CLOCKS_PER_SEC) * midi_ev.seconds, midi_ev);
            }
        }
    }
    std::sort(messages.begin(), messages.end());

    RtMidiOut midiout;
    if (0 == midiout.getPortCount())return;
    midiout.openPort(0);
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    while (1) {
        unsigned long long begin_time = my_clock();
        size_t pos = 0;
        while (pos < messages.size()) {
            unsigned long long note_ts = my_clock() - begin_time;
            if (note_ts >= messages[pos].first) {
                midiout.sendMessage(messages[pos].second.data(), messages[pos].second.size());
                pos++;
            }
            else {
                if (messages[pos].first - note_ts > 20)my_sleep(messages[pos].first - note_ts - 3);
            }
        }
    }
#ifdef _WIN32
    timeEndPeriod(1);
#endif
    midiout.closePort();
}
int main()
{
    std::thread th(soundThreadProc);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 800, "LearnOpenGL", NULL, NULL);
    if (!window)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    if (GLEW_OK != glewInit())return 1;
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (!status)
        return 1;
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char tmp[256];
        GLsizei retsize;
        glGetShaderInfoLog(fragmentShader, 256, &retsize, tmp);
        std::cout << tmp << std::endl;
        return 1;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
    if (!status)
        return 1;
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLint timestampLocation = glGetUniformLocation(shaderProgram, "timestamp");
    transparentLocation = glGetUniformLocation(shaderProgram, "transparent");
    glUniform1i(transparentLocation, 0);
    usestdLocation = glGetUniformLocation(shaderProgram, "use_standard");
    glUniform1i(usestdLocation, 0);
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glUniform1f(timestampLocation, fmod(glfwGetTime(), 1.5) / 1.5);
        
        glClearColor(0.0,0.0,0.0,transparent?0.0f:1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    exit(0);
    return 0;
}
bool F12_pressed = false;
bool Tab_pressed = false;
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS) {
        if (!F12_pressed) {
            F12_pressed = true;
            
            glfwSetWindowAttrib(window, GLFW_DECORATED, transparent);
            transparent = !transparent;
#ifdef GLFW_MOUSE_PASSTHROUGH
            glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, transparent);
#else
	#ifdef _WIN32
            SetWindowLongA(glfwGetWin32Window(window), GWL_EXSTYLE, GetWindowLongA(glfwGetWin32Window(window), GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	#endif		
#endif // DEBUG

#ifdef _WIN32
            SetWindowDisplayAffinity(glfwGetWin32Window(window), WDA_MONITOR);
            SetWindowLongPtrA(glfwGetWin32Window(window), GWLP_HWNDPARENT, (LONG_PTR)(transparent ? GetDesktopWindow() : NULL));
#endif
			glfwSetWindowAttrib(window, GLFW_FLOATING, transparent);
            glUniform1i(transparentLocation, transparent);
            
    }
    }
    else {
        F12_pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!Tab_pressed) {
            Tab_pressed = true;
            GLint is_std;
            glGetUniformiv(shaderProgram, usestdLocation, &is_std);
            is_std = (is_std == 0);
            glUniform1i(usestdLocation, is_std);
        }
    }
    else {
        Tab_pressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    if (width > height)
        glViewport((width - height) / 2, 0, height, height);
    else
        glViewport(0, (height - width) / 2, width, width);
}




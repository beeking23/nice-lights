#include <GL/glew.h>
#include <stdio.h>
#include <cassert>
#include <cstddef>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <set>
#include <utility>
#include <functional>
#include <memory>
#include <list>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "icosahedron.hpp"
#include "vertexdesc.hpp"
#include "glhelpers.hpp"
#include "network.hpp"
#include "serial.hpp"

// #define PROFILE_FPS

// -----------------------------------------
// App framework
// -----------------------------------------

int WIN_WIDTH = 1280;
int WIN_HEIGHT = 1024;
#define WIN_FLAGS SDL_WINDOW_OPENGL

#define PACKED __attribute__((__packed__))

struct PACKED SerialControllerPacket {
	uint8_t m_magicByte;
	
	uint8_t m_seqNum : 6;
	uint8_t m_zero1 : 1;
	uint8_t m_changed : 1;
	
	uint8_t m_buttonA : 1;
	uint8_t m_buttonB : 1;
	uint8_t m_buttonC : 1;
	uint8_t m_buttonX : 5;	

	uint8_t m_switchA : 2;
	uint8_t m_switchB : 2;
	uint8_t m_switchC : 2;
	uint8_t m_zero2 : 2;

	uint8_t m_faderA;
	uint8_t m_faderB;
	uint8_t m_faderC;

	uint8_t m_potA;
	uint8_t m_potB;
	uint8_t m_potC;	

	uint8_t m_joyAxisX;
	uint8_t m_joyAxisY;		
};

static_assert(sizeof(SerialControllerPacket) == 12);


class NiceLightsApp {
public:
  static NiceLightsApp& Instance();
  
  bool createWindow();
  void drawGL();
  void initGL();
  void onKeyboardEvent(auto sym);
	void onMouseMotionEvent(int x, int y, int relx, int rely, int btn);
	void onMouseButtonEvent(int x, int y, int btn, int state);
	void onMouseWheelEvent(int delta);
  void mainLoop();
  void toggleFullscreen();
  void animateLights();
	void drawGUI();
	void handleSerial();

private:
  NiceLightsApp() :
    m_ctx(0),
    m_window(nullptr),
    m_quit(false),
    m_fullscreen(false),
    m_drawFrame(true),
    m_animation(0),
		m_edge(1),
		m_camDistance(2.0),
		m_camAltAz(0.0, 0.0)
  {
		
	}

  void initMesh();
  void initLightPoints();
    
  SDL_GLContext m_ctx;
  SDL_Window *m_window;
  bool m_quit;
  bool m_fullscreen;
  bool m_drawFrame;
  int m_animation;
	int m_edge;

	float m_camDistance;	

	glm::vec2 m_camAltAz;
	glm::ivec2 m_mouseMotionStart;
	glm::vec2 m_mouseMotionStartAltAz;	 
	
  GLuint m_ubMatrices;
  
  glm::mat4 m_projMat;
  glm::mat4 m_viewMat;
  
  size_t m_triCountMesh;
  GLuint m_vaoMesh;
  GLuint m_progMesh;

  size_t m_countLightsPoints;
  GLuint m_vaoLightPoints;
  GLuint m_progLightPoints;

  std::vector<icosahedron::LightPoint> m_lightCol;
  std::vector<icosahedron::LightPoint> m_lightPos;  
  GLuint m_lightColBuffer;

	NetworkMultiSender m_netMultiSender;	
	NetworkSender m_netSender;
	NetworkReceiver m_netReceiver;

	float m_animParams[6];
	float m_insideOutside = 0.5f;
	float m_insideOutsideAnimateSpeed = 0.0f;

	SerialIO m_serial;

	bool m_readSerialData = false;
	bool m_applyPeripheralData = false;
	
	struct PeripheralBankData {
		int m_switch = 0;
		int m_pot = 0;
	};
	int m_bankSelect = 0;
	PeripheralBankData m_bankData[3];
	float m_faders[3] = {0.0f, 0.0f, 0.0f};
	float m_joyAxisX = 0.0f;
	float m_joyAxisY = 0.0f;
	int m_serialSeqNum = 0;

	char m_serialDev[128] = "/dev/ttyACM0";
};

NiceLightsApp& NiceLightsApp::Instance()
{
  static NiceLightsApp *inst = new NiceLightsApp;
  return *inst;
}

bool NiceLightsApp::createWindow()
{
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
		
  m_window = SDL_CreateWindow("Nice Lights", 0, 0, WIN_WIDTH, WIN_HEIGHT, WIN_FLAGS);
  if(!m_window)
    return false;

  SDL_DisplayMode DM;
  SDL_GetDesktopDisplayMode(0, &DM);
  WIN_WIDTH = DM.w - 100;
  WIN_HEIGHT = DM.h - 100;

  SDL_SetWindowSize(m_window, WIN_WIDTH, WIN_HEIGHT);
  
  m_ctx = SDL_GL_CreateContext(m_window);

#ifdef PROFILE_FPS
  SDL_GL_SetSwapInterval(0);
#endif
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(m_window, m_ctx);
	ImGui_ImplOpenGL3_Init("#version 150");	

  return true;
}

// ------------------------------
// Mesh shader
// ------------------------------

const char *g_fragShaderMesh = R"_X_(
#version 450 core
in vec4 vs_col;
in vec4 vs_nrm;
out vec4 fs_col;

void main(void)
{
  float light = clamp(abs(vs_nrm.y), 0.0, 1.0) + 0.05;
  fs_col = vs_col * light;
}

)_X_";

const char *g_vertShaderMesh = R"_X_(
#version 450 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 nrm;
layout (location = 2) in vec3 col;

out vec4 vs_col;
out vec4 vs_nrm;

layout(std140, binding = 1) uniform MatrixBlock {
  mat4 projMat;
  mat4 modelViewMat;
  mat4 modelViewProjMat;
};

void main(void)
{
  gl_Position = modelViewProjMat * vec4(pos, 1.0); 
  vs_col = vec4(col, 1.0);
  vs_nrm = vec4(nrm, 0.0);
}

)_X_";

// ------------------------------
// LightPoints shader
// ------------------------------

const char *g_fragShaderLightPoints = R"_X_(
#version 420
in vec4 vs_col;
in vec4 vs_centre;
in vec4 vs_v;
out vec4 fs_col;

void main(void)
{
  float d = 1.0 - (distance(vs_v, vs_centre) / 0.035);
  fs_col = vs_col * d;
}

)_X_";

const char *g_vertShaderLightPoints = R"_X_(
#version 420
layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 col;

out vec4 vs_col;
out vec4 vs_centre;
out vec4 vs_v;

layout(std140, binding = 1) uniform MatrixBlock {
  mat4 projMat;
  mat4 modelViewMat;
  mat4 modelViewProjMat;
};

const vec4 offsets[6] = {
{-1, -1, 0, 0},
{ 1,  1, 0, 0},
{-1,  1, 0, 0},
{-1, -1, 0, 0},
{ 1, -1, 0, 0},
{ 1,  1, 0 ,0}
};

void main(void)
{
  int iVert = gl_VertexID % 6;

  gl_Position = modelViewMat * vec4(pos, 1.0);
  gl_Position += offsets[iVert] * 0.05;
  vs_v = gl_Position;
  gl_Position = projMat * gl_Position;

  vs_centre = modelViewMat * vec4(pos, 1.0);
  vs_col = vec4(col, 1.0);
}

)_X_";

void NiceLightsApp::initLightPoints()
{
  std::vector<ShaderDesc> shadersLightPoints = {
    {g_vertShaderLightPoints, GL_VERTEX_SHADER},
    {g_fragShaderLightPoints, GL_FRAGMENT_SHADER}
  };  
  m_progLightPoints = ShaderDesc::CreateShaderProgram(shadersLightPoints);

  icosahedron::MakeIcosahedronLightPoints(m_lightPos, m_lightCol);
  
  m_countLightsPoints = m_lightPos.size();
  printf("Light Point count %lu\n", m_countLightsPoints);
  
  const auto lpSize = sizeof(icosahedron::LightPoint);
  size_t vertexDataSize = m_countLightsPoints * lpSize;

  std::vector<VertexDesc> vertexDescArray;
  
  VertexDesc v;
  v.m_attrib = glGetAttribLocation(m_progLightPoints, "pos");
  v.m_offset = 0;
  v.m_type = GL_FLOAT;
  v.m_count = 3;
  v.m_divisor = 1;
  v.m_stride = lpSize;
  v.m_vertexData = &m_lightPos[0].px;
  v.m_vertexDataSize = vertexDataSize;
  vertexDescArray.push_back(v);

  v.m_attrib = glGetAttribLocation(m_progLightPoints, "col");
  v.m_vertexData = &m_lightCol[0].px;
  vertexDescArray.push_back(v);

  m_vaoLightPoints = VertexDesc::CreateArrayOfArraysVAO(vertexDescArray);

  m_lightColBuffer = vertexDescArray[1].m_bufferID;
  assert(m_lightColBuffer >= 0);

	m_netSender.initPackets(m_lightCol.size());
	m_netReceiver.init(m_lightCol.size());	
}

void NiceLightsApp::animateLights()
{
  float t = SDL_GetTicks() / 1000.0;

	if(m_applyPeripheralData) {
		m_animation = m_bankData[m_bankSelect].m_pot;		
		m_animParams[0] = m_joyAxisX;
		m_animParams[1] = m_joyAxisY;
		m_animParams[2] = m_faders[0];
		static const float switchVal[3] = {0.1f, 0.5f, 0.9f};
		m_animParams[3] = switchVal[m_bankData[m_bankSelect].m_switch];
		m_insideOutside = m_faders[1];
		m_insideOutsideAnimateSpeed = m_faders[2];
	}
	
	if(m_netReceiver.m_enabled)
		 m_netReceiver.update(&m_lightCol[0], m_lightCol.size());
	else {
		float insideMix = m_insideOutside;
		if(m_insideOutsideAnimateSpeed > 0.0) {
			float mixShift = insideMix - 0.5f;
			float insideOutMod = fabs(fmod(t * m_insideOutsideAnimateSpeed * 8.0, 2.0f) - 1.0f);
			insideMix = insideOutMod + mixShift;
			if(insideMix > 1.0f)
				insideMix = 1.0;
			if(insideMix < 0.0f)
				insideMix = 0.0f;
		}
		
		icosahedron::WhichSideCallback whichSide = [this](int n) -> int {
		  if(m_netMultiSender.m_enabled)
		    return m_netMultiSender.whichSide(n);
		  else
		    return (n / icosahedron::NUM_LEDS_PER_EDGE) & 1;
		};
		
		icosahedron::AnimateLightColours(m_animation,
						 m_lightPos,
						 m_lightCol,
						 t,
						 m_edge,
						 m_animParams,
						 sizeof(m_animParams)/sizeof(m_animParams[0]),
						 insideMix,
						 whichSide);
						 
	}
	
  glNamedBufferSubData(m_lightColBuffer, 0, sizeof(icosahedron::LightPoint) * m_lightCol.size(), &m_lightCol[0].px);
  GL_CHECK_ERROR();
}

void NiceLightsApp::initMesh()
{
  std::vector<ShaderDesc> shadersMesh = {
    {g_vertShaderMesh, GL_VERTEX_SHADER},
    {g_fragShaderMesh, GL_FRAGMENT_SHADER}
  };
  
  m_progMesh = ShaderDesc::CreateShaderProgram(shadersMesh);
  
  std::vector<icosahedron::Vertex> mesh;
  icosahedron::MakeIcosahedronPipesMesh(mesh);
  icosahedron::MakeFloorPlane(mesh, -1.0, 6, 0.5);
  
  m_triCountMesh = mesh.size();
  printf("Tri count %lu\n", m_triCountMesh);

  std::vector<VertexDesc> vertexDesc = {
    {.m_attrib = glGetAttribLocation(m_progMesh, "pos"), .m_offset = offsetof(icosahedron::Vertex, px), .m_type = GL_FLOAT, .m_count = 3},
    {.m_attrib = glGetAttribLocation(m_progMesh, "nrm"), .m_offset = offsetof(icosahedron::Vertex, nx), .m_type = GL_FLOAT, .m_count = 3},
    {.m_attrib = glGetAttribLocation(m_progMesh, "col"), .m_offset = offsetof(icosahedron::Vertex, cx), .m_type = GL_FLOAT, .m_count = 3}
  };

  const auto vSize = sizeof(icosahedron::Vertex);
  m_vaoMesh = VertexDesc::CreateInterleavedVAO(vertexDesc, &mesh[0].px, m_triCountMesh * vSize, vSize, 0);
}

void NiceLightsApp::initGL()
{
  auto err = glewInit();
  if(err != GLEW_OK) {
    printf("glewInit failed: %s\n", glewGetErrorString(err));
    assert(false);
  }

  m_projMat = glm::perspective(glm::pi<float>() / 3.0f, (float)WIN_WIDTH / (float)WIN_HEIGHT, 0.1f, 150.0f);
  m_viewMat = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -2.0));  
  
  glCreateBuffers(1, &m_ubMatrices);
  GL_CHECK_ERROR();
  glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_ubMatrices);
  GL_CHECK_ERROR();  
  glNamedBufferStorage(m_ubMatrices, sizeof(glm::mat4) * 3, NULL, GL_DYNAMIC_STORAGE_BIT);
  GL_CHECK_ERROR();  
  
  initMesh();
  initLightPoints();

  glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
  glClearColor(0.f, 0.f, 0.f, 0.f);  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glBlendFunc(GL_ONE, GL_ONE);  
}

void NiceLightsApp::drawGUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Icosahedron");

	ImGui::SeparatorText("Rendering");
	auto& io = ImGui::GetIO();
	ImGui::Text("%.1f FPS", io.Framerate);		
	ImGui::Checkbox("Draw Support Frame", &m_drawFrame);
	ImGui::SliderFloat("Camera Distance", &m_camDistance, 0.1f, 10.0f);
	
	ImGui::Combo("Style", &m_animation, icosahedron::g_animationNames, icosahedron::GetNumAnimationPatterns());
	ImGui::SliderInt("Highlight", &m_edge, 1, 30);
	
	ImGui::SeparatorText("Generic Animation");
	ImGui::SliderFloat("Anim Param 1", &m_animParams[0], 0.0, 1.0);
	ImGui::SliderFloat("Anim Param 2", &m_animParams[1], 0.0, 1.0);
	ImGui::SliderFloat("Anim Param 3", &m_animParams[2], 0.0, 1.0);
	ImGui::SliderFloat("Anim Param 4", &m_animParams[3], 0.0, 1.0);
	ImGui::SliderFloat("Inside/Outside Mix", &m_insideOutside, 0.0, 1.0);
	ImGui::SliderFloat("Inside/Outside Mod", &m_insideOutsideAnimateSpeed, 0.0, 1.0);	
	
	ImGui::SeparatorText("E131 Basic");				
	if(ImGui::Checkbox("Receive", &m_netReceiver.m_enabled))
		m_netReceiver.updateEnabled();

	ImGui::Checkbox("Receiver Dont Wait", &m_netReceiver.m_dontWaitForAllUniverses);
	
	ImGui::InputText("Destination", m_netSender.m_ipAddress, sizeof(m_netSender.m_ipAddress)-1);

	if(ImGui::Checkbox("Transit", &m_netSender.m_enabled))
		m_netSender.updateEnabled();

	if(ImGui::SliderInt("Framerate divisor", &m_netSender.m_divisor, 1, 30))
		m_netSender.updateDivisor();
	
	ImGui::SliderInt("Max Universe", &m_netSender.m_maxUniverse, 1, m_netSender.getNumUniverses());
	
	ImGui::SeparatorText("E131 Rig");				
	
	if(ImGui::Button("Read Mapping File")) {
		m_netMultiSender.readRangesFile("./src/edge-map.txt");
	}
	if(ImGui::Button("Read Local Mapping File")) {
		m_netMultiSender.readRangesFile("./src/edge-map-local.txt");
	}	

	if(ImGui::Checkbox("Transmit", &m_netMultiSender.m_enabled))
		m_netMultiSender.updateEnabled();

	ImGui::SliderInt("Framerate divisor", &m_netMultiSender.m_frameDivisor, 1, 30);
	ImGui::SliderFloat("Gamma", &GammaCorrection::g_gammaCorrection, 0.1, 5.0);
	ImGui::SliderInt("Packet Offset", &m_netMultiSender.m_packetStartOffset, 0, 4);		

	ImGui::End();

	ImGui::Begin("Peripheral");

	ImGui::SeparatorText("Serial");
	ImGui::InputText("Dev file", m_serialDev, sizeof(m_serialDev)-1);
	if(ImGui::Button("Open device")) {
		if(!m_serial.open(m_serialDev)) {
			m_readSerialData = false;
		}
	}
	
	ImGui::Checkbox("Read", &m_readSerialData);
	ImGui::Checkbox("Apply", &m_applyPeripheralData);
	ImGui::SeparatorText("Hardware");
	ImGui::Text("Bytes read: %i", m_serial.getBytesRead());
	ImGui::Text("Sequence number: %i", m_serialSeqNum);
	
	ImGui::SliderInt("Bank select", &m_bankSelect, 0, 2);
	for(int n = 0; n<3; n++) {
		static const char *bankName[3] = {"Bank A", "Bank B", "Bank C"};
		static const char *switchName[3] = {"3 way switch A (Anim 4)", "3 way switch B (Anim 4)", "3 way switch C (Anim 4)"};
		static const char *potName[3] = {"Pot A (pattern)", "Pot B (pattern)", "Pot C (pattern)"};		
		ImGui::SeparatorText(bankName[n]);		
		ImGui::SliderInt(switchName[n], &m_bankData[n].m_switch, 0, 2);
		ImGui::SliderInt(potName[n], &m_bankData[n].m_pot, 0, icosahedron::GetNumAnimationPatterns()-1);
	}
	
	ImGui::SeparatorText("Sliders");		
	ImGui::SliderFloat("Slider A (Anim 2)", &m_faders[0], 0.0f, 1.0f);
	ImGui::SliderFloat("Slider B (In/Out balance)", &m_faders[1], 0.0f, 1.0f);
	ImGui::SliderFloat("Slider C (in/Out mod)", &m_faders[2], 0.0f, 1.0f);	

	ImGui::SeparatorText("Joystick");			
	ImGui::SliderFloat("Joy Axis X", &m_joyAxisX, 0.0f, 1.0f);
	ImGui::SliderFloat("Joy Axis Y", &m_joyAxisY, 0.0f, 1.0f);	
	
	ImGui::End();
	
	ImGui::Render();
}

void NiceLightsApp::drawGL()
{
	drawGUI();
	
  float rotVal = float(SDL_GetTicks()) * 0.0001f;
	m_viewMat = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -m_camDistance)); 

  glm::mat4 rotMatAlt = glm::rotate(glm::mat4(1.0f), m_camAltAz.y, glm::vec3(1.0, 0.0, 0.0));
  glm::mat4 rotMatAz = glm::rotate(glm::mat4(1.0f), m_camAltAz.x, glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 camXform = m_viewMat * rotMatAlt * rotMatAz;
  glm::vec3 camPos =  glm::inverse(camXform) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  
  glm::mat4 mats[3];
  mats[0] = m_projMat;
  mats[1] = glm::lookAt(camPos, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
  mats[2] = m_projMat * mats[1];
  glNamedBufferSubData(m_ubMatrices, 0, sizeof(mats), mats);
  GL_CHECK_ERROR();

	animateLights();

	unsigned int ledCount = m_lightCol.size();
	const icosahedron::LightPoint *lightPtr = &m_lightCol[0];
	m_netSender.update(lightPtr, ledCount);
	m_netMultiSender.update(lightPtr, ledCount);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  if(m_drawFrame) {
    glUseProgram(m_progMesh);
    glBindVertexArray(m_vaoMesh);  
    glDrawArrays(GL_TRIANGLES, 0, m_triCountMesh);
  }

  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  
  glUseProgram(m_progLightPoints);
  glBindVertexArray(m_vaoLightPoints);
  // This isn't immediately obvious. The divisor is how for to advance the attribute array per instance.
  // Each instance here is 6 vertices and there are countLightPoints of instances to draw. After each
  // each instance of 6 vertices then the attribute array is advanced by one. So each element in the array
  // describes 1 point (instance) - the vertex shader uses the VertexID to make the point into a billboard.
  glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_countLightsPoints);
  
  glDepthMask(GL_TRUE);  
  glDisable(GL_BLEND);
	
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());  
  SDL_GL_SwapWindow(m_window);  
}

void NiceLightsApp::toggleFullscreen()
{
  m_fullscreen = !m_fullscreen;
  if(m_fullscreen)
    SDL_SetWindowFullscreen(m_window, WIN_FLAGS | SDL_WINDOW_FULLSCREEN_DESKTOP);
  else
    SDL_SetWindowFullscreen(m_window, WIN_FLAGS);
}

void NiceLightsApp::onMouseButtonEvent(int x, int y, int btn, int state)
{
	if(btn == SDL_BUTTON_LEFT && state == SDL_PRESSED) {
		m_mouseMotionStart = glm::ivec2(x, y);
		m_mouseMotionStartAltAz = glm::vec2(m_camAltAz);
	}
}

void NiceLightsApp::onMouseMotionEvent(int x, int y, int relx, int rely, int btn)
{
	const auto pi = glm::pi<float>();
	
	if(btn & SDL_BUTTON_LMASK) {
		glm::ivec2 delta = glm::ivec2(x, y) - m_mouseMotionStart;
		m_camAltAz.x = m_mouseMotionStartAltAz.x + delta.x * 0.01;
		m_camAltAz.y = m_mouseMotionStartAltAz.y + delta.y * 0.01;
	}

}

void NiceLightsApp::onMouseWheelEvent(int delta)
{
	m_camDistance += delta * -0.1f;
	if(m_camDistance < 0.1)
		m_camDistance = 0.1;
	if(m_camDistance > 10.0)
		m_camDistance = 10.0;
}

void NiceLightsApp::onKeyboardEvent(auto sym)
{
  switch(sym) {
  case SDLK_ESCAPE:
    m_quit = true;
    break;
  case 'f':
    toggleFullscreen();
    break;
  case 'p':
    m_drawFrame = !m_drawFrame;
    break;
  case 'a':
    m_animation++;
    if(m_animation > icosahedron::GetNumAnimationPatterns())
      m_animation = 0;
  default:
    break;
  }
}

void NiceLightsApp::mainLoop()
{
	ImGuiIO& io = ImGui::GetIO();	
  SDL_Event event;  
  while(!m_quit) {
		handleSerial();			
    while(SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if(event.type == SDL_MOUSEWHEEL) {
				if(!io.WantCaptureMouse)
					onMouseWheelEvent(event.wheel.y); 
			} else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				if(!io.WantCaptureMouse)
					 onMouseButtonEvent(event.button.x, event.button.y, event.button.button, event.button.state);
			} else if(event.type == SDL_MOUSEMOTION) {
				if(!io.WantCaptureMouse)
					 onMouseMotionEvent(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, event.motion.state);
      } else if(event.type == SDL_KEYDOWN)
				 if(!io.WantCaptureKeyboard)
						onKeyboardEvent(event.key.keysym.sym);
      else if(event.type == SDL_QUIT) {
        m_quit = true;
      }
    }

    drawGL();
    
#ifdef PROFILE_FPS    
    static float start = SDL_GetTicks();
    static int frames = 0;
    frames++;
    float now = SDL_GetTicks();
    float elapsed = now - start;
    if(elapsed > 1000) {
      fprintf(stderr, "%f\n", (frames * 1000.0) / elapsed);
      start = SDL_GetTicks();
      frames = 0;
    }
#endif
  }
}

void NiceLightsApp::handleSerial()
{
	if(!m_readSerialData)
		return;

	while(m_serial.read(1)) {
		const SerialControllerPacket& packet = *(reinterpret_cast<const SerialControllerPacket *>(m_serial.getDataBuf()));
		int packetLen = m_serial.getDataBufLen();
		
		// if the packet doesn't start with the magic sequence then discard it
		if(packet.m_magicByte != 0xff) {
			m_serial.clearDataBuf();
			continue;
		}
		
		// check for a keep alive packet, use these to show when the peripheral is working
		if(packetLen == 2) {
			m_serialSeqNum = packet.m_seqNum;
			if(packet.m_changed == 0) {
				// flush this packet ready for the next one.
				m_serial.clearDataBuf();
				continue;
			}
		}

		//printf("Read packet %i, len %i\n", packet.m_seqNum, packetLen);
		
		// only process an entire packet
		if(packetLen < 12)
			continue;

		// ok now do something with it!
		if(packet.m_buttonA)
			m_bankSelect = 0;
		else if(packet.m_buttonB)
			m_bankSelect = 1;
		else if(packet.m_buttonC)
			m_bankSelect = 2;
		
		m_bankData[0].m_switch = packet.m_switchA;
		m_bankData[1].m_switch = packet.m_switchB;
		m_bankData[2].m_switch = packet.m_switchC;
		
		static auto mapFader = [](const uint8_t value) -> float {
			const uint8_t faderRange = 254;
			return float(value) / float(faderRange);
		};
		
		static auto mapPot = [](const uint8_t value) -> int {
			float v = mapFader(value);
			return std::min(icosahedron::GetNumAnimationPatterns()-1, int(v * icosahedron::GetNumAnimationPatterns()));
		};
		
		m_bankData[0].m_pot = mapPot(packet.m_potA);
		m_bankData[1].m_pot = mapPot(packet.m_potB);
		m_bankData[2].m_pot = mapPot(packet.m_potC);
		
		m_faders[0] = mapFader(packet.m_faderA);
		m_faders[1] = mapFader(packet.m_faderB);
		m_faders[2] = mapFader(packet.m_faderC);
		
		m_joyAxisX = mapFader(packet.m_joyAxisX);
		m_joyAxisY = mapFader(packet.m_joyAxisY);
		
		m_serial.clearDataBuf();

		//break;
	}
}

int main (int ac, char **av)
{
  srand(100);
  
  auto& app = NiceLightsApp::Instance();
  if(!app.createWindow()) {
    printf("Failed to create SDL window\n");
    return -1;
  }

  app.initGL();
  app.mainLoop();
  
  return 0;
}

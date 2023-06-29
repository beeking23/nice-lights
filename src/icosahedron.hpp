#pragma once

namespace icosahedron {

const int NUM_LEDS_PER_EDGE = 42;
  
struct Vertex {
  float px, py, pz;
  float nx, ny, nz;  
  float cx, cy, cz;  

  void setPos(const glm::vec3& pos) {
    px = pos.x;
    py = pos.y;
    pz = pos.z;    
  }

  void setNormal(const glm::vec3& nrm) {
    nx = nrm.x;
    ny = nrm.y;
    nz = nrm.z;    
  }  
  
  void setColour(const glm::vec3& col) {
    cx = col.x;
    cy = col.y;
    cz = col.z;    
  }
};

struct LightPoint {
  float px, py, pz;  
  
  void setPos(const glm::vec3& pos) {
    px = pos.x;
    py = pos.y;
    pz = pos.z;
  }
};

void MakeIcosahedronPipesMesh(std::vector<Vertex>& verts);
void MakeIcosahedronLightPoints(std::vector<LightPoint>& lightPos, std::vector<LightPoint>& lightCol);
void MakeFloorPlane(std::vector<Vertex>& verts, float height, int res, float scale);

  typedef std::function<int(int)> WhichSideCallback;
  //typedef int (*WhichSideCallback)(int srcIdx);
  
void AnimateLightColours(int pattern,
			 const std::vector<LightPoint>& positions,
			 std::vector<LightPoint>& colours,
			 float time,
			 int opt,
			 float *params,
			 int nParams,
			 float insideOutsideMix,
			 WhichSideCallback whichSideCallback);

int GetNumAnimationPatterns();

extern const char* g_animationNames[];

}



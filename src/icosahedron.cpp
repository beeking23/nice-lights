#include <vector>
#include <set>
#include <functional>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/string_cast.hpp>

#include "icosahedron.hpp"

namespace icosahedron
{

glm::vec3 HSVtoRGB(float H, float s, float v)
{
  float C = s*v;
  float X = C * (1.0 - fabs(fmod(H/60.0f, 2.0f) - 1.0f));
  float m = v-C;
  float r,g,b;
	
  if(H >= 0 && H < 60) {
    r = C, g = X, b = 0;
  }
  else if(H >= 60 && H < 120) {
    r = X, g = C, b = 0;
  }
  else if(H >= 120 && H < 180) {
    r = 0, g = C, b = X;
  }
  else if(H >= 180 && H < 240) {
    r = 0, g = X, b = C;
  }
  else if(H >= 240 && H < 300) {
    r = X, g = 0, b = C;
  }
  else{
    r = C, g = 0, b = X;
  }
  return glm::vec3(r + m, g + m, b + m);
}

glm::vec3 RandomColour()
{
  float H = 360.0f * (float(rand()) / (float)RAND_MAX);
  return HSVtoRGB(H, 1.0, 0.7);
}

glm::vec3 RandomColour(float f)
{
  float H = 360.0f * f;
	if(H > 360.0f)
		 H -= 360.0f;
  return HSVtoRGB(H, 1.0, 0.7);
}

struct TriangleIndices {
	 int m_idx[3];
};

using TriangleList=std::vector<TriangleIndices>;
using VertexList=std::vector<glm::vec3>;

static const TriangleList triangles= {
	{0,4,1},  {0,9,4},  {9,5,4},  {4,5,8},  {4,8,1},
	{8,10,1}, {8,3,10}, {5,3,8},  {5,2,3},  {2,7,3},
	{7,10,3}, {7,6,10}, {7,11,6}, {11,0,6}, {0,1,6},
	{6,1,10}, {9,0,11}, {9,11,2}, {9,2,5},  {7,2,11}
};

const VertexList& GetIcosahedronVertices()
{
	static auto genVerts = []() {
		const float X=.525731112119133606f;
		const float Z=.850650808352039932f;
		const float N=0.f;

		VertexList vertices= {
			{-X,Z, N}, {X,Z, N}, {-X,-Z, N}, {X,-Z, N},
			{N,X, Z}, {N,-X, Z}, {N,X, -Z}, {N,-X, -Z},
			{Z,N, X}, {-Z,N, X}, {Z,N, -X}, {-Z, N, -X}
		};

		// the icosahedron should have its lowest face aligned with the XZ axis.
		float angle = 0.5f * (180.0f - 138.189f);
		glm::mat4 xform(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(1.0, 0.0, 0.0)));
		for(auto&& v : vertices)
			 v = xform * glm::vec4(v, 1.0);
		return vertices;
	};

	static const VertexList verts(genVerts());
	return verts;
}

void MakeIcosahedronMesh(std::vector<Vertex>& verts, const glm::mat4& xform) {
	auto& vertices = GetIcosahedronVertices();
	glm::vec3 colour = RandomColour();
	for(auto& t : triangles) {
		glm::vec3 p1 = vertices[t.m_idx[0]];
		glm::vec3 p2 = vertices[t.m_idx[1]];
		glm::vec3 p3 = vertices[t.m_idx[2]];
		glm::vec3 nrm = glm::normalize(glm::cross(p1 - p2, p1 - p3));
    
		Vertex v;
		v.setColour(colour);
		v.setNormal(nrm);
		v.setPos(xform * glm::vec4(p3, 1.0));
		verts.push_back(v);      
		v.setPos(xform * glm::vec4(p2, 1.0));
		verts.push_back(v);      
		v.setPos(xform * glm::vec4(p1, 1.0));
		verts.push_back(v);            
	}
}

void MakeFloorPlane(std::vector<Vertex>& verts, float height, int res, float scale) {
	auto c1 = glm::vec3(0.3, 0.3, 0.3);
	auto c2 = glm::vec3(0.2, 0.2, 0.2);

	auto cx = glm::vec3(0.6, 0.2, 0.2);
	auto cy = glm::vec3(0.2, 0.2, 0.6);		
	Vertex v;
	v.setNormal(glm::vec3(0.0, 1.0, 0.0));
	
	int r2 = (res/2);
	for(int y = -r2; y<r2; y++) {
		for(int x = -r2; x<r2; x++) {
			float xi = x * scale;
			float xj = (x+1) * scale;
			float yi = y * scale;
			float yj = (y+1) * scale;
			glm::vec3 pa(xi, height, yi);
			glm::vec3 pb(xj, height, yi);
			glm::vec3 pc(xj, height, yj);
			glm::vec3 pd(xi, height, yj);


			auto col = ((x & 1) ^ (y & 1)) ? c1 : c2;
			if(y == 0 || y == -1)
				col.y = 0.4f;
			if(x == 0 || x == -1)
				col.x = 0.4;
			v.setColour(col);
			
			v.setPos(pa);
			verts.push_back(v);
			v.setPos(pc);
			verts.push_back(v);
			v.setPos(pb);
			verts.push_back(v);

			v.setPos(pa);
			verts.push_back(v);
			v.setPos(pd);
			verts.push_back(v);
			v.setPos(pc);
			verts.push_back(v);									
		}
	}
}

void CreatePipeSection(std::vector<Vertex>& verts, const glm::vec3& start, const glm::vec3& end, float radius, int numCirclePoints, int numLinePoints, float colourFactor, bool endCaps) {
	const auto AddTri = [&verts](const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const auto& col) {
		auto nrm = glm::normalize(glm::cross(p1 - p2, p1 - p3));
      
		Vertex v;
		v.setColour(col);

		v.setNormal(nrm);
		v.setPos(p1);
		verts.push_back(v);
		v.setPos(p2);
		verts.push_back(v);
		v.setPos(p3);
		verts.push_back(v);
	};
    
	const auto lookat = [](const auto& eye, const auto& dir) {
		// 'up' is not quite exactly vertical so there isn't a chance of it exactly
		// aligning with one of the icosahedron edges
		const auto up = glm::vec3(0.0f, 0.1f, 0.9f);
		return glm::inverse(glm::lookAt(eye, eye + dir, up));
	};

	std::vector<glm::vec4> circlePoints;
	for(int i = 0; i<numCirclePoints; ++i) {
		float angle = (glm::pi<float>() * 2.0 * float(i)) / float(numCirclePoints);
		circlePoints.push_back(glm::vec4(sin(angle) * radius, cos(angle) * radius, 0.0, 1.0));
	}

	glm::vec4 tmp(0.0, 0.0, 0.0, 1.0);    
	glm::vec3 lineDir = end - start;
	float lineLen = glm::length(lineDir);    
	lineDir = glm::normalize(lineDir);

	auto colour = glm::vec3(1.0, 1.0, 1.0);
      
	if(endCaps) {
		glm::mat4 matStart = lookat(start, lineDir * -1.0f);
		glm::mat4 matEnd = lookat(end, lineDir);

		for(int i = 0; i<numCirclePoints; i++) {
			int next = (i + 1) % numCirclePoints;
    
			glm::vec4 qa = matStart * glm::vec4(0.0, 0.0, 0.0, 1.0);
			glm::vec4 qb = matStart * circlePoints[i];
			glm::vec4 qc = matStart * circlePoints[next];
			AddTri(qa, qb, qc, colour);
	
			glm::vec4 qa2 = matEnd * glm::vec4(0.0, 0.0, 0.0, 1.0);
			glm::vec4 qb2 = matEnd * circlePoints[i];
			glm::vec4 qc2 = matEnd * circlePoints[next];
			AddTri(qa2, qb2, qc2, colour);            
		}
	}

	for(int j = 0; j<numLinePoints; j++) {
		float posA = (float(j) * lineLen) / float(numLinePoints);
		float posB = (float(j+1) * lineLen) / float(numLinePoints);      
		glm::vec3 lStart = start + (lineDir * posA);
		glm::vec3 lEnd = start + (lineDir * posB);
      
		glm::mat4 matStart = lookat(lStart, lineDir);
		glm::mat4 matEnd = lookat(lEnd, lineDir);
		for(int i = 0; i<numCirclePoints; i++) {
			int next = (i + 1) % numCirclePoints;
			glm::vec4 qa = matStart * circlePoints[i];
			glm::vec4 qb = matEnd * circlePoints[i];
			glm::vec4 qc = matStart * circlePoints[next];
			glm::vec4 qd = matEnd * circlePoints[next];
	
			AddTri(qa, qd, qb, colour);
			AddTri(qa, qc, qd, colour);      
		}
	}
}

template<typename T> static bool ApproxEqual(const T& a, const T& b)
{
	const T Epsilon = 0.00001;	
	T diff = fabs(a - b);
	if(diff < Epsilon)
		 return true;
	else
		 return false;
}

bool operator<(const glm::vec3& a, const glm::vec3& b) {
	if(!ApproxEqual(a.y, b.y))
		 return a.y < b.y;
	else if(!ApproxEqual(a.x, b.x))
		 return a.x < b.x;
	else
		 return a.z < b.z;
}

glm::vec3 max(const glm::vec3& a, const glm::vec3& b) {
	return glm::vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

glm::vec3 min(const glm::vec3& a, const glm::vec3& b) {
	return glm::vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

template<typename T, typename Real> T lerp(const T a, const T b, Real t)
{
	return a + (b - a) * t;
}

bool ComparePolarLocation(const glm::vec3& va, const glm::vec3& vb)
{
	if(!ApproxEqual(va.y, vb.y))
		 return va.y < vb.y;

	auto det = (va.x * vb.z) - (va.z * vb.x);
	return (det > 0.0);
}

const std::vector<std::pair<int, int>> GetIcosahedronEdges()
{
	static auto genEdges = []() {
		const auto& vertices = GetIcosahedronVertices();		
		std::set<std::pair<int, int>> edges;
		const auto AddEdge = [&edges, &vertices](int a, int b) {
			if(a <= b)
				 edges.insert(std::make_pair(a, b));
			else
				 edges.insert(std::make_pair(b, a));
		};
    
		for(auto& t : triangles) {
			AddEdge(t.m_idx[0], t.m_idx[1]);
			AddEdge(t.m_idx[1], t.m_idx[2]);
			AddEdge(t.m_idx[2], t.m_idx[0]);      
		}

		assert(edges.size() == 30);

		std::vector<std::pair<int, int>> vedges;
		std::copy(edges.begin(), edges.end(), std::inserter(vedges, vedges.begin()));

		for(auto& e : vedges) {
			const auto& a = vertices[e.first];
			const auto& b = vertices[e.second];
			if(!ComparePolarLocation(a, b))
				 std::swap(e.first, e.second);
		}

		std::sort(vedges.begin(), vedges.end(), [&vertices](const auto& a, const auto& b) -> bool {
			auto a_max = max(vertices[a.first], vertices[a.second]);
			auto a_min = min(vertices[a.first], vertices[a.second]);
			auto va = lerp(a_min, a_max, 0.5f);

			auto b_max = max(vertices[b.first], vertices[b.second]);
			auto b_min = min(vertices[b.first], vertices[b.second]);
			auto vb = lerp(b_min, b_max, 0.5f);

			return ComparePolarLocation(va, vb);
		});
		return vedges;
	};
	static const auto& edges = genEdges();	
	return edges;
}

static const float PIPE_RADIUS = 0.01f;

void MakeIcosahedronPipesMesh(std::vector<Vertex>& verts)
{
	auto& vertices = GetIcosahedronVertices();	
	const auto& edges = GetIcosahedronEdges();
	const float step = 1.0f / float(edges.size());
	float colourFactor = 0.0f;
	for(const auto& e : edges) {
		auto pa = vertices[e.first];
		auto pb = vertices[e.second];
		const glm::vec3 delta = pa - pb;
		const float len = glm::length(delta);
		const float trim = len * 0.07f;
		const glm::vec3 nd = glm::normalize(delta);
		pa = pa - (nd * trim);
		pb = pb + (nd * trim);
		CreatePipeSection(verts, pa, pb, PIPE_RADIUS, 20, 10, colourFactor, true);
		colourFactor += step;
	}

	const float scale = PIPE_RADIUS * 3.0f;
	for(const auto& v : vertices) {      
		glm::mat4 xform = glm::translate(glm::mat4(1.0), v);
		xform = glm::scale(xform, glm::vec3(scale, scale, scale));				       
		MakeIcosahedronMesh(verts, xform);
	}
}

void MakeIcosahedronLightPoints(std::vector<LightPoint>& lightPos, std::vector<LightPoint>& lightCol)
{
	const auto edges = GetIcosahedronEdges();
	auto& vertices = GetIcosahedronVertices();
	
  const auto randPos = []() {
    float f = float(rand()) / float(RAND_MAX);
    f = (f * 2.0) - 1.0;
    return f;
  };

	const auto& addLightPoint = [&lightPos, &lightCol](const auto& pos, const auto& colour) {
		icosahedron::LightPoint pnt;
		pnt.setPos(pos);
		lightPos.push_back(pnt);
		pnt.setPos(colour);
		lightCol.push_back(pnt);
	};

	const float step = 1.0f / float(edges.size());
	float colourFactor = 0.0f;

	const int numLights = NUM_LEDS_PER_EDGE;
		
	for(const auto& e : edges)  {
		const auto col = RandomColour(colourFactor);
		colourFactor += step;
		
		auto pa = vertices[e.first];
		auto pb = vertices[e.second];
		const glm::vec3 delta = pb - pa;
		float len = glm::length(delta);
		const glm::vec3 nd = glm::normalize(delta);
		const float trim = len * 0.1f;
		
		pa = pa + (nd * trim);
		pb = pb - (nd * trim);		
		
		const glm::vec3 edgeNormal = glm::normalize(pa + (delta * 0.5f));
		len -= (trim * 2.0);

		/*
			Can either arrange the light strings with the lights along each side in a string (endToEnd)
			or so that each front and back pair are connected.
		*/		
		const bool endToEnd = true;

		if(!endToEnd) {
			for(int n = 0; n<numLights; n++) {
				float t = (float(n) * len)/float(numLights-1);
				addLightPoint(pa + (edgeNormal * PIPE_RADIUS) + (nd * t), col);
				addLightPoint(pa + (edgeNormal * -PIPE_RADIUS) + (nd * t), col);			
			}
		} else {
			for(int n = 0; n<numLights; n++) {
				float t = (float(n) * len)/float(numLights-1);
				addLightPoint(pa + (edgeNormal * PIPE_RADIUS) + (nd * t), col);
			}			
			for(int n = 0; n<numLights; n++) {
				float t = (float(n) * len)/float(numLights-1);
				addLightPoint(pa + (edgeNormal * -PIPE_RADIUS) + (nd * t), col);
			}
		}
	}
}

// -----------------------------------------------
// Patterns
// -----------------------------------------------

static void ClearColours(std::vector<LightPoint>& colours)
{
	for(auto& c : colours) {
		c.px = 0.0f;
		c.py = 0.0f;
		c.pz = 0.0f;
	}
}

static void FixedEdgesPattern(const std::vector<LightPoint>& position, std::vector<LightPoint>& colours, float t, int target)
{
	float step = fmod(t, 1.0f);
	float flash = 1.0f - ((fabs(step - 0.5f)) * 2.0f);
	int idx = target * NUM_LEDS_PER_EDGE * 2;
	int idxLed = int(step * NUM_LEDS_PER_EDGE);
	
	glm::vec3 colorLed = HSVtoRGB(step * 360.f, 0.7, 1.0);
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f) * flash * 0.3f;
	
	for(int n = 0; n<NUM_LEDS_PER_EDGE; n++) {
		if(n == idxLed) {
			colours[idx++].setPos(colorLed);
			colours[idx++].setPos(colorLed);
		} else {
			colours[idx++].setPos(color);
			colours[idx++].setPos(color);
		}
	}	
}

static void StepEdgesPattern(const std::vector<LightPoint>& position, std::vector<LightPoint>& colours, float t)
{
	const auto& edges = GetIcosahedronEdges();
	int target = int(fmod(t, float(edges.size())));
	FixedEdgesPattern(position, colours, t, target);
}

static void BasicPattern(const std::vector<LightPoint>& position, std::vector<LightPoint>& colours, float t)
{
	int n = 0;
	int target = int(fmod(t * 200.0, float(colours.size())));
	for(auto& c : colours) {
		++n;
		if(n == target)
			 c.setPos(glm::vec3(1, 1, 1));
		else
			 c.setPos(glm::vec3(0, 0, 0));
	}
	
	/*
	float step = 1.0 / float(colours.size());
	float offset = 0.0;
	
	for(auto& c : colours) {
		float h, s, v;
		float r = fmod(t + offset, 1.0);
		const float range = 0.1;
		if(r < range) {
			float j = r * (1.0 / range);
			h = j * 360.0;
			s = 0.5;
			v = fabs((j - 5.0) / 5.0);
		} else {
			h = 0.0;
			s = 0.0;
			v = 0.0;
		}
			
		c.setPos(HSVtoRGB(h, s, v));
		offset += step;
	}
	*/				 
}

typedef std::function<float(const glm::vec3&)> HueCallback;

static void BaseRingPattern(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, const glm::mat4& xform, HueCallback func)
{	
	float range = 0.2;
	for(int n = 0; n<positions.size(); n++) {
		glm::vec4 pos = glm::vec4(positions[n].px, positions[n].py, positions[n].pz, 1.0);
		pos = xform * pos; 
		auto& col = colours[n];

		float d = fabs(pos.x);
		if(d < range) {
			float v = 1.0 - (d / range);
			v *= 1.3;
			v = v* v * v * v;
			glm::vec3 c = HSVtoRGB(func(pos), 0.7, v);
			col.px += c.x;
			col.py += c.y;
			col.pz += c.z;			
		}
	}
}

static void RingPattern1(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time)
{
  glm::mat4 xform = glm::rotate(glm::mat4(1.0), time * 3.0f, glm::vec3(0.0, 1.0, 0.0));
  xform = glm::rotate(xform, time * 2.0f, glm::vec3(1.0, 0.0, 0.0));
  xform = glm::rotate(xform, time * 0.5f, glm::vec3(0.0, 0.0, 1.0));		
	BaseRingPattern(positions, colours, xform, [time](const glm::vec3& pos) { return fmod(time * 100.0, 360.0f); });
}

static void RingPatternManual(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time, float *params, int nParams)
{
  glm::mat4 xform = glm::rotate(glm::mat4(1.0), float(params[0] * M_PI), glm::vec3(0.0, 1.0, 0.0));
  xform = glm::rotate(xform, float(params[1] * M_PI), glm::vec3(1.0, 0.0, 0.0));
  xform = glm::rotate(xform, float(params[2] * M_PI), glm::vec3(0.0, 0.0, 1.0));		
	BaseRingPattern(positions, colours, xform, [params](const glm::vec3& pos) { return params[3] * 360.0; });
}

static void RingPattern2(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time)
{
  glm::mat4 xform = glm::rotate(glm::mat4(1.0), time * 2.0f, glm::vec3(0.0, 1.0, 0.0));

	BaseRingPattern(positions, colours, xform,
									[time](const glm::vec3& pos) {
										float h = ((atan2(pos.z, pos.y) / glm::pi<float>()) * 180.0) + (time * 100.0);
										h = fmod(h, 360.0);			
										return h;
									});
}

static void MultiRingPattern(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time)
{
	// because we are accumulating this value every frame then this number is senstive.
	const float sf = 0.11;

	const float fixedFactor = 0.01;
	static float rx = 0.0;
	rx += glm::perlin(glm::vec2(time * 0.1f, 0.0)) * 0.11;
	if(rx > 0.0)
		 rx += fixedFactor;
	else
		 rx -= fixedFactor;
			 
	
	static float ry = 0.0;
	ry += glm::perlin(glm::vec2(time * 0.1f, 10.0)) * 0.11;
	if(ry > 0.0)
		 ry += fixedFactor;
	else
		 ry -= fixedFactor;	

	static float rz = 0.0;
	rz += glm::perlin(glm::vec2(time * 0.1f, 20.0)) * 0.11;
	if(rz > 0.0)
		 rz += fixedFactor;
	else
		 rz -= fixedFactor;	
	
  glm::mat4 xformA = glm::rotate(glm::mat4(1.0), rx, glm::vec3(0.0, 0.0, 1.0));
  glm::mat4 xformB = glm::rotate(glm::mat4(1.0), ry, glm::vec3(0.0, 1.0, 0.0));
  glm::mat4 xformC = glm::rotate(glm::mat4(1.0), rz, glm::vec3(1.0, 0.0, 0.0));		
	
	BaseRingPattern(positions, colours, xformA, [time](const glm::vec3& pos) { return 1.0;/*fmod(time * 36.0, 360.0f);*/ });
	BaseRingPattern(positions, colours, xformA * xformB, [time](const glm::vec3& pos) { return 90.0;/*fmod(time * 36.0, 360.0f);*/ });
	BaseRingPattern(positions, colours, xformA * xformB * xformC, [time](const glm::vec3& pos) { return 180.0;/*fmod(time * 36.0, 360.0f);*/ });

}

static void SweepPattern(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time, float *params, int nParams)
{
	float h = fmod(time * 0.2, 1.0) * 360.0;
	
	const float sweep = 2.0;
	glm::mat4 xform = glm::translate(glm::mat4(1.0f), glm::vec3(fmod(time * 3.0, sweep * 2.0f) - sweep, 0, 0));

	xform = glm::rotate(xform, float(params[0] * M_PI), glm::vec3(0.0, 1.0, 0.0));
  xform = glm::rotate(xform, float(params[1] * M_PI), glm::vec3(1.0, 0.0, 0.0));
  xform = glm::rotate(xform, float(params[2] * M_PI), glm::vec3(0.0, 0.0, 1.0));
	
	float range = 0.4;
	for(int n = 0; n<positions.size(); n++) {
		glm::vec4 pos = glm::vec4(positions[n].px, positions[n].py, positions[n].pz, 1.0);
		pos = xform * pos;
		auto& col = colours[n];

		float t = fabs(pos.x);
		if(t < range) {
			float v = pow(1.0f - (t / range), 1.0f / params[3]);
			col.setPos(HSVtoRGB(h, v, v));
		}
	}
}

static void InsideOutPattern(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time, float *params, int nParams, WhichSideCallback whichSide)
{
	float mix = fabs(fmod(time * 3.0f * params[0], 2.0f) - 1.0f);
	float sideMix[2] = {(mix * 2.0f), (1.0f - mix) * 2.0f};	
	float h = fmod(time * 0.2, 1.0) * 360.0;
	
	for(int n = 0; n<positions.size(); n++) {
		auto& col = colours[n];			
		float v = 1.0;
		col.setPos(HSVtoRGB(h, v, v));		
		int side = whichSide(n);//(n / NUM_LEDS_PER_EDGE) & 1;
		const float sm = sideMix[side];
		col.px *= sm;
		col.py *= sm;
		col.pz *= sm;			
	}		
}

static void PlasmaPattern(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time, float *params, int nParams)
{
	for(int n = 0; n<positions.size(); n++) {
		auto& col = colours[n];
		auto& pos = positions[n];						
		float h = (glm::perlin(glm::vec3(pos.px * 10.0, pos.pz * 10.0, time * params[0] * 10.0f)) + 1.0f) * 0.5f;
		float v = params[1];
		col.setPos(HSVtoRGB(h * 360.f, params[1], params[2]));
	}
}

static void SpecklyPlasmaPattern(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time, float *params, int nParams)
{
	for(int n = 0; n<positions.size(); n++) {
		auto& col = colours[n];
		auto& pos = positions[n];						
		float h = (glm::perlin(glm::vec3(pos.px * 10.0, pos.pz * 10.0, time * params[0] * 10.0f)) + 1.0f) * 0.5f;
		float v = (glm::perlin(glm::vec3(pos.px * 10.0, pos.pz * 10.0, time * params[2] * 10.0f)) + 1.0f) * 0.5f;
		v = pow(v, 1.0f / params[3]);
		col.setPos(HSVtoRGB(h * 360.f, params[1], v));
	}
}

static void MixInsideOutside(const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float mix, WhichSideCallback whichSide)
{
	float sideMix[2] = {(mix * 2.0f), (1.0f - mix) * 2.0f};
	for(int n = 0; n<positions.size(); n++) {
	  int side = whichSide(n);//(n / NUM_LEDS_PER_EDGE) & 1;
		auto& col = colours[n];
		const float sm = sideMix[side];
		col.px *= sm;
		col.py *= sm;
		col.pz *= sm;			
	}
}

const char* g_animationNames[] = {
	"Moving Dot",
	"Moving Edges",
	"Highlight Edge",
	"Manual Control Ring",
	"Ring1",
	"Ring2",
	"Sweep",
	"Multi Ring",
	"Inside Out",
	"Plasma",
	"Speckly Plasma"
};

int GetNumAnimationPatterns()
{
	return 11;
}

void AnimateLightColours(int pattern, const std::vector<LightPoint>& positions, std::vector<LightPoint>& colours, float time, int arg, float *params, int nParams, float insideOutsideMix, WhichSideCallback whichSide)
{
	ClearColours(colours);
	switch(pattern) {
	case 0:
		BasicPattern(positions, colours, time);
		break;
	case 1:
		StepEdgesPattern(positions, colours, time);
		break;
	case 2:
		FixedEdgesPattern(positions, colours, time, arg-1);
		break;
	case 3:
		RingPatternManual(positions, colours, time, params, nParams);
		break;
	case 4:
		RingPattern1(positions, colours, time);
		break;
	case 5:
		RingPattern2(positions, colours, time);
		break;		
	case 6:
		SweepPattern(positions, colours, time, params, nParams);
		break;
	case 7:
		MultiRingPattern(positions, colours, time);
		break;
	case 8:
	  InsideOutPattern(positions, colours, time, params, nParams, whichSide);
		break;
	case 9:
		PlasmaPattern(positions, colours, time, params, nParams);
		break;
	case 10:
		SpecklyPlasmaPattern(positions, colours, time, params, nParams);
		break;		
	}

	MixInsideOutside(positions, colours, insideOutsideMix, whichSide);
	
}

}

#pragma once
#include <vector>
#include "vmath.h"
#include "pez.h"

enum AttributeSlot {
    SlotPosition,
    SlotTexCoord,
};

struct TexturePod {
    GLuint Handle;
    GLsizei Width;
    GLsizei Height;
};

struct SurfacePod {
    GLuint FboHandle;
    GLuint ColorTexture;
    GLsizei Width;
    GLsizei Height;
    GLsizei Depth;
};

struct SlabPod {
    SurfacePod Ping;
    SurfacePod Pong;
};

TexturePod LoadTexture(const char* path);
SurfacePod CreateSurface(int width, int height, int numComponents = 4);
SurfacePod CreateVolume(int width, int height, int depth, int numComponents = 4);
GLuint CreatePointVbo(float x, float y, float z);
GLuint CreateQuadVbo();
void CreateObstacles(SurfacePod dest);
SlabPod CreateSlab(GLsizei width, GLsizei height, GLsizei depth, int numComponents);
void InitSlabOps();
void SwapSurfaces(SlabPod* slab);
void ClearSurface(SurfacePod s, float v);
TexturePod OverlayText(std::string message);
void ExportScreenshot(const char* filename);

extern const int ViewportWidth;
extern const int ViewportHeight;

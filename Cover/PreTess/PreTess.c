#include "pez.h"
#include "vmath.h"
#include <malloc.h>
#include <stdio.h>
#include <pointcloud.h>
#include <AntTweakBar.h>

float TargetVector[] = {-0.31f,0.85f,0.02f};
float TargetScale = 5.0f;
float EyeVector[] = {-5, 1, 0};
float EyeScale = 2.13f;
float YScale = 2.31f;
float YOffset = 0.54f;
char ShowAO = 1;
char ComputeAO = 1;

float AO_MaxDist = 0.0f;
int AO_NumPoints = 1;

static int PositionSlot = 0;
static int NormalSlot = 1;
static int OcclusionSlot = 2;

struct FboDesc {
    GLuint FboHandle;
    GLuint DepthTexture;
    GLuint TextureHandle[3];
    enum { Positions, Normals, Colors };
};

struct SceneParameters {
    FboDesc Surfaces;
    GLuint StatueVao;
    GLuint FloorVao;
    GLuint QuadVao;
    GLuint FloorIndexCount;
    GLuint CompositeTexture;
    int StatueVertexCount;
    float Theta;
    GLuint OffscreenProgram;
    GLuint DisplayProgram;
    GLuint QuadProgram;
    Matrix4 Projection;
    Matrix4 ViewMatrix;
    PtcPointCloud PointCloud;
} Scene;

static TwBar* TweakBar = 0;
static const GLboolean DrawSkySphere = GL_TRUE;
static const GLboolean DrawOrnament = GL_TRUE;

static GLuint LoadProgram(const char* vsKey, const char* gsKey, const char* fsKey);
static GLuint CurrentProgram();

#define u(x) glGetUniformLocation(CurrentProgram(), x)
#define a(x) glGetAttribLocation(CurrentProgram(), x)
#define pf(x) ((float*) (&x))
#define pv(x) ((void*) (x))
#define nil ((const char*)(0))

PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = "PreTess";
    config.Width = 350 * 3 / 2;
    config.Height = 500 * 3 / 2;
    config.Multisampling = 1;
    config.VerticalSync = 1;
    return config;
}

static FboDesc CreateSurfaces(GLsizei width, GLsizei height, int numComponents, int numTargets)
{
    FboDesc surface;
    glGenFramebuffers(1, &surface.FboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, surface.FboHandle);

    for (int attachment = 0; attachment < numTargets; ++attachment) {

        GLuint textureHandle;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        surface.TextureHandle[attachment] = textureHandle;

        switch (numComponents) {
            case 1: glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0); break;
            case 2: glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, 0); break;
            case 3: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); break;
            case 4: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0); break;
            default: pezFatal("Illegal slab format.");
        }

        pezCheck(GL_NO_ERROR == glGetError(), "Unable to create FBO texture");

        GLuint colorbuffer;
        glGenRenderbuffers(1, &colorbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, GL_TEXTURE_2D, textureHandle, 0);
        pezCheck(GL_NO_ERROR == glGetError(), "Unable to attach color buffer");
    }
    
    // Create a depth texture:
    glGenTextures(1, &surface.DepthTexture);
    glBindTexture(GL_TEXTURE_2D, surface.DepthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);
    pezCheck(GL_NO_ERROR == glGetError(), "Unable to create depth texture");
    
    // Create a FBO and attach the depth texture:
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, surface.DepthTexture, 0);
    pezCheck(GL_NO_ERROR == glGetError(), "Unable to attach depth texture");

    pezCheck(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER), "Unable to create FBO.");

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return surface;
}

static void ReadVerts(const char* filename)
{
    FILE* infile = fopen(filename, "rb");
    pezCheck(infile != 0, "Can't read file: %s\n", filename);

    glGenVertexArrays(1, &Scene.StatueVao);
    glBindVertexArray(Scene.StatueVao);
    
    fread(&Scene.StatueVertexCount, 1, 4, infile);

    int vertexStride = sizeof(float) * 3;
    GLsizeiptr positionsTotalSize = Scene.StatueVertexCount * vertexStride;
    GLfloat* positionsBuffer = (GLfloat*) malloc(positionsTotalSize);

    GLsizeiptr normalsTotalSize = Scene.StatueVertexCount * vertexStride;
    GLfloat* normalsBuffer = (GLfloat*) malloc(normalsTotalSize);

    fread(positionsBuffer, 1, positionsTotalSize, infile);
    fread(normalsBuffer, 1, normalsTotalSize, infile);
    fclose(infile);

    GLuint positionsHandle;
    glGenBuffers(1, &positionsHandle);
    glBindBuffer(GL_ARRAY_BUFFER, positionsHandle);
    glBufferData(GL_ARRAY_BUFFER, positionsTotalSize, positionsBuffer, GL_STATIC_DRAW);

    GLuint normalsHandle;
    glGenBuffers(1, &normalsHandle);
    glBindBuffer(GL_ARRAY_BUFFER, normalsHandle);
    glBufferData(GL_ARRAY_BUFFER, normalsTotalSize, normalsBuffer, GL_STATIC_DRAW);
    
    // Load AO data from point cloud
    // https://renderman.pixar.com/forum/docs/RPS_16/index.php?url=ptcloudApi.php#intro
    char* ptcFile = "../Cover/PreTess/City.ptc";
    Scene.PointCloud = PtcSafeOpenPointCloudFile(ptcFile);
    pezCheck(Scene.PointCloud != 0, "Unable to open point cloud '%s'", ptcFile);
    GLsizeiptr occlusionsTotalSize = sizeof(float) * Scene.StatueVertexCount;
    float* occlusionsBuffer = (float*) malloc(occlusionsTotalSize);

    {
        float* pVertex = positionsBuffer;
        float* pNormal = normalsBuffer;
        float* pOcclusion = occlusionsBuffer;
        float normal[] = {0, 1, 0};
        for (int i = 0; i < Scene.StatueVertexCount; i++)
        {
            pNormal[0] *= -1; pNormal[1] *= -1; pNormal[2] *= -1;
            *pOcclusion = 0;
            int retval = PtcGetNearestPointsData(Scene.PointCloud, pVertex, pNormal, AO_MaxDist, AO_NumPoints, pOcclusion); 
            pVertex += 3;
            pNormal += 3;
            pOcclusion++;
        }
    }

    free(normalsBuffer);    
    free(positionsBuffer);

    // Create VBO for AO data
    GLuint occlusionsHandle; 
    glGenBuffers(1, &occlusionsHandle);
    glBindBuffer(GL_ARRAY_BUFFER, occlusionsHandle);
    glBufferData(GL_ARRAY_BUFFER, occlusionsTotalSize, occlusionsBuffer, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, positionsHandle);
    glEnableVertexAttribArray(PositionSlot);
    glVertexAttribPointer(PositionSlot, 3, GL_FLOAT, 0, vertexStride, nil);

    glBindBuffer(GL_ARRAY_BUFFER, normalsHandle);
    glEnableVertexAttribArray(NormalSlot);
    glVertexAttribPointer(NormalSlot, 3, GL_FLOAT, 0, vertexStride, nil);

    pezCheck(glGetError() == GL_NO_ERROR, "OpenGL error.\n");

    if (OcclusionSlot > -1) {
        glBindBuffer(GL_ARRAY_BUFFER, occlusionsHandle);
        glEnableVertexAttribArray(OcclusionSlot);
        glVertexAttribPointer(OcclusionSlot, 1, GL_FLOAT, 0, sizeof(float), nil);
    }

    pezCheck(glGetError() == GL_NO_ERROR, "OpenGL error.\n");

    // Create VAO for the floor

    float minX = -3.3f, maxX = +3.3f;
    float minY = -3.0f, maxY = +3.0f;
    float floorZ = 0;
    int floorRows = 512;
    int floorCols = 512;
    AO_MaxDist = 0.05f;
    AO_NumPoints = 1;

    int floorVertexCount = (floorRows + 1) * (floorCols + 1);
    GLsizeiptr interleavedTotalSize = sizeof(float) * 4 * floorVertexCount;
    float* interleavedBuffer = (float*) malloc(interleavedTotalSize);
    float dx = (maxX - minX) / floorRows;
    float dy = (maxY - minY) / floorCols;
    float* pBuffer = interleavedBuffer;
    int writtenVerts = 0;
    for (float x = minX; x < maxX + dx / 2; x += dx) {
        for (float y = minY; y < maxY + dy / 2; y += dy) {
            *(pBuffer + 0) = x;
            *(pBuffer + 1) = floorZ;
            *(pBuffer + 2) = y;
            float pNormal[] = {0, 1, 0};
            *(pBuffer + 3) = 0.5f;
            PtcGetNearestPointsData(Scene.PointCloud, pBuffer, pNormal, AO_MaxDist, AO_NumPoints, pBuffer + 3);
            pBuffer += 4;
            ++writtenVerts;
        }
    }
    pezCheck(writtenVerts == floorVertexCount, "Internal error.");

    glGenVertexArrays(1, &Scene.FloorVao);
    glBindVertexArray(Scene.FloorVao);

    GLuint interleavedHandle; 
    glGenBuffers(1, &interleavedHandle);
    glBindBuffer(GL_ARRAY_BUFFER, interleavedHandle);
    glBufferData(GL_ARRAY_BUFFER, interleavedTotalSize, interleavedBuffer, GL_STATIC_DRAW);

    vertexStride = 16;
    glEnableVertexAttribArray(PositionSlot);
    glVertexAttribPointer(PositionSlot, 3, GL_FLOAT, 0, vertexStride, nil);

    if (OcclusionSlot > -1) {
        glEnableVertexAttribArray(OcclusionSlot);
        glVertexAttribPointer(OcclusionSlot, 1, GL_FLOAT, 0, vertexStride, pv(12));
    }

    free(interleavedBuffer);

    Scene.FloorIndexCount = floorRows * floorCols * 3 * 2;
    int indicesTotalSize = sizeof(int) * Scene.FloorIndexCount;
    int* indicesBuffer = (int*) malloc(indicesTotalSize);

    int* pIndex = indicesBuffer;
    int n = 0;
    for (int i = 0; i < floorRows; ++i) {
        for (int j = 0; j < floorCols; ++j) {
            int A = n++;
            int B = A + 1;
            int C = A + floorCols + 1;
            int D = B + floorCols + 1;
            *pIndex++ = A; *pIndex++ = D; *pIndex++ = C;
            *pIndex++ = A; *pIndex++ = B; *pIndex++ = D;
        }
        n++;
    }

    GLuint indicesHandle; 
    glGenBuffers(1, &indicesHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesTotalSize, indicesBuffer, GL_STATIC_DRAW);

    free(indicesBuffer);
}

static GLuint CreateQuad()
{
    short positions[] = {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    
    // Create the VAO:
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create the VBO:
    GLuint vbo;
    GLsizeiptr size = sizeof(positions);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);

    // Set up the vertex layout:
    GLsizeiptr stride = 2 * sizeof(positions[0]);
    glEnableVertexAttribArray(PositionSlot);
    glVertexAttribPointer(PositionSlot, 2, GL_SHORT, GL_FALSE, stride, 0);

    return vao;
}

void LookupAoCallback(void* userData)
{
    ComputeAO = 1;
}

void PezInitialize()
{
    TweakBar = TwNewBar("TweakBar");
    TwDefine(" GLOBAL help='This example shows how to integrate AntTweakBar with GLUT and OpenGL.' "); // Message added to the help bar.
    TwDefine(" TweakBar size='200 400' color='96 216 224' "); // change default tweak bar size and color

    TwAddVarRW(TweakBar, "Theta", TW_TYPE_FLOAT, &Scene.Theta, " min=0.01 max=6.28 step=0.01 ");
    TwAddVarRW(TweakBar, "TargetVector", TW_TYPE_DIR3F, &TargetVector, " ");
    TwAddVarRW(TweakBar, "TargetScale", TW_TYPE_FLOAT, &TargetScale, " min=0.01 max=10.00 step=0.01 ");
    TwAddVarRW(TweakBar, "EyeVector", TW_TYPE_DIR3F, &EyeVector, " ");
    TwAddVarRW(TweakBar, "EyeScale", TW_TYPE_FLOAT, &EyeScale, " min=0.01 max=10.00 step=0.01 ");
    TwAddVarRW(TweakBar, "YScale", TW_TYPE_FLOAT, &YScale, " min=0.01 max=10.00 step=0.01 ");
    TwAddVarRW(TweakBar, "YOffset", TW_TYPE_FLOAT, &YOffset, " min=0.01 max=10.00 step=0.01 ");
    TwAddButton(TweakBar, "Lookup AO", LookupAoCallback, 0, "");
    TwAddVarRW(TweakBar, "Show AO", TW_TYPE_BOOL8, &ShowAO, "");

    PezConfig cfg = PezGetConfig();

    pezAddPath("../Cover/PreTess/", ".glsl");
    Scene.DisplayProgram = LoadProgram("PreTess.VS", 0, "PreTess.Display");
    Scene.OffscreenProgram = LoadProgram("PreTess.VS", 0, "PreTess.Offscreen");
    Scene.QuadProgram = LoadProgram("PreTess.Quad.VS", 0, "PreTess.Quad.FS");
    Scene.Theta = 2.36f;

    Scene.Surfaces = CreateSurfaces(cfg.Width, cfg.Height, 4, 3);
    GLenum err = glGetError();
    pezCheck(err == GL_NO_ERROR, "OpenGL error.\n");

    Scene.QuadVao = CreateQuad();

    ReadVerts("../Cover/PreTess/City.dat");
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 0);

    glGenTextures(1, &Scene.CompositeTexture);
    glBindTexture(GL_TEXTURE_2D, Scene.CompositeTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, cfg.Width, cfg.Height, GL_FALSE, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    err = glGetError();
    pezCheck(err == GL_NO_ERROR, "OpenGL error.\n");
}

void PezUpdate(float seconds)
{
    // Create the model-view matrix:
    Point3 eye = P3MakeFromElems(EyeVector[0] * EyeScale, EyeVector[1] * EyeScale, EyeVector[2] * EyeScale);
    Point3 target = P3MakeFromElems(TargetVector[0] * TargetScale, TargetVector[1] * TargetScale, TargetVector[2] * TargetScale);
    Vector3 up = V3MakeFromElems(0, 1, 0);
    Scene.ViewMatrix = M4MakeLookAt(eye, target, up);

    PezConfig cfg = PezGetConfig();
    const float h = 5.0f;
    const float w = h * cfg.Width / cfg.Height;
    const float hither = 5;
    const float yon = 200;
    Scene.Projection = M4MakeFrustum(-w, w, -h, h, hither, yon);
}

void SetUniforms()
{
    glUniformMatrix4fv(u("ViewMatrix"), 1, 0, pf(Scene.ViewMatrix));
    glUniformMatrix4fv(u("Projection"), 1, 0, pf(Scene.Projection));
    Matrix4 spinMatrix = M4MakeRotationY(Scene.Theta);
    Matrix4 scaleMatrix = M4MakeScale(V3MakeFromScalar(2.0f));
    Matrix4 modelMatrix = M4Mul(scaleMatrix, spinMatrix);
    Matrix4 modelview = M4Mul(Scene.ViewMatrix, modelMatrix);
    Matrix3 normalMatrix = M4GetUpper3x3(modelview);
    glUniformMatrix4fv(u("ModelMatrix"), 1, 0, pf(modelMatrix));
    glUniformMatrix4fv(u("Modelview"), 1, 0, pf(modelview));
    glUniformMatrix3fv(u("NormalMatrix"), 1, 0, pf(normalMatrix));
    glUniform1f(u("YScale"), YScale);
    glUniform1f(u("YOffset"), YOffset);
    glUniform4f(u("Color"), 1, 1, 1, 1);
}

void DrawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform4f(u("Color"), 1.3f, 1.3f, 1.3f, 1.0f);
    glBindVertexArray(Scene.StatueVao);
    glDrawArrays(GL_QUADS, 0, Scene.StatueVertexCount);
    glUniform4f(u("Color"), 0.0f/255.0f,165.0f/255.0f,211.0f/255.0f, 1.0f);
    glBindVertexArray(Scene.FloorVao);
    glVertexAttrib3f(NormalSlot, 0, -1, 0);
    glDrawElements(GL_TRIANGLES, Scene.FloorIndexCount, GL_UNSIGNED_INT, 0);
}

void PezRender()
{
    // Pass 1: draw into offscreen FBO
    {
        glUseProgram(Scene.OffscreenProgram);
        SetUniforms();
        glBindFramebuffer(GL_FRAMEBUFFER, Scene.Surfaces.FboHandle);
        GLenum renderTargets[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glDrawBuffers(3, &renderTargets[0]);
        DrawScene();
    }

    // Read the offscreen buffers into CPU memory
    if (ComputeAO) 
    {
        ComputeAO = 0;
        PezConfig cfg = PezGetConfig();

        static float* PositionData = (float*) malloc(sizeof(float) * 3 * cfg.Width * cfg.Height);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + FboDesc::Positions);
        glReadPixels(0, 0, cfg.Width, cfg.Height, GL_RGB, GL_FLOAT, PositionData);

        static float* NormalData = (float*) malloc(sizeof(float) * 3 * cfg.Width * cfg.Height);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + FboDesc::Normals);
        glReadPixels(0, 0, cfg.Width, cfg.Height, GL_RGB, GL_FLOAT, NormalData);

        static float* ColorData = (float*) malloc(sizeof(float) * 3 * cfg.Width * cfg.Height);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + FboDesc::Colors);
        glReadPixels(0, 0, cfg.Width, cfg.Height, GL_RGB, GL_FLOAT, ColorData);

        static float* CompositeData = (float*) malloc(sizeof(float) * 4 * cfg.Width * cfg.Height);

        float* pPosition = PositionData;
        float* pNormal = NormalData;
        float* pColor = ColorData;
        float* pComposite = CompositeData;
        for (int x = 0; x < cfg.Width; ++x) {
            for (int y = 0; y < cfg.Height; ++y) {
                
                AO_MaxDist = 0.025f;
                AO_NumPoints = 16;

                if (pColor[0] > 0.0f || pColor[1] > 0.0f || pColor[2] > 0.0f) {
                    float occlusion = 0.5f;
                    PtcGetNearestPointsData(Scene.PointCloud, pPosition, pNormal, AO_MaxDist, AO_NumPoints, &occlusion);
                    occlusion = 1.0f - occlusion;
                    pComposite[0] = pColor[0] * occlusion;
                    pComposite[1] = pColor[1] * occlusion;
                    pComposite[2] = pColor[2] * occlusion;
                    pComposite[3] = 1.0f;
                } else {
                    pComposite[0] = 0;
                    pComposite[1] = 0;
                    pComposite[2] = 0;
                    pComposite[3] = 0;
                }

                pPosition += 3;
                pNormal += 3;
                pColor += 3;
                pComposite += 4;
            }
        }
 
        glBindTexture(GL_TEXTURE_2D, Scene.CompositeTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, cfg.Width, cfg.Height, GL_FALSE, GL_RGBA, GL_FLOAT, CompositeData);

        pezCheck(glGetError() == GL_NO_ERROR, "OpenGL error.\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GLenum renderTargets[3] = {GL_BACK_LEFT, GL_NONE, GL_NONE};
    glDrawBuffers(3, &renderTargets[0]);

    // Draw into backbuffer
    if (ShowAO) {
        glBindTexture(GL_TEXTURE_2D, Scene.CompositeTexture);
        glUseProgram(Scene.QuadProgram);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(Scene.QuadVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        glUseProgram(Scene.DisplayProgram);
        SetUniforms();
        DrawScene();
    }

    pezCheck(glGetError() == GL_NO_ERROR, "OpenGL error.\n");
    glUseProgram(0);
    glBindVertexArray(0);
    TwDraw();
    glGetError();
}

static GLuint CurrentProgram()
{
    GLuint p;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &p);
    return p;
}

static GLuint LoadProgram(const char* vsKey, const char* gsKey, const char* fsKey)
{
    GLchar spew[256];
    GLint compileSuccess;
    GLuint programHandle = glCreateProgram();

    const char* vsSource = pezGetShader(vsKey);
    pezCheck(vsSource != 0, "Can't find vshader: %s\n", vsKey);
    GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsHandle, 1, &vsSource, 0);
    glCompileShader(vsHandle);
    glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(vsHandle, sizeof(spew), 0, spew);
    pezCheck(compileSuccess, "Can't compile vshader:\n%s", spew);
    glAttachShader(programHandle, vsHandle);

    if (gsKey) {
        const char* gsSource = pezGetShader(gsKey);
        pezCheck(gsSource != 0, "Can't find gshader: %s\n", gsKey);
        GLuint gsHandle = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gsHandle, 1, &gsSource, 0);
        glCompileShader(gsHandle);
        glGetShaderiv(gsHandle, GL_COMPILE_STATUS, &compileSuccess);
        glGetShaderInfoLog(gsHandle, sizeof(spew), 0, spew);
        pezCheck(compileSuccess, "Can't compile gshader:\n%s", spew);
        glAttachShader(programHandle, gsHandle);
    }

    if (fsKey) {
        const char* fsSource = pezGetShader(fsKey);
        pezCheck(fsSource != 0, "Can't find fshader: %s\n", fsKey);
        GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fsHandle, 1, &fsSource, 0);
        glCompileShader(fsHandle);
        glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess);
        glGetShaderInfoLog(fsHandle, sizeof(spew), 0, spew);
        pezCheck(compileSuccess, "Can't compile fshader:\n%s", spew);
        glAttachShader(programHandle, fsHandle);
    }

    glBindAttribLocation(programHandle, PositionSlot, "Position");
    glBindAttribLocation(programHandle, NormalSlot, "Normal");
    glBindAttribLocation(programHandle, OcclusionSlot, "Occlusion");

    glLinkProgram(programHandle);
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    glGetProgramInfoLog(programHandle, sizeof(spew), 0, spew);
    pezCheck(linkSuccess, "Can't link shaders:\n%s", spew);
    glUseProgram(programHandle);
    return programHandle;
}

#include "pez.h"
#include "vmath.h"
#include "teapot.h"
#include <stdio.h>

static void CreateTeapot();
static GLuint LoadEffect(const char* effect);
static GLuint CurrentProgram();

#define u(x) glGetUniformLocation(CurrentProgram(), x)
#define a(x) glGetAttribLocation(CurrentProgram(), x)

static GLuint SurfaceKnotsVao;
static GLsizei VertCount;
static GLsizei KnotCount;
static const GLuint PositionSlot = 0;
static const GLuint NormalSlot = 1;

static GLuint SurfaceProgram;
static GLuint HairProgram;

static Matrix4 ProjectionMatrix;
static Matrix4 ModelviewMatrix;
static Matrix3 NormalMatrix;
static Point3 CenterPoint;
static float TessInner = 8.0f;
static float TessOuter = 8.0f;

PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = "Teapot";
    config.Width = 960;
    config.Height = 540;
    config.Multisampling = 1;
    config.VerticalSync = 0;
    return config;
}

void PezRender()
{
    // Update dynamic uniforms:
    glUniformMatrix4fv(u("Modelview"), 1, 0, &ModelviewMatrix.col0.x);

    Matrix3 nm = M3Transpose(NormalMatrix);
    float packed[9] = { nm.col0.x, nm.col1.x, nm.col2.x,
                        nm.col0.y, nm.col1.y, nm.col2.y,
                        nm.col0.z, nm.col1.z, nm.col2.z };
    glUniformMatrix3fv(u("NormalMatrix"), 1, 0, &packed[0]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPatchParameteri(GL_PATCH_VERTICES, 16);

    float inner[] = { TessInner, TessInner };
    float outer[] = { TessOuter, TessOuter, TessOuter, TessOuter };
    glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, &inner[0]);
    glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, &outer[0]);

    glBindVertexArray(SurfaceKnotsVao);
    glDrawElements(GL_PATCHES, KnotCount, GL_UNSIGNED_SHORT, 0);
}

void PezInitialize()
{
    TessInner = 8;
    TessOuter = 8;

    pezAddPath("../Teapot/", ".glsl");
    CreateTeapot();
    SurfaceProgram = LoadEffect("Surface");
    //HairProgram = LoadEffect("Hair");

    glUseProgram(SurfaceProgram);
    //glUseProgram(HairProgram);

    // Set up the projection matrix:
    const float HalfWidth = 1.5f;
    const float HalfHeight = HalfWidth * PezGetConfig().Height / PezGetConfig().Width;
    ProjectionMatrix = M4MakeFrustum(-HalfWidth, +HalfWidth, -HalfHeight, +HalfHeight, 5, 1000);

    // Initialize various uniforms:
    glUniform3f(u("DiffuseMaterial"), 0, 0.75, 0.75);
    glUniform3f(u("AmbientMaterial"), 0.04f, 0.04f, 0.04f);
    glUniform3f(u("SpecularMaterial"), 0.5f, 0.5f, 0.5f);
    glUniform1f(u("Shininess"), 50);
    glUniformMatrix4fv(u("Projection"), 1, 0, &ProjectionMatrix.col0.x);

    Vector4 lightPosition = V4MakeFromElems(0.25, 0.25, 1, 0);
    glUniform3fv(u("LightPosition"), 1, &lightPosition.x);

    Matrix4 bezier = M4MakeFromCols(
        V4MakeFromElems(-1, 3, -3, 1),
        V4MakeFromElems(3, -6, 3, 0),
        V4MakeFromElems(-3, 3, 0, 0),
        V4MakeFromElems(1, 0, 0, 0) );
    glUniformMatrix4fv(u("B"), 1, GL_FALSE, &bezier.col0.x);
    glUniformMatrix4fv(u("BT"), 1, GL_TRUE, &bezier.col0.x);

    // Initialize various state:
    glEnable(GL_DEPTH_TEST);
    //glClearColor(0.7f, 0.6f, 0.5f, 1.0f);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

static void CreateTeapot()
{
    const GLsizei Vec3Stride = 3 * sizeof(float);
    const GLsizei IndexStride = sizeof(unsigned short);
    VertCount = sizeof(TeapotPositions) / Vec3Stride;
    KnotCount = sizeof(TeapotKnots) / IndexStride;

    // Create the VAO:
    glGenVertexArrays(1, &SurfaceKnotsVao);
    glBindVertexArray(SurfaceKnotsVao);

    // Find the axis-aligned bounding box:
    Point3 minBound = P3MakeFromElems(1000, 1000, 1000);
    Point3 maxBound = P3MakeFromElems(-1000, -1000, -1000);
    int vert;
    for (vert = 0; vert < VertCount; vert++) {
        float x = TeapotPositions[vert][0];
        float y = TeapotPositions[vert][1];
        float z = TeapotPositions[vert][2];
        Point3 p = P3MakeFromElems(x, y, z);
        minBound = P3MinPerElem(minBound, p);
        maxBound = P3MaxPerElem(maxBound, p);
    }
    CenterPoint = P3Scale(P3AddV3(maxBound, V3MakeFromP3(minBound)), 0.5f);
    pezPrintString("CenterPoint = (%f, %f, %f)\n", CenterPoint.x, CenterPoint.y, CenterPoint.z);

    // Create the VBO for positions:
    GLuint positions;
    {
        GLsizei totalSize = Vec3Stride * VertCount;
        glGenBuffers(1, &positions);
        glBindBuffer(GL_ARRAY_BUFFER, positions);
        glBufferData(GL_ARRAY_BUFFER, totalSize, TeapotPositions, GL_STATIC_DRAW);
    }

    // Create the normals VBO:
    GLuint normals;
    {
      GLsizei totalSize = Vec3Stride * VertCount;
      glGenBuffers(1, &normals);
      glBindBuffer(GL_ARRAY_BUFFER, normals);
      glBufferData(GL_ARRAY_BUFFER, totalSize, TeapotNormals, GL_STATIC_DRAW);
    }

    // Create the knots VBO:
    GLuint knots;
    {
      GLsizei totalSize = IndexStride * KnotCount;
      glGenBuffers(1, &knots);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, knots);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalSize, TeapotKnots, GL_STATIC_DRAW);
    }

    // Create the VAO
    glBindBuffer(GL_ARRAY_BUFFER, positions);
    glEnableVertexAttribArray(PositionSlot);
    glVertexAttribPointer(PositionSlot, 3, GL_FLOAT, GL_FALSE, Vec3Stride, 0);
    glEnableVertexAttribArray(NormalSlot);
    glBindBuffer(GL_ARRAY_BUFFER, normals);
    glVertexAttribPointer(NormalSlot, 3, GL_FLOAT, GL_FALSE, Vec3Stride, 0);
}

static GLuint getShader(const char* effect, const char* stage, GLenum e)
{
    GLchar key[64];
    sprintf(key, "%s.%s", effect, stage);
    const char* src = pezGetShader(key);
    pezCheck(src != 0, "Can't find shader %s.\n", key);

    GLuint handle = glCreateShader(e);
    glShaderSource(handle, 1, &src, 0);
    glCompileShader(handle);

    GLint compileSuccess;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &compileSuccess);

    GLchar compilerSpew[256];
    glGetShaderInfoLog(handle, sizeof(compilerSpew), 0, compilerSpew);
    pezCheck(compileSuccess, "%s Errors:\n%s", key, compilerSpew);

    return handle;
}

static GLuint LoadEffect(const char* effect)
{
    GLuint vsHandle = getShader(effect, "Vertex", GL_VERTEX_SHADER);
    GLuint tcsHandle = getShader(effect, "TessControl", GL_TESS_CONTROL_SHADER);
    GLuint tesHandle = getShader(effect, "TessEval", GL_TESS_EVALUATION_SHADER);
    GLuint gsHandle = getShader(effect, "Geometry", GL_GEOMETRY_SHADER);
    GLuint fsHandle = getShader(effect, "Fragment", GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vsHandle);
    glAttachShader(program, tcsHandle);
    glAttachShader(program, tesHandle);
    glAttachShader(program, gsHandle);
    glAttachShader(program, fsHandle);
    glBindAttribLocation(program, PositionSlot, "Position");
    glBindAttribLocation(program, NormalSlot, "Normal");
    glLinkProgram(program);

    GLint linkSuccess;
    GLchar linkerSpew[256];
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccess);
    glGetProgramInfoLog(program, sizeof(linkerSpew), 0, linkerSpew);
    pezCheck(linkSuccess, "Shader Link Errors:\n%s", linkerSpew);

    return program;
}

void PezUpdate(float seconds)
{
    const float RadiansPerSecond = 0.0000005f;
    static float Theta = 0;
    //Theta += seconds * RadiansPerSecond;
    
    Vector3 offset = V3MakeFromElems(CenterPoint.x, CenterPoint.y, CenterPoint.z);
    Matrix4 model = M4MakeRotationY(Theta);
    model = M4Mul(M4MakeTranslation(offset), model);
    model = M4Mul(model, M4MakeTranslation(V3Neg(offset)));

    Point3 eyePosition = P3MakeFromElems(CenterPoint.x, CenterPoint.y, -200);
    Point3 targetPosition = P3MakeFromElems(CenterPoint.x, CenterPoint.y, 0);
    Vector3 upVector = V3MakeFromElems(0, 1, 0);
    Matrix4 view = M4MakeLookAt(eyePosition, targetPosition, upVector);
    
    ModelviewMatrix = M4Mul(view, model);
    NormalMatrix = M4GetUpper3x3(ModelviewMatrix);
}

void PezHandleMouse(int x, int y, int action)
{
    if (action == PEZ_DOWN) {
        if (x < PezGetConfig().Width / 2) {
            if (TessInner > 1) {
                TessInner--;
                TessOuter--;
            }
        } else {
            TessInner++;
            TessOuter++;
        }
  }
}

GLuint CurrentProgram()
{
    GLuint p;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &p);
    return p;
}

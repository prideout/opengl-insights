
-- Vertex

in vec4 Position;
in vec3 Normal;
out vec3 vPosition;
out vec3 vNormal;

void main()
{
    vPosition = Position.xyz;
    vNormal = Normal;
}

-- TessControl.Inefficient

layout(vertices = 16) out;
in vec3 vPosition[];
in vec3 vNormal[];
out vec3 tcPosition[];
out vec3 tcNormal[];
patch out mat4 cx, cy, cz;
uniform mat4 B, BT;

#define ID gl_InvocationID

void main()
{
    tcPosition[ID] = vPosition[ID];
    tcNormal[ID] = vNormal[ID];

    mat4 Px, Py, Pz;
    for (int idx = 0; idx < 16; ++idx) {
        Px[idx/4][idx%4] = vPosition[idx].x;
        Py[idx/4][idx%4] = vPosition[idx].y;
        Pz[idx/4][idx%4] = vPosition[idx].z;
    }

    cx = B * Px * BT;
    cy = B * Py * BT;
    cz = B * Pz * BT;
}

-- TessControl

layout(vertices = 16) out;
in vec3 vPosition[];
in vec3 vNormal[];
out vec3 tcPosition[];
out vec3 tcNormal[];
patch out mat4 cx, cy, cz;
uniform mat4 B, BT;

#define ID gl_InvocationID

void main()
{
    tcPosition[ID] = vPosition[ID];
    tcNormal[ID] = vNormal[ID];
    if (ID > 2) return;

    mat4 P;
    for (int idx = 0; idx < 16; ++idx) {
      P[idx/4][idx%4] = vPosition[idx][ID];
    }

    P = B * P * BT;

    // Drivers have trouble with varying arrays, so...
    if (ID == 0) cx = P;
    if (ID == 1) cy = P;
    if (ID == 2) cz = P;

    // Drivers have trouble with tess level defaults, so...
    gl_TessLevelInner[0] = gl_TessLevelInner[1] = 8;
    gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = 8;
    gl_TessLevelOuter[2] = gl_TessLevelOuter[3] = 8;
}

-- TessEval

layout(quads) in;
in vec3 tcPosition[];
in vec3 tcNormal[];
patch in mat4 cx, cy, cz;
out vec3 teNormal;
out vec3 tePosition;
out vec4 tePatchDistance;
out vec3 teTessCoord;
uniform mat4 Projection;
uniform mat4 Modelview;

void main()
{
    teTessCoord = gl_TessCoord;

    float u = gl_TessCoord.x, v = gl_TessCoord.y;
    vec4 U = vec4(u*u*u, u*u, u, 1);
    vec4 V = vec4(v*v*v, v*v, v, 1);

    float x = dot(cx * V, U);
    float y = dot(cy * V, U);
    float z = dot(cz * V, U);
    tePosition =  vec3(x, y, z);

    const float E = 0.0;
    u = clamp(u, E, 1.0-E);
    v = clamp(v, E, 1.0-E);
    vec4 dU = vec4(3*u*u, 2*u, 1, 0);
    vec4 dV = vec4(3*v*v, 2*v, 1, 0);
    U = vec4(u*u*u, u*u, u, 1);
    V = vec4(v*v*v, v*v, v, 1);

    vec3 du;
    du.x = dot(cx * V, dU);
    du.y = dot(cy * V, dU);
    du.z = dot(cz * V, dU);

    vec3 dv;
    dv.x = dot(cx * dV, U);
    dv.y = dot(cy * dV, U);
    dv.z = dot(cz * dV, U);

    vec3 a = mix(tcNormal[0], tcNormal[3], u);
    vec3 b = mix(tcNormal[12], tcNormal[15], u);
    teNormal = mix(a, b, v);

    teNormal = normalize(cross(du, dv));

    tePatchDistance = vec4(u, v, 1-u, 1-v);

    gl_Position = Projection * Modelview * vec4(tePosition, 1);
}

-- Geometry

uniform mat4 Modelview;
uniform mat3 NormalMatrix;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 tePosition[3];
in vec4 tePatchDistance[3];
in vec3 teTessCoord[3];
in vec3 teNormal[3];
out vec3 gNormal;
out vec3 gFacetNormal;
out vec4 gPatchDistance;
out vec3 gTriDistance;
out vec3 gTessCoord;

void main()
{
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = NormalMatrix * normalize(cross(A, B));
    
    gPatchDistance = tePatchDistance[0];
    gTessCoord = teTessCoord[0];
    gTriDistance = vec3(1, 0, 0);
    gNormal = NormalMatrix * teNormal[0];
    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[1];
    gTessCoord = teTessCoord[1];
    gTriDistance = vec3(0, 1, 0);
    gNormal = NormalMatrix * teNormal[1];
    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[2];
    gTessCoord = teTessCoord[2];
    gTriDistance = vec3(0, 0, 1);
    gNormal = NormalMatrix * teNormal[2];
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}

-- Fragment

out vec4 FragColor;
in vec3 gFacetNormal;
in vec3 gTriDistance;
in vec3 gNormal;
in vec4 gPatchDistance;
in vec3 gTessCoord;

uniform vec3 LightPosition;
uniform vec3 DiffuseMaterial;
uniform vec3 AmbientMaterial;
uniform vec3 SpecularMaterial;
uniform float Shininess;

const vec3 InnerLineColor = vec3(1, 1, 1);
const bool DrawLines = false;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}

void main()
{
    vec3 N = normalize(gNormal);
    //vec3 N = normalize(gFacetNormal);  
    vec3 L = LightPosition;
    vec3 E = vec3(0, 0, 1);
    vec3 H = normalize(L + E);

    float df = abs(dot(N, L));
    float sf = abs(dot(N, H));
    sf = pow(sf, Shininess);
    vec3 color = AmbientMaterial + df * DiffuseMaterial + sf * SpecularMaterial;

    if (DrawLines) {
        float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
        float d2 = min(min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z), gPatchDistance.w);
        //float d3 = min(min(min(gCellDistance.x, gCellDistance.y), gCellDistance.z), gCellDistance.w);
        d1 = 1 - amplify(d1, 50, -0.5);
        d2 = amplify(d2, 50, -0.5);
        color = d2 * color + d1 * d2 * InnerLineColor;
    }

    // Useful for diagnosing orientation issues
//    color = vec3(4.0 * mod(gTessCoord.x, 0.125));

    FragColor = vec4(color, 1.0);
    //FragColor = vec4(gTessCoord, 1.0);
}

-- Loop's Domain Shader

float u[4], du[4];
float3 uB[4], duB[4];
CubicBezier(uv.x, u, du);
for (uint i = 0; i < 4; i++) {
     uB[i] =  float3(0, 0, 0);
    duB[i] =  float3(0, 0, 0);
    for (uint j = 0; j < 4; j++) {
        float3 A = B[4*i + j];
         uB[i] +=  u[j] * A;
        duB[i] += du[j] * A;
    }
}
float3 WorldPos  = float3(0, 0, 0);
float3 Tangent   = float3(0, 0, 0);
float3 BiTangent = float3(0, 0, 0);
CubicBezier(uv.y, u, du);
for (i = 0; i < 4; i++) {
    WorldPos  +=  uB[i] *  u[i];
    Tangent   += duB[i] *  u[i];
    BiTangent +=  uB[i] * du[i];
}

-- Loop's CubicBezier function

void CubicBezier(in float u, out float b[4], out float d[4])
{
    float t = u;
    float s = 1.0 - u;
   float a0 = s * s;
    float a1 = 2 * s * t;
    float a2 = t * t;
    b[0] =          s * a0;
    b[1] = t * a0 + s * a1;
    b[2] = t * a1 + s * a2;
    b[3] = t * a2;
    d[0] =    - a0;
    d[1] = a0 - a1;
    d[2] = a1 - a2;
    d[3] = a2;
}

-- VS

in vec4 Position;
in vec3 Normal;

out vec3 vPosition;
out vec3 vNormal;

in vec2 TexCoord;
out vec2 vTexCoord;

in float Occlusion;
out float vOcclusion;

void main()
{
    vTexCoord = TexCoord;
    vPosition = Position.xyz;
    vOcclusion = Occlusion;
    vNormal = Normal;
}

-- TessControl

layout(vertices = 4) out;

in vec3 vPosition[];
out vec3 tcPosition[];

in vec3 vNormal[];
out vec3 tcNormal[];

in vec2 vTexCoord[];
out vec2 tcTexCoord[];

in float vOcclusion[];
out float tcOcclusion[];

uniform float TessLevelInner;
uniform float TessLevelOuter;

#define ID gl_InvocationID

void main()
{
    tcPosition[ID] = vPosition[ID];
    tcNormal[ID] = vNormal[ID];
    tcTexCoord[ID] = vTexCoord[ID];
    tcOcclusion[ID] = vOcclusion[ID];

    if (ID == 0) {
        gl_TessLevelInner[0] = TessLevelInner;
        gl_TessLevelInner[1] = TessLevelInner;
        gl_TessLevelOuter[0] = TessLevelOuter;
        gl_TessLevelOuter[1] = TessLevelOuter;
        gl_TessLevelOuter[2] = TessLevelOuter;
        gl_TessLevelOuter[3] = TessLevelOuter;
    }
}

-- TessEval

layout(quads) in;

in vec3 tcPosition[];
out vec3 tePosition;

in vec3 tcNormal[];
out vec3 teNormal;

in vec2 tcTexCoord[];
out vec2 teTexCoord;

in float tcOcclusion[];
out float teOcclusion;
out vec4 tePatchDistance;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;
uniform mat3 NormalMatrix;
uniform float YScale, YOffset;

void main()
{
    float u = gl_TessCoord.x, v = gl_TessCoord.y;
    tePatchDistance = vec4(u, v, 1-u, 1-v);

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    tePosition = mix(a, b, v);

    a = mix(tcNormal[0], tcNormal[1], u);
    b = mix(tcNormal[3], tcNormal[2], u);
    teNormal = mix(a, b, v);

    float x = mix(tcOcclusion[0], tcOcclusion[1], u);
    float y = mix(tcOcclusion[3], tcOcclusion[2], u);
    teOcclusion = mix(x, y, v);

    vec2 s2 = mix(tcTexCoord[0], tcTexCoord[1], u);
    vec2 t2 = mix(tcTexCoord[3], tcTexCoord[2], u);
    teTexCoord = mix(s2, t2, v);

    gl_Position = Projection * Modelview * vec4(tePosition, 1);

    x = 2 * gl_Position.x / gl_Position.w;
    y = gl_Position.y / gl_Position.w;

    x = clamp(x, -1.0, 1.0);
    float s = acos(x) / (3.14 / 2.0) - 1;
    float t = y * YScale + YOffset;
    gl_Position.xy = vec2(s, t);
    gl_Position.z /= gl_Position.w;
    gl_Position.w = 1;
}

-- Display

in vec3 teNormal;
in float teOcclusion;
out vec4 FragColor;
uniform vec4 Color;

void main()
{
    FragColor = vec4(Color.rgb * vec3(1.0 - teOcclusion), 1.0);
}

-- Offscreen

in vec3 teNormal;
in vec3 tePosition;

uniform vec4 Color;

const int Positions = 0;
const int Normals = 1;
const int Colors = 2;
out vec3 FragData[3];

void main()
{
    FragData[Colors] = Color.rgb;
    FragData[Positions] = tePosition;
    FragData[Normals] = -normalize(teNormal);
}

-- Floor

in vec3 teNormal;
in vec3 tePosition;
in vec4 tePatchDistance;

uniform vec4 Color;

const int Positions = 0;
const int Normals = 1;
const int Colors = 2;
out vec3 FragData[3];

const vec3 LineColor = vec3(0, 0, 0);

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}
 
void main()
{
    if (true) {
        float d2 = min(min(min(tePatchDistance.x, tePatchDistance.y), tePatchDistance.z), tePatchDistance.w);
        d2 = amplify(d2, 50, -0.5);
        FragData[Colors] = d2 * Color.rgb;
    }
    
    FragData[Positions] = tePosition;
    FragData[Normals] = -normalize(teNormal);
}

-- Quad.VS

in vec4 Position;
out vec2 vTexCoord;

void main()
{
    gl_Position = Position;
    vTexCoord = 0.5 + Position.xy * 0.5;
}

-- Quad.FS

uniform sampler2D PostageStamp;

in vec2 vTexCoord;
out vec4 FragColor;

void main()
{
    FragColor = texture(PostageStamp, vTexCoord);
}
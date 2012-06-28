-- VS

in vec4 Position;
in vec3 Normal;

out vec3 vPosition;
out vec3 vNormal;

in vec2 TexCoord;
out vec2 vTexCoord;

in float Occlusion;
out float vOcclusion;

uniform float YScale, YOffset;
uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;
uniform mat3 NormalMatrix;

void main()
{
    vTexCoord = TexCoord;
    vPosition = Position.xyz;
    vOcclusion = Occlusion;
    vNormal = Normal;
    gl_Position = Projection * Modelview * Position;

    float x = 2 * gl_Position.x / gl_Position.w;
    float y = gl_Position.y / gl_Position.w;

    x = clamp(x, -1.0, 1.0);
    float s = acos(x) / (3.14 / 2.0) - 1;
    float t = y * YScale + YOffset;
    gl_Position.xy = vec2(s, t);
    gl_Position.z /= gl_Position.w;
    gl_Position.w = 1;
}

-- Display

in vec3 vNormal;
in float vOcclusion;
out vec4 FragColor;
uniform vec4 Color;

void main()
{
    FragColor = vec4(Color.rgb * vec3(1.0 - vOcclusion), 1.0);
}

-- Offscreen

in vec3 vNormal;
in vec3 vPosition;

uniform vec4 Color;

const int Positions = 0;
const int Normals = 1;
const int Colors = 2;
out vec3 FragData[3];

void main()
{
    FragData[Colors] = Color.rgb;
    FragData[Positions] = vPosition;
    FragData[Normals] = -normalize(vNormal);
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
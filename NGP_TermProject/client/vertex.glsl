#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec4 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
// [11.7] 메뉴에서만 텍스처의 선택한 부분을 사용한다.
uniform bool uiMode;
uniform vec4 uiUVRect;

void main()
{
    FragPos = vec4(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    if (uiMode) TexCoord = mix(uiUVRect.xy, uiUVRect.zw, aTexCoord);
    
    gl_Position = projection * view * vec4(FragPos);
}

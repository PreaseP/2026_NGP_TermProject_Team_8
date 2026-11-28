#version 330 core

in vec4 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 vColor;
uniform vec3 viewPos;
uniform bool lightingEnabled;
uniform bool useTexture;
uniform bool textureAlphaCutout; // [12.1] 방패 PNG의 투명한 부분을 건너뛴다.
uniform sampler2D texture1;
uniform float hitTint; // [11.25] 피격된 캐릭터에 섞을 붉은색의 세기
// [11.7] 메뉴에만 투명도와 색을 적용한다. 기존 게임 출력은 그대로 둔다.
uniform bool uiMode;
uniform vec4 uiColor;

void main ()
{
    if (uiMode) {
        FragColor = uiColor;
        if (useTexture) FragColor *= texture(texture1, TexCoord);
        return;
    }
	vec3 objectColor;
	
	if (useTexture) {
		vec4 texel = texture(texture1, TexCoord);
		if (textureAlphaCutout && texel.a < 0.5) discard;
		objectColor = texel.rgb;
	} else {
		objectColor = vColor;
	}

	if (!lightingEnabled)
	{
		FragColor = vec4(objectColor, 1.0f);
		return;
	}

	float ambientLight = 0.5f;
	vec3 ambient = ambientLight * lightColor;

	vec3 normalVector = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos.xyz);
	float diffuseLight = max(dot(normalVector, lightDir), 0.0);
	vec3 diffuse = diffuseLight * lightColor;

	int shininess = 128;
	vec3 viewDir = normalize(viewPos - FragPos.xyz);
	vec3 reflectDir = reflect(-lightDir, normalVector);
	float specularLight = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	vec3 specular = specularLight * lightColor;

	vec3 result = (ambient + diffuse + specular) * objectColor;
	if (hitTint > 0.0) {
		result = mix(clamp(result, 0.0, 1.0), vec3(1.0, 0.12, 0.09), hitTint);
	}

	FragColor = vec4(result, 1.0f);
}

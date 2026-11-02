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
uniform sampler2D texture1;

void main ()
{
	vec3 objectColor;
	
	if (useTexture) {
		objectColor = texture(texture1, TexCoord).rgb;
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

	FragColor = vec4(result, 1.0f);
}
#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include "stb_image.h"

struct Part {
	GLuint vao{ 0 };
	GLuint vbo{ 0 };
	GLuint nbo{ 0 };
	GLuint tbo{ 0 };
	GLsizei count{ 0 };
	glm::vec3 offset{ 0.0f };
	glm::vec3 pivot{ 0.0f };
};

struct UVRect {
	float u, v, w, h;
};

namespace Init {

	static GLuint loadTexture(const char* filepath)
	{
		GLuint textureID;
		glGenTextures(1, &textureID);

		stbi_set_flip_vertically_on_load(true);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(filepath, &width, &height, &nrComponents, 0);
		if (data)
		{
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << filepath << std::endl;
			stbi_image_free(data);
		}

		stbi_set_flip_vertically_on_load(false);
		return textureID;
	}

	static void uploadMesh(const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texCoords, GLuint& vao, GLuint& vbo, GLuint& nbo, GLuint& tbo, GLsizei& count)
	{
		if (vao == 0) glGenVertexArrays(1, &vao);
		if (vbo == 0) glGenBuffers(1, &vbo);
		if (nbo == 0) glGenBuffers(1, &nbo);
		if (tbo == 0) glGenBuffers(1, &tbo);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, nbo);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		count = static_cast<GLsizei>(positions.size());
	}

	static void makeCube(const glm::vec3& h, const UVRect faceUVs[6], const bool flips[6], float texWidth, float texHeight,
		std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& texCoords)
	{
		const glm::vec3 p[] = {
			glm::vec3(-h.x, -h.y, -h.z), glm::vec3(-h.x, -h.y, +h.z),
			glm::vec3(-h.x, +h.y, -h.z), glm::vec3(-h.x, +h.y, +h.z),
			glm::vec3(+h.x, -h.y, -h.z), glm::vec3(+h.x, -h.y, +h.z),
			glm::vec3(+h.x, +h.y, -h.z), glm::vec3(+h.x, +h.y, +h.z)
		};

		auto add_face = [&](int a, int b, int c, int d, const glm::vec3& norm, const UVRect& uvRect, bool flipU) {
			positions.push_back(p[a]); positions.push_back(p[b]); positions.push_back(p[c]);
			positions.push_back(p[c]); positions.push_back(p[d]); positions.push_back(p[a]);
			for (int i = 0; i < 6; ++i) normals.push_back(norm);

			float u_start = uvRect.u / texWidth;
			float u_end = (uvRect.u + uvRect.w) / texWidth;
			float flipped_v = texHeight - uvRect.v - uvRect.h;
			float v_start = flipped_v / texHeight;
			float v_end = (flipped_v + uvRect.h) / texHeight;
			float u0 = flipU ? u_end : u_start;
			float u1 = flipU ? u_start : u_end;

			texCoords.push_back(glm::vec2(u0, v_start));
			texCoords.push_back(glm::vec2(u1, v_start));
			texCoords.push_back(glm::vec2(u1, v_end));
			texCoords.push_back(glm::vec2(u1, v_end));
			texCoords.push_back(glm::vec2(u0, v_end));
			texCoords.push_back(glm::vec2(u0, v_start));
			};

		add_face(5, 4, 6, 7, glm::vec3(1, 0, 0), faceUVs[0], flips[0]);
		add_face(0, 1, 3, 2, glm::vec3(-1, 0, 0), faceUVs[1], flips[1]);
		add_face(3, 7, 6, 2, glm::vec3(0, 1, 0), faceUVs[2], flips[2]);
		add_face(0, 4, 5, 1, glm::vec3(0, -1, 0), faceUVs[3], flips[3]);
		add_face(1, 5, 7, 3, glm::vec3(0, 0, 1), faceUVs[4], flips[4]);
		add_face(4, 0, 2, 6, glm::vec3(0, 0, -1), faceUVs[5], flips[5]);
	}

	static Part makeCubePart(const glm::vec3& half, const glm::vec3& offset, const UVRect faceUVs[6], const bool flips[6], const glm::vec3& pivotLocal)
	{
		Part p;
		std::vector<glm::vec3> positions, normals;
		std::vector<glm::vec2> texCoords;
		const float texWidth = 64.0f, texHeight = 64.0f;
		makeCube(half, faceUVs, flips, texWidth, texHeight, positions, normals, texCoords);
		uploadMesh(positions, normals, texCoords, p.vao, p.vbo, p.nbo, p.tbo, p.count);
		p.offset = offset;
		p.pivot = pivotLocal;
		return p;
	}

	Part makeLineCubePart(float width, float height, float depth) {
		std::vector<glm::vec3> vertices;
		float w = width / 2.0f;
		float h = height / 2.0f;
		float d = depth / 2.0f;

		glm::vec3 p[8];
		p[0] = glm::vec3(-w, -h, -d);
		p[1] = glm::vec3(w, -h, -d);
		p[2] = glm::vec3(w, -h, d);
		p[3] = glm::vec3(-w, -h, d);
		p[4] = glm::vec3(-w, h, -d);
		p[5] = glm::vec3(w, h, -d);
		p[6] = glm::vec3(w, h, d);
		p[7] = glm::vec3(-w, h, d);

		// 아래쪽 4개 선
		vertices.push_back(p[0]); vertices.push_back(p[1]);
		vertices.push_back(p[1]); vertices.push_back(p[2]);
		vertices.push_back(p[2]); vertices.push_back(p[3]);
		vertices.push_back(p[3]); vertices.push_back(p[0]);

		// 위쪽 4개 선
		vertices.push_back(p[4]); vertices.push_back(p[5]);
		vertices.push_back(p[5]); vertices.push_back(p[6]);
		vertices.push_back(p[6]); vertices.push_back(p[7]);
		vertices.push_back(p[7]); vertices.push_back(p[4]);

		// 옆면 4개 선
		vertices.push_back(p[0]); vertices.push_back(p[4]);
		vertices.push_back(p[1]); vertices.push_back(p[5]);
		vertices.push_back(p[2]); vertices.push_back(p[6]);
		vertices.push_back(p[3]); vertices.push_back(p[7]);

		Part part;
		part.count = vertices.size();

		glGenVertexArrays(1, &part.vao);
		glBindVertexArray(part.vao);

		glGenBuffers(1, &part.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return part;
	}
}
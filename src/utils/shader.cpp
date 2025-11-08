#include "shader.h"

#include <fstream>
#include <sstream>



namespace df {
	void check_shader(const GLuint shader) noexcept {
		int success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (!success) {
			char infoLog[512];
			GLsizei length;
			glGetShaderInfoLog(shader, sizeof(infoLog), &length, infoLog);
			infoLog[--length] = '\0'; // Remove trailing newline
			fmt::println("[ GL COMPILE ERROR ]\t{}", infoLog);
			exit(1);
		}
	}


	std::optional<Shader> Shader::init(const assets::Shader asset) noexcept {
		Shader self;

		const std::string basePath = assets::getAssetPath(asset);
		std::ifstream vertexShaderFile{ basePath + ".vert.glsl" };
		std::ifstream fragmentShaderFile{ basePath + ".frag.glsl" };

		if (vertexShaderFile.bad()) {
			fmt::println("Failed to read shader file: {}", basePath + ".vert.glsl");
			return std::nullopt;
		}

		if (fragmentShaderFile.bad()) {
			fmt::println("Failed to read shader file: {}", basePath + ".frag.glsl");
			return std::nullopt;
		}

		std::stringstream vertexShaderStream, fragmentShaderStream;
		vertexShaderStream << vertexShaderFile.rdbuf();
		fragmentShaderStream << fragmentShaderFile.rdbuf();
		const std::string vertexShaderSource = vertexShaderStream.str();
		const std::string fragmentShaderSource = fragmentShaderStream.str();

		self.handle = glCreateProgram();
		const uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
		{
			const char* src = vertexShaderSource.c_str();
			glShaderSource(vertexShader, 1, &src, nullptr);
			glCompileShader(vertexShader);
			check_shader(vertexShader);
			glAttachShader(self.handle, vertexShader);
		}

		const uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		{
			const char* src = fragmentShaderSource.c_str();
			glShaderSource(fragmentShader, 1, &src, nullptr);
			glCompileShader(fragmentShader);
			check_shader(fragmentShader);
			glAttachShader(self.handle, fragmentShader);
		}

		glLinkProgram(self.handle);
		int success = 0;
		glGetProgramiv(self.handle, GL_LINK_STATUS, &success);

		if (!success) {
			char infoLog[512];
			GLsizei length;
			glGetProgramInfoLog(self.handle, sizeof(infoLog), &length, infoLog);
			infoLog[--length] = '\0'; // Remove trailing newline
			fmt::println("[ GL COMPILE ERROR ]\t{}", infoLog);
			exit(1);
		}

		glDetachShader(self.handle, vertexShader);
		glDeleteShader(vertexShader);
		glDetachShader(self.handle, fragmentShader);
		glDeleteShader(fragmentShader);

		return self;
	}
}

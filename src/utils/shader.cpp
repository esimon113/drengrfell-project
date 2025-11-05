#include "shader.h"

#include <fstream>
#include <sstream>



namespace df {
	void check_shader(const GLuint shader) noexcept {
		int success = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char info_log[512];
			GLsizei length;
			glGetShaderInfoLog(shader, sizeof(info_log), &length, info_log);
			info_log[--length] = '\0'; // Remove trailing newline
			fmt::println("[ GL COMPILE ERROR ]\t{}", info_log);
			exit(1);
		}
	}


	std::optional<Shader> Shader::init(const assets::Shader asset) noexcept {
		Shader self;

		const std::string base_path = assets::getAssetPath(asset);
		std::ifstream vertex_shader_file{ base_path + ".vert.glsl" };
		std::ifstream fragment_shader_file{ base_path + ".frag.glsl" };

		if (vertex_shader_file.bad()) {
			fmt::println("Failed to read shader file: {}", base_path + ".vert.glsl");
			return std::nullopt;
		}
		if (fragment_shader_file.bad()) {
			fmt::println("Failed to read shader file: {}", base_path + ".frag.glsl");
			return std::nullopt;
		}

		std::stringstream vertex_shader_stream, fragment_shader_stream;
		vertex_shader_stream << vertex_shader_file.rdbuf();
		fragment_shader_stream << fragment_shader_file.rdbuf();
		const std::string vertex_shader_source = vertex_shader_stream.str();
		const std::string fragment_shader_source = fragment_shader_stream.str();

		self.m_handle = glCreateProgram();
		const uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		{
			const char* src = vertex_shader_source.c_str();
			glShaderSource(vertex_shader, 1, &src, nullptr);
			glCompileShader(vertex_shader);
			check_shader(vertex_shader);
			glAttachShader(self.m_handle, vertex_shader);
		}
		const uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		{
			const char* src = fragment_shader_source.c_str();
			glShaderSource(fragment_shader, 1, &src, nullptr);
			glCompileShader(fragment_shader);
			check_shader(fragment_shader);
			glAttachShader(self.m_handle, fragment_shader);
		}

		glLinkProgram(self.m_handle);
		int success = 0;
		glGetProgramiv(self.m_handle, GL_LINK_STATUS, &success);
		if (!success)
		{
			char info_log[512];
			GLsizei length;
			glGetProgramInfoLog(self.m_handle, sizeof(info_log), &length, info_log);
			info_log[--length] = '\0'; // Remove trailing newline
			fmt::println("[ GL COMPILE ERROR ]\t{}", info_log);
			exit(1);
		}

		glDetachShader(self.m_handle, vertex_shader);
		glDeleteShader(vertex_shader);
		glDetachShader(self.m_handle, fragment_shader);
		glDeleteShader(fragment_shader);

		return self;
	}
}

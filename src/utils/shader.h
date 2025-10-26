#pragma once

#include <common.h>
#include "assets.h"
#include <glm/gtc/type_ptr.hpp>

class Shader {
	public:
		Shader() = default;
		~Shader() = default;

		static std::optional<Shader> init(const assets::Shader asset) noexcept;
		inline void deinit() const noexcept { glDeleteProgram(m_handle); }
		inline const Shader& use() const noexcept { glUseProgram(m_handle); return *this; }
		
		inline const Shader& setMat4(const char* name, const glm::mat4 matrix) const noexcept {
			glUniformMatrix4fv(glGetUniformLocation(m_handle, name), 1, GL_FALSE, glm::value_ptr(matrix));
			return *this;
		};
		inline const Shader& setVec3(const char* name, const glm::vec3 vector) const noexcept {
			glUniform3fv(glGetUniformLocation(m_handle, name), 1, glm::value_ptr(vector));
			return *this;
		}
		inline const Shader& setSampler(const char* name, const GLuint sampler) const noexcept {
			glUniform1i(glGetUniformLocation(m_handle, name), sampler);
			return *this;
		}
		inline const Shader& setInt(const char* name, const GLuint value) const noexcept {
			glUniform1i(glGetUniformLocation(m_handle, name), value);
			return *this;
		}
		inline const Shader& setFloat(const char* name, const float value) const noexcept {
			glUniform1f(glGetUniformLocation(m_handle, name), value);
			return *this;
		}

	private:
		uint32_t m_handle;
};

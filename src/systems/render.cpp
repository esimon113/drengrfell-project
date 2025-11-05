#include "render.h"



namespace df {
	RenderSystem RenderSystem::init(Window* window, Registry* registry) noexcept {
		RenderSystem self;

		self.m_window = window;
		self.m_registry = registry;

		self.m_viewport.m_origin = glm::uvec2(0);
		self.m_viewport.m_size = self.m_window->windowExtent();

		// load resources for rendering
		self.m_chicken_mesh = Mesh::init(assets::Mesh::chicken).value();

		self.m_chicken_shader = Shader::init(assets::Shader::chicken).value();
		self.m_sprite_shader = Shader::init(assets::Shader::sprite).value();
		self.m_wind_shader = Shader::init(assets::Shader::wind).value();
		self.m_egg_shader = Shader::init(assets::Shader::egg).value();

		self.m_eagle_texture = Texture::init(assets::Texture::eagle);
		self.m_bug_texture = Texture::init(assets::Texture::bug);

		glm::uvec2 extent = self.m_window->windowExtent();
		self.m_intermediate_framebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

		glGenVertexArrays(1, &self.m_quad_vao);
		glBindVertexArray(self.m_quad_vao);
		glGenBuffers(1, &self.m_quad_ebo);

		constexpr std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_quad_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

		glGenVertexArrays(1, &self.m_egg_vao);
		glBindVertexArray(self.m_egg_vao);
		glGenBuffers(1, &self.m_egg_vbo);
		// allocate an empty vertex attribute buffer here
		// it will be populated during the `renderStep` at run time
		glBindBuffer(GL_ARRAY_BUFFER, self.m_egg_vbo);
		// NOTE: We now set the usage parameter to GL_DYNAMIC_DRAW, since we intend to change the buffers contents frequently
		glBufferData(GL_ARRAY_BUFFER, sizeof(RenderSystem::Egg) * 128, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(RenderSystem::Egg), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(RenderSystem::Egg), (void*)sizeof(glm::vec2));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(RenderSystem::Egg), (void*)(sizeof(float) * 3));

		return self;
	}

	void RenderSystem::deinit() noexcept {
		m_chicken_mesh.deinit();

		m_chicken_shader.deinit();
		m_sprite_shader.deinit();
		m_wind_shader.deinit();
		m_egg_shader.deinit();

		m_eagle_texture.deinit();
		m_bug_texture.deinit();

	}

	void RenderSystem::step(const float) noexcept {
		m_intermediate_framebuffer.bind();
		glm::uvec2 extent = m_intermediate_framebuffer.extent();
		glViewport(0, 0, extent.x, extent.y);
		glClearColor(0.674f, 0.847f, 1.0f , 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_PROGRAM_POINT_SIZE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		// render scene

		glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 projection = glm::ortho(0.f, 1.f, 0.f, 2.f, 0.1f, 10.f);

		// draw player
		{
			Shader& chicken_shader = m_chicken_shader;
			Mesh& chicken_mesh = m_chicken_mesh;


			glm::vec2 position = m_registry->m_positions.get(m_registry->player());
			glm::vec2 scale = m_registry->m_scales.get(m_registry->player());

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(position, 0));
			model = glm::scale(model, glm::vec3(scale, 1));
			// TODO: (A2) Add rotation to the chain of transformations.
			//            Keep the effect the order of transformations has in mind.

			chicken_shader.use()
				.setMat4("model", model)
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setVec3("fcolor", {1, 1, 1});
			// TODO: (A2) Implement a `setBool` member function for the Shader class using `glUniform1i`.
			//            Use this function to set the `light_up` uniform to the appropriate value.
			chicken_mesh.bind();

			glDrawElements(GL_TRIANGLES, chicken_mesh.indexCount(), GL_UNSIGNED_INT, 0);
		}

		// draw sprites
		{
			glBindVertexArray(m_quad_vao);
			Shader& sprite_shader = m_sprite_shader;
			Texture& eagle_texture = m_eagle_texture;

			sprite_shader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setSampler("sprite", 0)
				.setVec3("fcolor", {1, 1, 1});
			eagle_texture.bind(0);
			GLsizei count = 0;
			for (Entity eagle : m_registry->m_eagles.entities) {
				glm::vec2 position = m_registry->m_positions.get(eagle);
				glm::vec2 scale = m_registry->m_scales.get(eagle);
				float angle = m_registry->m_angles.get(eagle);

				glm::mat4 model = glm::identity<glm::mat4>();
				model = glm::translate(model, glm::vec3(position, 0));
				model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 0, 1));
				model = glm::scale(model, glm::vec3(scale, 1));
				std::string name = fmt::format("model[{}]", count);
				sprite_shader.setMat4(name.c_str(), model);
				count++;
			}
			glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);

			Texture& bug_texture = m_bug_texture;
			bug_texture.bind(0);
			count = 0;
			for (Entity bug : m_registry->m_bugs.entities) {
				glm::vec2 position = m_registry->m_positions.get(bug);
				glm::vec2 scale = m_registry->m_scales.get(bug);
				float angle = m_registry->m_angles.get(bug);

				glm::mat4 model = glm::identity<glm::mat4>();
				model = glm::translate(model, glm::vec3(position, 0));
				model = glm::rotate(model, glm::radians(angle), glm::vec3(0, 0, 1));
				model = glm::scale(model, glm::vec3(scale, 1));
				std::string name = fmt::format("model[{}]", count);
				sprite_shader.setMat4(name.c_str(), model);
				count++;
			}
			glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, count);
		}

		// draw eggs
		{
			Shader& egg_shader = m_egg_shader;

			GLsizei count = static_cast<GLsizei>(m_registry->m_eggs.size());
			std::vector<RenderSystem::Egg> render_data;
			render_data.reserve(count);
			for (Entity egg : m_registry->m_eggs.entities) {
				render_data.emplace_back(
					m_registry->m_positions.get(egg),
					m_registry->m_collision_radius.get(egg),
					m_registry->m_colors.get(egg)
				);
			}
			glBindVertexArray(m_egg_vao);

			glBindBuffer(GL_ARRAY_BUFFER, m_egg_vbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(RenderSystem::Egg) * count, render_data.data());

			egg_shader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setInt("width", extent.x);

			glDrawArrays(GL_POINTS, 0, count);
		}

		m_intermediate_framebuffer.unbind();
		extent = m_window->windowExtent();
		glViewport(0, 0, extent.x, extent.y);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(m_viewport.m_origin.x, m_viewport.m_origin.y,
			m_viewport.m_size.x, m_viewport.m_size.y);
		// render full screen post-processing effect
		
		glBindVertexArray(m_quad_vao);
		Shader& wind_shader = m_wind_shader;
		wind_shader.use()
			.setSampler("screen_texture", 0)
			.setFloat("screen_darkness", m_registry->screenDarkness());
		m_intermediate_framebuffer.colorAttachments()->bind(0);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	void RenderSystem::reset() noexcept {}

	static constexpr float GAME_ASPECT_RATIO = 1.f/2; //< Aspect ratio of the in-game map.
	static std::pair<glm::uvec2, glm::uvec2> computeViewportConfig(const glm::uvec2 resolution) noexcept {
		float window_aspect_ratio = (float)resolution.x/resolution.y;
		if (window_aspect_ratio > GAME_ASPECT_RATIO) {
			glm::uvec2 viewport_size = { resolution.y * GAME_ASPECT_RATIO, resolution.y };
			uint32_t offset = (resolution.x - viewport_size.x) / 2;
			glm::uvec2 viewport_offset = { offset, 0 };

			return { viewport_offset, viewport_size };
		}

		glm::uvec2 viewport_size = { resolution.x, resolution.x * 1/GAME_ASPECT_RATIO };
		uint32_t offset = (resolution.y - viewport_size.y) / 2;
		glm::uvec2 viewport_offset = { 0, offset };

		return { viewport_offset, viewport_size };
	}

	void RenderSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
		auto [origin, size] = computeViewportConfig({width, height});

		m_viewport.m_origin = origin;
		m_viewport.m_size = size;

		// reinitialize off-screen framebuffer
		m_intermediate_framebuffer.deinit();
		m_intermediate_framebuffer = Framebuffer::init({ (GLsizei)size.x, (GLsizei)size.y, 1, true });
	}
}

#include "render.h"
#include <iostream>
#include "hero.h"
#include "../core/player.h"
#include "../core/tile.h"
#include "utils/textureArray.h"


namespace df {

    RenderSystem RenderSystem::init(Window* window, Registry* registry) noexcept {

        RenderSystem self;

        self.window = window;
        self.registry = registry;

        self.m_viewport.m_origin = glm::uvec2(0);
        self.m_viewport.m_size = self.window->getWindowExtent();

		// load resources for rendering
		self.spriteShader = Shader::init(assets::Shader::sprite).value();
		self.windShader = Shader::init(assets::Shader::wind).value();
        self.heroShader = Shader::init(assets::Shader::hero).value();
		self.tileShader = Shader::init(assets::Shader::tile).value();
		self.tileAtlas = TextureArray::init(assets::Texture::TILE_ATLAS, static_cast<int>(df::types::TileType::COUNT), 60, 59);

		self.buildingHoverShader = Shader::init(assets::Shader::buildingHover).value();
		self.buildingShadowShader = Shader::init(assets::Shader::buildingShadow).value();
		self.settlementTexture = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);
		self.roadPreviewTexture = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_UP);

		glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

        float quadVertices[] = {
            // positions    // texcoords
            0.0f, 0.0f,     0.0f, 0.0f,
            1.0f, 0.0f,     1.0f, 0.0f,
            1.0f, 1.0f,     1.0f, 1.0f,
            0.0f, 1.0f,     0.0f, 1.0f
        };
        constexpr GLuint quadIndices[] = { 0, 1, 2, 2, 3, 0 };

        glGenVertexArrays(1, &self.m_quad_vao);
        glBindVertexArray(self.m_quad_vao);

        GLuint quadVBO;
        glGenBuffers(1, &quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        GLuint quadEBO;
        glGenBuffers(1, &quadEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

        // Vertexattribs: pos (vec2), texcoord (vec2)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);

        // the texture buffer should be moved to another class, not in render::init but we leave it here for the
        // first milestone and change it later on when we have the logic to change between states (idling, swiming etc.)
        // maybe own class textureManager 
        std::vector<std::string> heroAttackTexturePaths = {
            "../../../assets/textures/hero/attack/attack_0.png",
            "../../../assets/textures/hero/attack/attack_1.png",
            //"../../../assets/textures/hero/attack/attack_2.png", weird animation
        };

        for (const auto& path : heroAttackTexturePaths) {
            Texture tex = Texture::init(path.c_str());
            self.heroAttackTextures.push_back(std::move(tex));
        }

        std::vector<std::string> heroSwimTexturePaths = {
            "../../../assets/textures/hero/swim/swim_0.png",
            "../../../assets/textures/hero/swim/swim_1.png",
            "../../../assets/textures/hero/swim/swim_2.png",
            "../../../assets/textures/hero/swim/swim_3.png",
            "../../../assets/textures/hero/swim/swim_4.png",
            "../../../assets/textures/hero/swim/swim_5.png",

        };

        for (const auto& path : heroSwimTexturePaths) {
            Texture tex = Texture::init(path.c_str());
            self.heroSwimTextures.push_back(std::move(tex));
        }
       
        std::vector<std::string> heroIdleTexturePaths = {
            "../../../assets/textures/hero/idle/idle_0.png",
            "../../../assets/textures/hero/idle/idle_1.png",
            "../../../assets/textures/hero/idle/idle_2.png",
        };

        for (const auto& path : heroIdleTexturePaths) {
            Texture tex = Texture::init(path.c_str());
            self.heroIdleTextures.push_back(std::move(tex));
        }

        std::vector<std::string> heroJumpTexturesPath = {
            "../../../assets/textures/hero/jump/jump_0.png",
            "../../../assets/textures/hero/jump/jump_1.png",
            "../../../assets/textures/hero/jump/jump_2.png",
            "../../../assets/textures/hero/jump/jump_3.png",
            "../../../assets/textures/hero/jump/jump_4.png",
            "../../../assets/textures/hero/jump/jump_5.png",
        };

        for (const auto& path : heroJumpTexturesPath) {
            Texture tex = Texture::init(path.c_str());
            self.heroJumpTextures.push_back(std::move(tex));
        }

        // Hero entity
        // shouldnt be initialized here, but we currently dont use world.cpp so i leave it here for 
        // milestone 2 and move it to another class when our project is more strucuted
        Entity hero;
        registry->positions.emplace(hero, glm::vec2(0.5f, 0.5f));
        registry->scales.emplace(hero, glm::vec2(0.1f, 0.1f));
        registry->collisionRadius.emplace(hero, 0.5f);

        std::vector<int> animationOrder = { 0,1,2,1 }; // Sequenz 0-1-2-1 for idle
        Animation anim(animationOrder, 0.65f, true);
        registry->animations.emplace(hero, AnimationComponent{ anim });

        
		constexpr ::std::array<GLuint, 6> indices = { 0, 1, 2, 2, 3, 0 };
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.m_quad_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		self.initMap();

		return self;
	}


	void RenderSystem::deinit() noexcept {
		spriteShader.deinit();
		windShader.deinit();
        heroShader.deinit();
		tileShader.deinit();
		tileAtlas.deinit();

		buildingHoverShader.deinit();
		buildingShadowShader.deinit();
		settlementTexture.deinit();
		roadPreviewTexture.deinit();
	}


    Texture& RenderSystem::getCurrentTexture(AnimationComponent& animComp, int frameIndex) {
        switch (animComp.currentType) {
        case Hero::AnimationType::Idle:
            return heroIdleTextures[frameIndex];
        case Hero::AnimationType::Jump:
            return heroJumpTextures[frameIndex];
        case Hero::AnimationType::Swim:
            return heroSwimTextures[frameIndex];
        case Hero::AnimationType::Attack:
            return heroAttackTextures[frameIndex];
        default:
            return heroIdleTextures[frameIndex];
        }
    }   

    const std::vector<int>& getHeroAnimationSequence(Hero::AnimationType type) {
        switch (type) {
        case Hero::AnimationType::Idle:   return Hero::HeroAnimations::idle;
        case Hero::AnimationType::Swim:   return Hero::HeroAnimations::swim;
        case Hero::AnimationType::Jump:   return Hero::HeroAnimations::jump;
        case Hero::AnimationType::Attack: return Hero::HeroAnimations::attack;
        default:                          return Hero::HeroAnimations::idle;
        }
    }

    // sequence dereferenzen mit *sequence

    

    void RenderSystem::step(float deltaTime) noexcept {
        renderMap();
        glm::uvec2 extent = window->getWindowExtent();
        glViewport(0, 0, extent.x, extent.y);
        //glClearColor(0.674f, 0.847f, 1.0f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::ortho(0.f, static_cast<float>(extent.x), 0.f, static_cast<float>(extent.y), 0.1f, 10.f);
        
        glBindVertexArray(m_quad_vao);


        for (Entity e : registry->animations.entities) {
            auto& animComp = registry->animations.get(e);
            animComp.anim.step(deltaTime);
            // not working correctly yet, needs fix
            // idea was to get the animation type like "Idle", "Swim" etc from animations.get(e) 
            // and then we chose the correct textureBuffer and the correct animationSequence and render
            // the animation, but currently the animationSequence is fixed and cant be changed in this loop
            // needs change, but it works to display hero idle animation for milestone 2
            int texIndex = animComp.anim.getCurrentFrameTextureIndex(); 
            Texture& tex = getCurrentTexture(animComp, texIndex);
            tex.bind(0);
            std::vector<int> animationOrder = getHeroAnimationSequence(animComp.currentType);


            glm::vec2 pos = registry->positions.get(e);
            glm::vec2 scale = registry->scales.get(e);
            glm::vec2 pixelPos = glm::vec2(pos.x * extent.x, pos.y * extent.y);
            glm::vec2 pixelScale = glm::vec2(scale.x * extent.x, scale.y * extent.y);



            glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(pixelPos, 0));
            model = glm::scale(model, glm::vec3(pixelScale, 1));

            Shader& shader = heroShader;
            shader.use()
                .setMat4("model", model)
                .setMat4("view", view)
                .setMat4("projection", projection)
                .setVec3("fcolor", glm::vec3(1.0f));

        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }


    void RenderSystem::reset() noexcept {}

    static constexpr float GAME_ASPECT_RATIO = 1.f / 2;

    static std::pair<glm::uvec2, glm::uvec2> computeViewportConfig(const glm::uvec2 resolution) noexcept {
        float window_aspect_ratio = static_cast<float>(resolution.x) / resolution.y;
        if (window_aspect_ratio > GAME_ASPECT_RATIO) {
            glm::uvec2 viewport_size = { resolution.y * GAME_ASPECT_RATIO, resolution.y };
            uint32_t offset = (resolution.x - viewport_size.x) / 2;
            glm::uvec2 viewport_offset = { offset, 0 };
            return { viewport_offset, viewport_size };
        }

        glm::uvec2 viewport_size = { resolution.x, resolution.x * 1 / GAME_ASPECT_RATIO };
        uint32_t offset = (resolution.y - viewport_size.y) / 2;
        glm::uvec2 viewport_offset = { 0, offset };
        return { viewport_offset, viewport_size };
    }

    void RenderSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
        auto [origin, size] = computeViewportConfig({ width, height });
        m_viewport.m_origin = origin;
        m_viewport.m_size = size;

		// reinitialize off-screen framebuffer
		intermediateFramebuffer.deinit();
		intermediateFramebuffer = Framebuffer::init({ (GLsizei)size.x, (GLsizei)size.y, 1, true });
	}


	/**
	 * Initializes a mesh of one tile.
	 */
	std::vector<float> RenderSystem::createTileMesh() noexcept {
		// Appends the hexagons corners counter-clockwise to the vertices array.
		// The center of the hexagon is at the origin.
		// It is rotated by 30 degrees in order to have a corner at the top,
		// as the tile textures already created have also the corner at the top.

		constexpr float SQRT_3_DIV_2 = 0.866025404f; //sqrt(3.0) / 2.0f;
		std::vector<TileVertex> vertices;
		for (int vertex = 0; vertex < 6; vertex++) {
			const float angle = M_PI / 180.0f * (60.0f * static_cast<float>(vertex) - 30.0f);
			float x = std::cos(angle);
			float y = std::sin(angle);
			float u = (x + SQRT_3_DIV_2) / (2.0f * SQRT_3_DIV_2);
			float v = (y + 1.0f) / 2.0f;
			vertices.emplace_back(glm::vec2(x, y), glm::vec2(u, v));
		}

		// This is a triangulation I've come up with on my ipad.
		// The triangles are counter-clockwise as the vertices above
		// are counter-clockwise around the origin.

		std::vector<float> meshData;

		// Big center triangle
		for (int i = 0; i < 6; i += 2) {
			meshData.push_back(vertices[i].position.x);
			meshData.push_back(vertices[i].position.y);
			meshData.push_back(vertices[i].uv.x);
			meshData.push_back(vertices[i].uv.y);
		}

		// Three side triangles
		for (int i = 0; i < 3; i++) {
			meshData.push_back(vertices[2 * i + 0].position.x);
			meshData.push_back(vertices[2 * i + 0].position.y);
			meshData.push_back(vertices[2 * i + 0].uv.x);
			meshData.push_back(vertices[2 * i + 0].uv.y);

			meshData.push_back(vertices[2 * i + 1].position.x);
			meshData.push_back(vertices[2 * i + 1].position.y);
			meshData.push_back(vertices[2 * i + 1].uv.x);
			meshData.push_back(vertices[2 * i + 1].uv.y);

			meshData.push_back(vertices[(2 * i + 2) % 6].position.x);
			meshData.push_back(vertices[(2 * i + 2) % 6].position.y);
			meshData.push_back(vertices[(2 * i + 2) % 6].uv.x);
			meshData.push_back(vertices[(2 * i + 2) % 6].uv.y);
		}

		return meshData;
	}

	std::vector<float> RenderSystem::createRectangularTileMesh() noexcept {
		std::vector<TileVertex> vertices;
		vertices.push_back({{1, -1}, {1, 0}});
		vertices.push_back({{1, 1}, {1, 1}});
		vertices.push_back({{-1, 1}, {0, 1}});
		vertices.push_back({{-1, -1}, {0, 0}});

		std::vector<float> meshData;
		for (int i = 0; i < 2; i++) {
			meshData.push_back(vertices[2 * i + 0].position.x);
			meshData.push_back(vertices[2 * i + 0].position.y);
			meshData.push_back(vertices[2 * i + 0].uv.x);
			meshData.push_back(vertices[2 * i + 0].uv.y);

			meshData.push_back(vertices[2 * i + 1].position.x);
			meshData.push_back(vertices[2 * i + 1].position.y);
			meshData.push_back(vertices[2 * i + 1].uv.x);
			meshData.push_back(vertices[2 * i + 1].uv.y);

			meshData.push_back(vertices[(2 * i + 2) % 4].position.x);
			meshData.push_back(vertices[(2 * i + 2) % 4].position.y);
			meshData.push_back(vertices[(2 * i + 2) % 4].uv.x);
			meshData.push_back(vertices[(2 * i + 2) % 4].uv.y);
		}


		return meshData;
	}

	/**
	 * Creates a vector of the tile-specific data for the whole map arranged in a "rectangle".
	 * The tile specific data is the position and tile type, yet.
	 */
	std::vector<RenderSystem::TileInstance> RenderSystem::createTileInstances(const int columns, const int rows) noexcept {
		// TODO: Add dedicated world generator.
		auto randomEngine = std::default_random_engine(std::random_device()());
		auto uniformTileTypeDistribution = std::uniform_int_distribution(2, static_cast<int>(df::types::TileType::COUNT) - 1);

		std::vector<TileInstance> instances;

		// Only one ice-desert tile -> like in catan game
		std::unordered_map<int, int> tileCount;
    	std::unordered_map<int, int> tileMax = {{ (int)df::types::TileType::ICE,    1 }};

		for (int row = rows - 1; row >= 0; row--) {
			for (int column = 0; column < columns; column++) {
				glm::vec2 position;
				position.x = 2.0f * (column + 0.5f * (row & 1));
				position.y = row * 1.5f;
				// Creating an island with two water wide borders
				if(row<1 || column <1 || row > rows -2 || column > columns -2){
				    // make border tiles water
				    instances.push_back({position, static_cast<int>(df::types::TileType::WATER), 0, 1});
				    continue;
				}
				int type = uniformTileTypeDistribution(randomEngine);
				
				if(tileMax.contains(type)){
				    if(tileCount[type] >= tileMax[type]){
						do {
							type = uniformTileTypeDistribution(randomEngine); // TODO: look for a more optimal solution 
						} while (type == (int)df::types::TileType::ICE);
					} else {
				        tileCount[type]++;
				    }
				}

				instances.push_back({position, type, 0, uniformTileTypeDistribution(randomEngine) % 6 > 2});
			}
		}
		return instances;
	}

	/**
	 * Initializes the map data
	 */
	void RenderSystem::initMap() noexcept {
		this->tileMesh = createRectangularTileMesh();
		this->tileInstances = createTileInstances(10, 10);

		glGenVertexArrays(1, &tileVao);
		glGenBuffers(1, &tileVbo);
		glGenBuffers(1, &tileInstanceVbo);

		{
			glBindVertexArray(tileVao);

			tileInstances[0].explored = 1; // for testing
			tileInstances[10].explored = 1; // for testing

			glBindBuffer(GL_ARRAY_BUFFER, tileVbo);
			glBufferData(GL_ARRAY_BUFFER, this->tileMesh.size() * sizeof(float), this->tileMesh.data(), GL_STATIC_DRAW);

			// layout(location = 0) in vec2 position;
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, position));

			// layout(location = 1) in vec2 vertexUv;
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, uv));

			// Define instance attributes
			glBindBuffer(GL_ARRAY_BUFFER, tileInstanceVbo);
			glBufferData(GL_ARRAY_BUFFER, this->tileInstances.size() * sizeof(TileInstance), this->tileInstances.data(), GL_STATIC_DRAW);

			// layout(location = 2) in vec2 instancePosition;
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance), (void*)offsetof(TileInstance, position));
			glVertexAttribDivisor(2, 1);

			// layout(location = 3) in int type;
			glEnableVertexAttribArray(3);
			glVertexAttribIPointer(3, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, type));
			glVertexAttribDivisor(3, 1);

			// layout(location = 4) in int explored;
			glEnableVertexAttribArray(4);
			glVertexAttribIPointer(4, 1, GL_INT, sizeof(TileInstance), (void*)offsetof(TileInstance, explored));
			glVertexAttribDivisor(4, 1);

			glBindVertexArray(0);
		}
	}


	glm::vec2 RenderSystem::calculateWorldDimensions(const int columns, const int rows) noexcept {
		return {
			sqrt(3.0f) * (columns + 0.5f),
			1.5f * (rows + 1.0f)
		};
	}

	void RenderSystem::renderMap(const glm::vec2 scale) const noexcept {
		const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);
		

		Camera& cam = registry->cameras.get(registry->getCamera());
		glm::vec2 camPos = cam.position;
		float camZoom = cam.zoom;

		const glm::mat4 projection = glm::ortho(
			camPos.x, camPos.x + worldDimensions.x / camZoom,
			camPos.y, camPos.y + worldDimensions.y / camZoom,
			-1.0f, 1.0f
		);

		glm::mat4 model = glm::identity<glm::mat4>();
		// camera translation: move the world around the camera position
		model = glm::translate(model, glm::vec3(-camPos, 0.0f));

		model = glm::scale(model, glm::vec3(scale, 1));

		tileAtlas.bind(0);
		tileShader.use()
			.setMat4("model", model)
			.setMat4("projection", projection)
			.setSampler("tileAtlas", 0);

		glBindVertexArray(tileVao);
		glDrawArraysInstanced(GL_TRIANGLES, 0, this->tileMesh.size() / FLOATS_PER_TILE_VERTEX, this->tileInstances.size());
		glBindVertexArray(0);
	}

	void RenderSystem::updateFogOfWar(const Player* player) noexcept {
		if (player == nullptr) return;

		for(auto& instance : tileInstances) {
			instance.explored = 0;
		}

		const auto& exploredTiles = player->getExploredTiles();
    	for (const Tile* tile : exploredTiles) {
        	if (tile == nullptr) continue;

        	size_t tileId = tile->getId();
        	if (tileId < tileInstances.size()) {
        	    tileInstances[tileId].explored = 1;
        	}
    	}

		glBindBuffer(GL_ARRAY_BUFFER, tileInstanceVbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0,
						tileInstances.size() * sizeof(TileInstance),
						tileInstances.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	// Converts screen coordinates to world coordinates
	glm::vec2 RenderSystem::screenToWorldCoordinates(const glm::vec2& screenPos) const noexcept {
	    const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);

		glm::vec2 viewportPos = screenPos - glm::vec2(this->m_viewport.m_origin);
		glm::vec2 normalizedPos = viewportPos / glm::vec2(this->m_viewport.m_size);
		normalizedPos.y = 1.0f - normalizedPos.y; // flip y: screen-y increases downwards, world-y up

		return normalizedPos * worldDimensions;
	}


	// TODO: combine into one function if there are more builting types to preview
	// Then pass some kind of building descriptor as parameter to the new function
	// -> beware NOT to create a gigantic switch statement

	void RenderSystem::renderSettlementPreview(const glm::vec2& worldPosition, bool active, float time) noexcept {
		if (!active) return;

		// Set viewport = game viewport
		glViewport(this->m_viewport.m_origin.x, this->m_viewport.m_origin.y, this->m_viewport.m_size.x, this->m_viewport.m_size.y);

		const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);
		const glm::mat4 projection = glm::ortho(0.0f, worldDimensions.x, 0.0f, worldDimensions.y, -1.0f, 1.0f);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const float settlementSize = 0.5f;
		const float shadowOffsetY = -0.15f;
		const float shadowScale = 1.4f; // shadow is a bit larger than actual settlement

		// render shadow first = below settlement-textrue
		glm::mat4 shadowModel = glm::identity<glm::mat4>();
		shadowModel = glm::translate(shadowModel, glm::vec3(worldPosition.x, worldPosition.y + shadowOffsetY, -0.01f)); // Slightly behind
		shadowModel = glm::scale(shadowModel, glm::vec3(settlementSize * shadowScale, settlementSize * shadowScale, 1.0f));

		this->settlementTexture.bind(0);
		this->buildingShadowShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", shadowModel)
			.setSampler("sprite", 0);

		glBindVertexArray(this->m_quad_vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// render settlement on top of shadow
		glm::mat4 hoverModel = glm::identity<glm::mat4>();
		hoverModel = glm::translate(hoverModel, glm::vec3(worldPosition, 0.0f));
		hoverModel = glm::scale(hoverModel, glm::vec3(settlementSize, settlementSize, 1.0f));

		this->settlementTexture.bind(0);
		this->buildingHoverShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", hoverModel)
			.setSampler("sprite", 0)
			.setVec3("fcolor", glm::vec3(1.0f, 1.0f, 1.0f))
			.setFloat("time", time);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}


	void RenderSystem::renderRoadPreview(const glm::vec2& worldPosition, bool active, float time) noexcept {
		if (!active) return;

		glViewport(this->m_viewport.m_origin.x, this->m_viewport.m_origin.y, this->m_viewport.m_size.x, this->m_viewport.m_size.y);

		const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);
		const glm::mat4 projection = glm::ortho(0.0f, worldDimensions.x, 0.0f, worldDimensions.y, -1.0f, 1.0f);
		const glm::mat4 view = glm::identity<glm::mat4>();

		const glm::vec2 baseRoadScale = glm::vec2(1.0f, 0.5f);
		const glm::vec2 roadScale = baseRoadScale * 2.5f;
		const float roadShadowOffsetY = -0.2f;
		const float roadShadowScale = 1.1f;

		// TODO: get shadow gradient to work (and look good, similar to settlement) -> wasted to much time on this for now
		glm::mat4 shadowModel = glm::identity<glm::mat4>();
		shadowModel = glm::translate(shadowModel, glm::vec3(worldPosition.x, worldPosition.y + roadShadowOffsetY, -0.01f));
		shadowModel = glm::scale(shadowModel, glm::vec3(roadScale * roadShadowScale, 1.0f));

		this->roadPreviewTexture.bind(0);
		this->buildingShadowShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", shadowModel)
			.setSampler("sprite", 0);

		glBindVertexArray(this->m_quad_vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		// Road sprite pass
		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::translate(model, glm::vec3(worldPosition, 0.0f));
		model = glm::scale(model, glm::vec3(roadScale, 1.0f));

		this->roadPreviewTexture.bind(0);
		this->buildingHoverShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", model)
			.setSampler("sprite", 0)
			.setVec3("fcolor", glm::vec3(1.0f))
			.setFloat("time", time);

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

}

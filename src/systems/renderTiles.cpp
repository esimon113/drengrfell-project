#include "renderTiles.h"
#include <iostream>
#include "../core/player.h"
#include "../core/tile.h"
#include "utils/textureArray.h"
#include "common.h"

namespace df {

    RenderTilesSystem RenderTilesSystem::init(Window* window, Registry* registry) noexcept {

        RenderTilesSystem self;

        self.window = window;
        self.registry = registry;

        self.viewport.origin = glm::uvec2(0);
        self.viewport.size = self.window->getWindowExtent();

		// load resources for rendering
		self.tileShader = Shader::init(assets::Shader::tile).value();
		self.tileAtlas = TextureArray::init(assets::Texture::TILE_ATLAS);

		const glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({ static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true });

		self.initMap();

		return self;
	}


	void RenderTilesSystem::initMap() noexcept {
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


	void RenderTilesSystem::deinit() noexcept {
		tileShader.deinit();
		tileAtlas.deinit();
	}


	float accumulator = 0.0f;
    void RenderTilesSystem::step(const float delta) noexcept {
    	accumulator += delta;
    	if (accumulator > 1.0) {
    		accumulator = 0.0f;
    	}
        renderMap(accumulator);
    }


	void RenderTilesSystem::renderMap(const float timeInSeconds, const glm::vec2 scale) const noexcept {
    	glEnable(GL_BLEND);
    	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    	const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);

    	Camera& cam = registry->cameras.get(registry->getCamera());
    	glm::vec2 camPos = cam.position;
    	float camZoom = cam.zoom;

    	const glm::mat4 projection = glm::ortho(
			camPos.x, camPos.x + worldDimensions.x / camZoom,
			camPos.y, camPos.y + worldDimensions.y / camZoom,
			-1.0f, 1.0f
		);

    	auto model = glm::identity<glm::mat4>();
    	model = glm::translate(model, glm::vec3(-camPos, 0.0f));
    	model = glm::scale(model, glm::vec3(scale, 1));

    	tileAtlas.bind(0);
    	tileShader.use()
			.setMat4("model", model)
			.setMat4("projection", projection)
	    	.setFloat("time", timeInSeconds)
    		.setInt("frames", 4)
			.setSampler("tileAtlas", 0);

    	glBindVertexArray(tileVao);
    	glDrawArraysInstanced(GL_TRIANGLES, 0, this->tileMesh.size() / FLOATS_PER_TILE_VERTEX, this->tileInstances.size());
    	glBindVertexArray(0);
    }


    void RenderTilesSystem::reset() noexcept {}


	std::vector<float> RenderTilesSystem::createRectangularTileMesh() noexcept {
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
	std::vector<RenderTilesSystem::TileInstance> RenderTilesSystem::createTileInstances(const int columns, const int rows) noexcept {
		// TODO: Add dedicated world generator.
		auto randomEngine = std::default_random_engine(std::random_device()());
		auto uniformTileTypeDistribution = std::uniform_int_distribution(2, static_cast<int>(df::types::TileType::COUNT) - 1);

		std::vector<TileInstance> instances;

		// Only one ice-desert tile -> like in catan game
		std::unordered_map<int, int> tileCount;
    	std::unordered_map<int, int> tileMax = {{ (int)types::TileType::ICE,    1 }};

		for (int row = rows - 1; row >= 0; row--) {
			for (int column = 0; column < columns; column++) {
				glm::vec2 position;
				position.x = 2.0f * (column + 0.5f * (row & 1));
				position.y = row * 1.5f;
				// Creating an island with two water wide borders
				if(row<1 || column <1 || row > rows -2 || column > columns -2){
				    // make border tiles water
				    instances.push_back({position, static_cast<int>(types::TileType::WATER), 0, 1});
				    continue;
				}
				int type = uniformTileTypeDistribution(randomEngine);

				if(tileMax.contains(type)){
				    if(tileCount[type] >= tileMax[type]){
						do {
							type = uniformTileTypeDistribution(randomEngine); // TODO: look for a more optimal solution
						} while (type == static_cast<int>(types::TileType::ICE));
					} else {
				        tileCount[type]++;
				    }
				}

				instances.push_back({position, type, 0, uniformTileTypeDistribution(randomEngine) % 6 > 2});
			}
		}
		return instances;
	}


	void RenderTilesSystem::updateFogOfWar(const Player& player) noexcept {
    	for(auto& instance : tileInstances) {
    		instance.explored = 0;
    	}

    	for (size_t tileId : player.getExploredTileIds()) {
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


}

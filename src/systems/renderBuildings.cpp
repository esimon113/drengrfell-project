#include "renderBuildings.h"
#include "GL/glcorearb.h"
#include "core/camera.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_uint2.hpp"
#include "glm/gtc/constants.hpp"
#include "systems/renderCommon.h"
#include "utils/worldNodeMapper.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <GLFW/glfw3.h>



namespace df {

	RenderBuildingsSystem RenderBuildingsSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState) noexcept {
		RenderBuildingsSystem self;

		self.window = window;
		self.registry = registry;
		self.gamestate = gameState;

		self.viewport.origin = glm::uvec2(0);
		self.viewport.size = self.window->getWindowExtent();

		self.spriteShader = Shader::init(assets::Shader::sprite).value();
		self.locationHighlightShader = Shader::init(assets::Shader::locationHighlight).value();
		self.roadTextureDiagonalUp = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_UP);
		self.roadTextureDiagonalDown = Texture::init(assets::Texture::DIRT_ROAD_DIAGONAL_DOWN);
		self.roadTextureVertical = Texture::init(assets::Texture::DIRT_ROAD_VERTICAL);

		// Load all settlement textures
		self.settlementTextures[0] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT1);
		self.settlementTextures[1] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT2);
		self.settlementTextures[2] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT3);
		self.settlementTextures[3] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT4);
		self.settlementTextures[4] = Texture::init(assets::Texture::VIKING_WOOD_SETTLEMENT5);

		glm::uvec2 extent = self.window->getWindowExtent();
		self.intermediateFramebuffer = Framebuffer::init({static_cast<GLsizei>(extent.x), static_cast<GLsizei>(extent.y), 1, true});

		float quadVertices[] = {
			// positions (centered at origin)	// texcoords
			-0.5f, -0.5f, 0.0f, 0.0f,
			0.5f, -0.5f, 1.0f, 0.0f,
			0.5f, 0.5f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 1.0f};
		constexpr GLuint quadIndices[] = {0, 1, 2, 2, 3, 0};

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

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		self.rng = std::mt19937(std::random_device{}());

		return self;
	}


	void RenderBuildingsSystem::deinit() noexcept {
		spriteShader.deinit();
		locationHighlightShader.deinit();
		roadTextureDiagonalUp.deinit();
		roadTextureDiagonalDown.deinit();
		roadTextureVertical.deinit();

		for (auto& tex : settlementTextures) {
			tex.deinit();
		}

		intermediateFramebuffer.deinit();
	}


	void RenderBuildingsSystem::step(float /*dt*/) noexcept {
		float time = static_cast<float>(glfwGetTime());
		renderBuildings(time);
	}


	void RenderBuildingsSystem::reset() noexcept {
		dustPuffs.clear();
		lastSettlementCount = 0;
		lastRoadCount = 0;
		lastHoveredConnectedEdges.clear();
		lastHoveredEdgeId = SIZE_MAX;
		hoverFadeStartTime = 0.0f;
		hoverActive = false;
	}

	glm::vec2 RenderBuildingsSystem::getCursorWorldPos(const Camera& cam) const noexcept {
		Viewport cursorViewport;
		cursorViewport.origin = glm::uvec2(0);
		cursorViewport.size = window->getWindowExtent();
		glm::vec2 cursorScreenPos = window->getCursorPosition();
		glm::vec2 cursorWorldOffset = screenToWorldCoordinates(
			cursorScreenPos,
			cursorViewport,
			glm::vec2(cam.viewWidth, cam.viewHeight));
		return cam.position + cursorWorldOffset;
	}

	float RenderBuildingsSystem::distanceToEdge(const Graph& map, const glm::vec2& cursorWorldPos, size_t edgeId) const noexcept {
		EdgeHandle edge = map.findEdgeById(edgeId);
		if (!edge)
			return (std::numeric_limits<float>::max)();
		auto edgeVertices = map.getEdgeVertices(edge);
		if (!edgeVertices)
			return (std::numeric_limits<float>::max)();

		glm::vec2 v0 = WorldNodeMapper::getWorldPositionForVertex((*edgeVertices)[0]->getId(), map);
		glm::vec2 v1 = WorldNodeMapper::getWorldPositionForVertex((*edgeVertices)[1]->getId(), map);
		glm::vec2 segment = v1 - v0;
		float segLenSq = glm::dot(segment, segment);
		if (segLenSq <= 0.0001f)
			return glm::distance(cursorWorldPos, v0);
		float t = glm::dot(cursorWorldPos - v0, segment) / segLenSq;
		t = glm::clamp(t, 0.0f, 1.0f);
		glm::vec2 closestPoint = v0 + segment * t;
		return glm::distance(cursorWorldPos, closestPoint);
	}

	size_t RenderBuildingsSystem::findClosestOwnedEdge(const Graph& map,
		const glm::vec2& cursorWorldPos,
		const std::unordered_map<size_t, Entity>& ownedRoadEntitiesByEdge,
		float& outClosestDistance) const noexcept {
		size_t hoveredEdgeId = SIZE_MAX;
		float closestDistance = (std::numeric_limits<float>::max)();
		for (const auto& [edgeId, _] : ownedRoadEntitiesByEdge) {
			float distance = distanceToEdge(map, cursorWorldPos, edgeId);
			if (distance < closestDistance) {
				closestDistance = distance;
				hoveredEdgeId = edgeId;
			}
		}
		outClosestDistance = closestDistance;
		return hoveredEdgeId;
	}

	std::unordered_set<size_t> RenderBuildingsSystem::collectConnectedOwnedEdges(const Graph& map,
		size_t startEdgeId,
		const std::unordered_map<size_t, Entity>& ownedRoadEntitiesByEdge) const noexcept {
		std::unordered_set<size_t> visited;
		if (startEdgeId == SIZE_MAX)
			return visited;

		std::queue<size_t> toVisit;
		visited.insert(startEdgeId);
		toVisit.push(startEdgeId);

		while (!toVisit.empty()) {
			size_t edgeId = toVisit.front();
			toVisit.pop();

			EdgeHandle edge = map.findEdgeById(edgeId);
			if (!edge)
				continue;
			auto edgeVertices = map.getEdgeVertices(edge);
			if (!edgeVertices)
				continue;

			for (const auto& vertex : *edgeVertices) {
				if (!vertex)
					continue;
				auto vertexEdges = map.getVertexEdges(vertex);
				if (!vertexEdges)
					continue;
				for (const auto& connectedEdge : *vertexEdges) {
					if (!connectedEdge)
						continue;
					size_t connectedId = connectedEdge->getId();
					if (ownedRoadEntitiesByEdge.contains(connectedId) && visited.insert(connectedId).second) {
						toVisit.push(connectedId);
					}
				}
			}
		}

		return visited;
	}

	void RenderBuildingsSystem::renderRoadHighlights(const std::unordered_set<size_t>& edgeIds,
		const std::unordered_map<size_t, glm::vec2>& ownedRoadPosByEdge,
		const std::unordered_map<size_t, int>& ownedRoadEdgeIndexByEdge,
		float time,
		float baseAlpha,
		const glm::mat4& view,
		const glm::mat4& projection) noexcept {
		const glm::vec3 highlightColor = glm::vec3(0.95f, 0.86f, 0.55f);
		const float diagonalAngle = std::atan(2.0f);
		const std::array<float, 3> rotationAngles = {0.0f, diagonalAngle, -diagonalAngle};

		for (size_t edgeId : edgeIds) {
			auto posIt = ownedRoadPosByEdge.find(edgeId);
			auto idxIt = ownedRoadEdgeIndexByEdge.find(edgeId);
			if (posIt == ownedRoadPosByEdge.end() || idxIt == ownedRoadEdgeIndexByEdge.end())
				continue;

			float rotationAngle = rotationAngles[idxIt->second % 3];
			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(posIt->second, 0.0f));
			model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(0.35f, 1.15f, 1.0f));

			locationHighlightShader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setMat4("model[0]", model)
				.setVec3("highlightColor", highlightColor)
				.setFloat("alpha", baseAlpha)
				.setFloat("time", time)
				.setFloat("pulseStrength", 0.35f);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}


	const glm::mat4 RenderBuildingsSystem::calculateProjection(const Camera& cam) const {
		glm::uvec2 extent = window->getWindowExtent();
		glViewport(0, 0, extent.x, extent.y);

		return glm::ortho(
			cam.minX(), cam.maxX(),
			cam.minY(), cam.maxY(),
			-1.0f, 1.0f);
	}


	void RenderBuildingsSystem::renderBuildings(float time) noexcept {
		if (!registry || !gamestate)
			return;

		glBindVertexArray(m_quad_vao);
		Camera& cam = registry->cameras.get(registry->getCamera());

		const glm::mat4 view = glm::identity<glm::mat4>();
		const glm::mat4 projection = this->calculateProjection(cam);

		size_t currentSettlementCount = registry->settlements.entities.size();
		size_t currentRoadCount = registry->roads.entities.size();
		if (currentSettlementCount < lastSettlementCount || currentRoadCount < lastRoadCount) {
			dustPuffs.clear();
			lastSettlementCount = currentSettlementCount;
			lastRoadCount = currentRoadCount;
		}

		if (currentSettlementCount > lastSettlementCount) {
			for (size_t i = lastSettlementCount; i < currentSettlementCount; ++i) {
				Entity e = registry->settlements.entities[i];
				if (registry->positions.has(e)) {
					spawnDustAt(registry->positions.get(e), time, 8, 0.7f);
				}
			}
		}
		if (currentRoadCount > lastRoadCount) {
			for (size_t i = lastRoadCount; i < currentRoadCount; ++i) {
				Entity e = registry->roads.entities[i];
				if (registry->positions.has(e)) {
					spawnDustAt(registry->positions.get(e), time, 6, 0.5f);
				}
			}
		}
		lastSettlementCount = currentSettlementCount;
		lastRoadCount = currentRoadCount;

		// TODO: use consistent FPS for animations
		constexpr float animationSpeed = 5.0f; // fps
		constexpr int numFrames = 5;		   // how many frames per animation run
		int textureIndex = static_cast<int>(time * animationSpeed) % numFrames;

		// Render settlements from ECS
		for (Entity e : registry->settlements.entities) {
			if (!registry->positions.has(e) || !registry->scales.has(e))
				continue;

			// const Settlement& settlement = registry->settlements.get(e);
			const glm::vec2& worldPos = registry->positions.get(e);
			const glm::vec2& scale = registry->scales.get(e);

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(worldPos, 0.0f));
			model = glm::scale(model, glm::vec3(scale, 1.0f));

			// Use animated settlement texture
			settlementTextures[textureIndex].bind(0);
			spriteShader.use()
				.setMat4("view", view)
				.setMat4("model[0]", model)
				.setMat4("projection", projection)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f));

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// Render roads from ECS
		std::unordered_map<size_t, Entity> ownedRoadEntitiesByEdge;
		std::unordered_map<size_t, int> ownedRoadEdgeIndexByEdge;
		std::unordered_map<size_t, glm::vec2> ownedRoadPosByEdge;
		const size_t currentPlayerId = gamestate->getCurrentPlayerId();
		for (Entity e : registry->roads.entities) {
			if (!registry->positions.has(e) || !registry->scales.has(e))
				continue;

			const glm::vec2& worldPos = registry->positions.get(e);
			const glm::vec2& scale = registry->scales.get(e);
			const Road& road = registry->roads.get(e);
			const size_t roadEdgeId = road.getEdgeId();

			int edgeIndex = this->registry->roadEdgeIndices.has(e) ? this->registry->roadEdgeIndices.get(e) : -1;

			Texture* roadTexture = nullptr;
			if (edgeIndex == 0 || edgeIndex == 3)
				roadTexture = &roadTextureVertical;
			else if (edgeIndex == 1 || edgeIndex == 4)
				roadTexture = &roadTextureDiagonalDown;
			else if (edgeIndex == 2 || edgeIndex == 5)
				roadTexture = &roadTextureDiagonalUp;
			else
				roadTexture = &roadTextureVertical;

			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(worldPos, 0.0f));
			model = glm::scale(model, glm::vec3(scale, 1.0f));

			assert(roadTexture != nullptr); // should not fail
			roadTexture->bind(0);
			spriteShader.use()
				.setMat4("model[0]", model)
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setSampler("sprite", 0)
				.setVec3("fcolor", glm::vec3(1.0f));

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

			if (road.getPlayerId() == currentPlayerId) {
				ownedRoadEntitiesByEdge[roadEdgeId] = e;
				ownedRoadEdgeIndexByEdge[roadEdgeId] = edgeIndex;
				ownedRoadPosByEdge[roadEdgeId] = worldPos;
			}
		}

		const bool isBuildingPreviewActive = !registry->buildingPreviews.entities.empty();
		if (!ownedRoadEntitiesByEdge.empty()) {
			const Graph& map = gamestate->getMap();
			const float hoverRadius = 0.12f / cam.zoom;
			size_t hoveredEdgeId = SIZE_MAX;
			float closestDistance = (std::numeric_limits<float>::max)();
			bool isHovering = false;

			if (!isBuildingPreviewActive) {
				glm::vec2 cursorWorldPos = getCursorWorldPos(cam);
				hoveredEdgeId = findClosestOwnedEdge(map, cursorWorldPos, ownedRoadEntitiesByEdge, closestDistance);
				isHovering = hoveredEdgeId != SIZE_MAX && closestDistance <= hoverRadius;
			}
			if (isHovering && (!hoverActive || hoveredEdgeId != lastHoveredEdgeId)) {
				hoverFadeStartTime = time;
				hoverActive = true;
				lastHoveredEdgeId = hoveredEdgeId;
			} else if (!isHovering && hoverActive) {
				hoverFadeStartTime = time;
				hoverActive = false;
			}

			if (isHovering) {
				lastHoveredConnectedEdges = collectConnectedOwnedEdges(map, hoveredEdgeId, ownedRoadEntitiesByEdge);
			}

			if (!lastHoveredConnectedEdges.empty()) {
				float elapsed = time - hoverFadeStartTime;
				float fadeT = glm::clamp(elapsed / hoverFadeDuration, 0.0f, 1.0f);
				float fade = hoverActive ? glm::smoothstep(0.0f, 1.0f, fadeT) : (1.0f - glm::smoothstep(0.0f, 1.0f, fadeT));
				if (!hoverActive && fadeT >= 1.0f) {
					lastHoveredConnectedEdges.clear();
					lastHoveredEdgeId = SIZE_MAX;
				}

				const float baseAlpha = 0.32f * fade;
				renderRoadHighlights(lastHoveredConnectedEdges, ownedRoadPosByEdge, ownedRoadEdgeIndexByEdge, time, baseAlpha, view, projection);
			}
		}

		renderDust(time, view, projection, cam);

		glBindVertexArray(0);
	}

	void RenderBuildingsSystem::spawnDustAt(const glm::vec2& worldPos, float time, int count, float baseSize) noexcept {
		std::uniform_real_distribution<float> angleDist(0.0f, glm::two_pi<float>());
		std::uniform_real_distribution<float> radiusDist(0.0f, 1.0f);
		std::uniform_real_distribution<float> sizeJitter(0.6f, 1.1f);
		std::uniform_real_distribution<float> lifeJitter(0.5f, 0.9f);

		for (int i = 0; i < count; ++i) {
			float angle = angleDist(rng);
			float radius = radiusDist(rng) * baseSize * 0.6f;
			glm::vec2 offset = glm::vec2(std::cos(angle), std::sin(angle)) * radius;

			DustPuff puff;
			puff.position = worldPos + offset;
			puff.startTime = time;
			puff.duration = lifeJitter(rng);
			puff.size = baseSize * sizeJitter(rng);
			puff.baseAlpha = 0.22f;
			dustPuffs.push_back(puff);
		}
	}

	void RenderBuildingsSystem::renderDust(float time, const glm::mat4& view, const glm::mat4& projection, const Camera& cam) noexcept {
		if (dustPuffs.empty())
			return;

		(void)cam;
		dustPuffs.erase(std::remove_if(dustPuffs.begin(), dustPuffs.end(), [time](const DustPuff& puff) {
			return (time - puff.startTime) >= puff.duration;
		}),
			dustPuffs.end());

		const glm::vec3 dustColor = glm::vec3(0.82f, 0.76f, 0.66f);
		for (const auto& puff : dustPuffs) {
			float age = (time - puff.startTime) / puff.duration;
			float fade = std::exp(-2.2f * age);
			float alpha = puff.baseAlpha * fade;
			if (alpha < 0.02f)
				continue;

			float size = puff.size * (1.0f + 0.6f * age);
			glm::mat4 model = glm::identity<glm::mat4>();
			model = glm::translate(model, glm::vec3(puff.position, 0.0f));
			model = glm::scale(model, glm::vec3(size, size, 1.0f));

			locationHighlightShader.use()
				.setMat4("view", view)
				.setMat4("projection", projection)
				.setMat4("model[0]", model)
				.setVec3("highlightColor", dustColor)
				.setFloat("alpha", alpha)
				.setFloat("time", time)
				.setFloat("pulseStrength", 0.0f);

			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}
} // namespace df

#include "renderSettlementMenu.h"
#include "core/camera.h"
#include "glm/ext/matrix_transform.hpp"
#include "utils/worldNodeMapper.h"
#include "renderWeather.h"

#include <algorithm>
#include <sstream>

#include <GLFW/glfw3.h>

namespace df {

	RenderSettlementMenuSystem RenderSettlementMenuSystem::init(Window* window, Registry* registry, std::shared_ptr<GameState> gameState, GameController* gameController) noexcept {
		RenderSettlementMenuSystem self;

		self.window = window;
		self.registry = registry;
		self.gameState = gameState;
		self.gameController = gameController;

		self.viewport.origin = glm::uvec2(0);
		self.viewport.size = self.window->getWindowExtent();

		self.rectShader = Shader::init(assets::Shader::hud).value();
		self.locationHighlightShader = Shader::init(assets::Shader::locationHighlight).value();
		self.textureShader = Shader::init(assets::Shader::menu).value();
		self.menuBackgroundTexture = Texture::init(assets::Texture::NOTIFICATIONS_BACKGROUND);
		self.menuButtonTexture = Texture::init(assets::Texture::NOTIFICATIONS_BUTTON);

		float quad[] = {
			0.f, 0.f,
			1.f, 0.f,
			1.f, 1.f,
			0.f, 1.f};

		glGenVertexArrays(1, &self.quadVao);
		glBindVertexArray(self.quadVao);

		glGenBuffers(1, &self.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

		glBindVertexArray(0);

		float textureQuad[] = {
			0.f, 0.f, 0.f, 0.f,
			1.f, 0.f, 1.f, 0.f,
			1.f, 1.f, 1.f, 1.f,
			0.f, 1.f, 0.f, 1.f};

		glGenVertexArrays(1, &self.textureVao);
		glBindVertexArray(self.textureVao);

		glGenBuffers(1, &self.textureVbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.textureVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(textureQuad), textureQuad, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);

		float highlightQuadVertices[] = {
			// positions (centered at origin)	// texcoords
			-0.5f, -0.5f, 0.0f, 0.0f,
			0.5f, -0.5f, 1.0f, 0.0f,
			0.5f, 0.5f, 1.0f, 1.0f,
			-0.5f, 0.5f, 0.0f, 1.0f};

		glGenVertexArrays(1, &self.highlightVao);
		glBindVertexArray(self.highlightVao);

		glGenBuffers(1, &self.highlightVbo);
		glBindBuffer(GL_ARRAY_BUFFER, self.highlightVbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(highlightQuadVertices), highlightQuadVertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		glBindVertexArray(0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		return self;
	}

	void RenderSettlementMenuSystem::deinit() noexcept {
		rectShader.deinit();
		locationHighlightShader.deinit();
		textureShader.deinit();
		if (highlightVbo != 0) {
			glDeleteBuffers(1, &highlightVbo);
			highlightVbo = 0;
		}
		if (highlightVao != 0) {
			glDeleteVertexArrays(1, &highlightVao);
			highlightVao = 0;
		}
		if (textureVbo != 0) {
			glDeleteBuffers(1, &textureVbo);
			textureVbo = 0;
		}
		if (textureVao != 0) {
			glDeleteVertexArrays(1, &textureVao);
			textureVao = 0;
		}
	}

	void RenderSettlementMenuSystem::reset() noexcept {
		active = false;
		selectedSettlementId = SIZE_MAX;
		title.clear();
		textLines.clear();
		displayLines.clear();
		textPositions.clear();
		buttons.clear();
	}

	void RenderSettlementMenuSystem::showMenu(size_t settlementId) {
		if (!registry || !gameState || !gameController) {
			return;
		}
		auto* WeatherSystem = this->registry->getSystem<df::RenderWeatherSystem>();
		const Settlement* settlementPtr = nullptr;
		for (const auto& settlement : gameState->getSettlements()) {
			if (settlement && settlement->getId() == settlementId) {
				settlementPtr = settlement.get();
				break;
			}
		}
		if (!settlementPtr) {
			close();
			return;
		}

		selectedSettlementId = settlementId;
		active = true;

		title = "Settlement " + std::to_string(settlementId);
		textLines.clear();
		displayLines.clear();
		textPositions.clear();
		buttons.clear();

		textLines.emplace_back("Productivity");

		types::WeatherType currentWeather = WeatherSystem->getCurrentType();

		const Graph& map = gameState->getMap();
		auto vertex = map.findVertexById(settlementPtr->getVertexId());
		if (vertex) {
			if (auto tilesOpt = map.getVertexTiles(vertex); tilesOpt) {
				for (const auto& tile : *tilesOpt) {
					if (!tile) {
						continue;
					}
					const auto type = tile->getType();
					if (type != types::TileType::FOREST &&
						type != types::TileType::MOUNTAIN &&
						type != types::TileType::GRASS &&
						type != types::TileType::FIELD &&
						type != types::TileType::CLAY) {
						continue;
					}

					const int percent = getPotencyPercent(tile->getEffectivePotency());
					textLines.emplace_back(std::string(types::tileTypeToString(type)) + " " + std::to_string(percent) + "%" + tile->getPotencyModifierLabel(currentWeather));
					
				}
			}
		}

		textLines.emplace_back("");
		textLines.emplace_back("Upgrades");

		auto makeCostVector = [](int wood, int grass, int stone, int field, int clay) {
			std::vector<int> cost(static_cast<size_t>(types::TileType::COUNT), 0);
			cost[static_cast<size_t>(types::TileType::FOREST)] = wood;
			cost[static_cast<size_t>(types::TileType::GRASS)] = grass;
			cost[static_cast<size_t>(types::TileType::MOUNTAIN)] = stone;
			cost[static_cast<size_t>(types::TileType::FIELD)] = field;
			cost[static_cast<size_t>(types::TileType::CLAY)] = clay;
			return cost;
		};

		const std::vector<int> stoneSettlementCost = makeCostVector(10, 10, 20, 10, 10);
		const std::vector<int> castleCost = makeCostVector(30, 20, 50, 40, 30);
		const std::vector<int> lumberCampCost = makeCostVector(10, 0, 30, 0, 20);
		const std::vector<int> stoneQuarryCost = makeCostVector(30, 0, 10, 0, 20);
		const std::vector<int> stableCost = makeCostVector(30, 0, 0, 10, 20);
		const std::vector<int> millCost = makeCostVector(20, 0, 20, 0, 20);
		const std::vector<int> brickKilnCost = makeCostVector(20, 10, 30, 0, 0);

		const types::SettlementType settlementType = settlementPtr->getSettlementType();
		if (settlementType == types::SettlementType::WOOD) {
			textLines.emplace_back("Stone Settlement: " + formatCostLine(stoneSettlementCost));
			buttons.push_back({"Stone Settlement", ButtonAction::UpgradeStone});
		} else {
			if (settlementType == types::SettlementType::STONE) {
				textLines.emplace_back("Castle: " + formatCostLine(castleCost));
				buttons.push_back({"Castle", ButtonAction::UpgradeCastle});
			}

			if (vertex) {
				if (auto tilesOpt = map.getVertexTiles(vertex); tilesOpt) {
					for (const auto& tile : *tilesOpt) {
						if (!tile) {
							continue;
						}
						if (tile->hasBuilding()) {
							continue;
						}
						const auto type = tile->getType();
						switch (type) {
						case types::TileType::FOREST:
							textLines.emplace_back("Lumber Camp: " + formatCostLine(lumberCampCost));
							buttons.push_back({"Lumber Camp", ButtonAction::BuildProductivity, type, tile->getId()});
							break;
						case types::TileType::MOUNTAIN:
							textLines.emplace_back("Stone Quarry: " + formatCostLine(stoneQuarryCost));
							buttons.push_back({"Stone Quarry", ButtonAction::BuildProductivity, type, tile->getId()});
							break;
						case types::TileType::GRASS:
							textLines.emplace_back("Stable: " + formatCostLine(stableCost));
							buttons.push_back({"Stable", ButtonAction::BuildProductivity, type, tile->getId()});
							break;
						case types::TileType::FIELD:
							textLines.emplace_back("Mill: " + formatCostLine(millCost));
							buttons.push_back({"Mill", ButtonAction::BuildProductivity, type, tile->getId()});
							break;
						case types::TileType::CLAY:
							textLines.emplace_back("Brick Kiln: " + formatCostLine(brickKilnCost));
							buttons.push_back({"Brick Kiln", ButtonAction::BuildProductivity, type, tile->getId()});
							break;
						default:
							break;
						}
					}
				}
			}
		}

		buttons.push_back({"Close", ButtonAction::Close});

		rebuildLayout();
	}

	void RenderSettlementMenuSystem::close() noexcept {
		active = false;
		selectedSettlementId = SIZE_MAX;
	}

	bool RenderSettlementMenuSystem::isPointInsideMenu(glm::vec2 mouse) const noexcept {
		if (!active) {
			return false;
		}
		return mouse.x >= boxPos.x && mouse.x <= boxPos.x + boxSize.x &&
			   mouse.y >= boxPos.y && mouse.y <= boxPos.y + boxSize.y;
	}

	bool RenderSettlementMenuSystem::onMouseButton(glm::vec2 mouse, int button, int action) noexcept {
		if (!active || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
			return false;
		}

		for (const auto& btn : buttons) {
			if (mouse.x >= btn.x && mouse.x <= btn.x + btn.w &&
				mouse.y >= btn.y && mouse.y <= btn.y + btn.h) {
				const size_t playerId = gameState ? gameState->getCurrentPlayerId() : 0;
				bool success = false;
				switch (btn.action) {
				case ButtonAction::UpgradeStone: {
					const std::vector<int> cost = {0, 0, 10, 10, 20, 10, 10, 0};
					success = gameController->upgradeSettlement(playerId, selectedSettlementId, types::SettlementType::STONE, cost);
					break;
				}
				case ButtonAction::UpgradeCastle: {
					const std::vector<int> cost = {0, 0, 30, 20, 50, 40, 30, 0};
					success = gameController->upgradeSettlement(playerId, selectedSettlementId, types::SettlementType::CASTLE, cost);
					break;
				}
				case ButtonAction::BuildProductivity: {
					std::vector<int> cost(static_cast<size_t>(types::TileType::COUNT), 0);
					switch (btn.tileType) {
					case types::TileType::FOREST:
						cost[static_cast<size_t>(types::TileType::FOREST)] = 10;
						cost[static_cast<size_t>(types::TileType::MOUNTAIN)] = 30;
						cost[static_cast<size_t>(types::TileType::CLAY)] = 20;
						break;
					case types::TileType::MOUNTAIN:
						cost[static_cast<size_t>(types::TileType::FOREST)] = 30;
						cost[static_cast<size_t>(types::TileType::MOUNTAIN)] = 10;
						cost[static_cast<size_t>(types::TileType::CLAY)] = 20;
						break;
					case types::TileType::GRASS:
						cost[static_cast<size_t>(types::TileType::FOREST)] = 30;
						cost[static_cast<size_t>(types::TileType::CLAY)] = 20;
						cost[static_cast<size_t>(types::TileType::FIELD)] = 10;
						break;
					case types::TileType::FIELD:
						cost[static_cast<size_t>(types::TileType::FOREST)] = 20;
						cost[static_cast<size_t>(types::TileType::MOUNTAIN)] = 20;
						cost[static_cast<size_t>(types::TileType::CLAY)] = 20;
						break;
					case types::TileType::CLAY:
						cost[static_cast<size_t>(types::TileType::FOREST)] = 20;
						cost[static_cast<size_t>(types::TileType::MOUNTAIN)] = 30;
						cost[static_cast<size_t>(types::TileType::GRASS)] = 10;
						break;
					default:
						break;
					}
					success = gameController->buildProductivityBuilding(playerId, btn.tileId, btn.tileType, cost);
					break;
				}
				case ButtonAction::Close:
					close();
					return true;
				}
				if (success) {
					showMenu(selectedSettlementId);
				}
				return true;
			}
		}

		return isPointInsideMenu(mouse);
	}

	void RenderSettlementMenuSystem::onKeyCallback(int key, int action) noexcept {
		if (!active) {
			return;
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
			close();
		}
	}

	void RenderSettlementMenuSystem::onResizeCallback(GLFWwindow*, int width, int height) noexcept {
		viewport.size = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)};

		if (active) {
			rebuildLayout();
		}
	}

	void RenderSettlementMenuSystem::step(float /*dt*/) noexcept {
		if (!active) {
			return;
		}

		float time = static_cast<float>(glfwGetTime());
		size_t hoveredTileId = SIZE_MAX;
		bool hasMouse = false;
		glm::vec2 mouse{};
		if (window) {
			glm::vec2 cursor = window->getCursorPosition();
			mouse = {
				cursor.x,
				static_cast<float>(viewport.size.y) - cursor.y};
			hasMouse = true;

			for (const auto& btn : buttons) {
				if (btn.action != ButtonAction::BuildProductivity) {
					continue;
				}
				if (mouse.x >= btn.x && mouse.x <= btn.x + btn.w &&
					mouse.y >= btn.y && mouse.y <= btn.y + btn.h) {
					hoveredTileId = btn.tileId;
					break;
				}
			}
		}

		if (hoveredTileId != SIZE_MAX) {
			renderHoveredTileHighlight(hoveredTileId, time);
		}

		auto* textSystem = registry->getSystem<RenderTextSystem>();
		if (!textSystem) {
			return;
		}

		drawSprite(menuBackgroundTexture, boxPos, boxSize, {1.f, 1.f, 1.f});

		glm::vec2 titleSize = textSystem->measureText(title, scale * 1.2f);
		const float topInset = paddingY * 0.8f;
		glm::vec2 titlePos{
			boxPos.x + (boxSize.x - titleSize.x) / 2.0f,
			boxPos.y + boxSize.y - titleSize.y - paddingY - topInset + titleSize.y * 0.40f};
		textSystem->renderText(title, titlePos, scale * 1.2f, {0.f, 0.f, 0.f});

	for (size_t i = 0; i < displayLines.size(); ++i) {
			if (i >= textPositions.size()) {
				break;
			}
		textSystem->renderText(displayLines[i], textPositions[i], scale, {0.f, 0.f, 0.f});
		}

	if (separatorY > 0.0f) {
		const float separatorHeight = std::max(1.0f, 1.5f * scale);
		renderBox({boxPos.x + paddingX, separatorY}, {boxSize.x - paddingX * 2.f, separatorHeight}, {0.25f, 0.25f, 0.25f});
	}

		for (const Button& btn : buttons) {
			bool hovered = hasMouse &&
				mouse.x >= btn.x && mouse.x <= btn.x + btn.w &&
				mouse.y >= btn.y && mouse.y <= btn.y + btn.h;
			drawSprite(menuButtonTexture, {btn.x, btn.y}, {btn.w, btn.h}, hovered ? glm::vec3(1.3f) : glm::vec3(1.f));
			glm::vec2 textSize = textSystem->measureText(btn.text, scale);
			glm::vec2 textPos{
				btn.x + (btn.w - textSize.x) / 2.0f,
				btn.y + (btn.h - textSize.y) / 2.0f + textSize.y * 0.15f};
			textSystem->renderText(btn.text, textPos, scale, {0.f, 0.f, 0.f});
		}
	}

	void RenderSettlementMenuSystem::rebuildLayout() {
		auto* textSystem = registry->getSystem<RenderTextSystem>();
		if (!textSystem) {
			return;
		}

		float scaleX = viewport.size.x / DEFAULT_WIDTH;
		float scaleY = viewport.size.y / DEFAULT_HEIGHT;
		scale = std::min(scaleX, scaleY) * 0.9f;
		paddingX = 28.f * scale;
		paddingY = 24.f * scale;
		const float outerMargin = 36.f * scale;

	glm::vec2 titleSize = textSystem->measureText(title, scale * 1.2f);
		const float topInset = paddingY * 1.2f;

	const float maxBoxWidth = viewport.size.x * 0.5f;
	boxSize.x = std::max(1.0f, maxBoxWidth - outerMargin * 2.f);

	const float maxContentWidth = std::max(1.0f, boxSize.x - paddingX * 2.f);
	displayLines.clear();

	for (const auto& line : textLines) {
		if (line.empty()) {
			displayLines.push_back("");
			continue;
		}

		std::istringstream iss(line);
		std::string word;
		std::string current;

		while (iss >> word) {
			std::string candidate = current.empty() ? word : current + " " + word;
			float candidateWidth = textSystem->measureText(candidate, scale).x;
			if (candidateWidth <= maxContentWidth || current.empty()) {
				current = candidate;
			} else {
				displayLines.push_back(current);
				current = word;
			}
		}
		if (!current.empty()) {
			displayLines.push_back(current);
		}
	}

	const float lineHeight = textSystem->measureText("Ay", scale).y;
		const float lineSpacing = lineHeight * 0.35f;
		const float buttonHeight = 56.f * scale;
		const float buttonSpacing = paddingY * 0.35f;
		const float titleContentGap = lineHeight * 1.1f;

	float contentHeight = 0.f;
	for (size_t i = 0; i < displayLines.size(); ++i) {
			contentHeight += lineHeight;
		if (i + 1 < displayLines.size()) {
				contentHeight += lineSpacing;
			}
		}

		float buttonsHeight = 0.f;
		if (!buttons.empty()) {
			buttonsHeight = buttons.size() * buttonHeight + (buttons.size() - 1) * buttonSpacing;
		}

	const float hudHeight = viewport.size.y * 0.10f;
	boxSize.y = std::max(1.0f, viewport.size.y - hudHeight - outerMargin * 2.f);

	boxPos.x = viewport.size.x - boxSize.x - outerMargin;
	boxPos.y = hudHeight + outerMargin;

	textPositions.clear();
	separatorY = -1.0f;
	float yCursor = boxPos.y + boxSize.y - paddingY - titleSize.y - paddingY - topInset - titleContentGap;
	for (const auto& line : displayLines) {
		float lineWidth = textSystem->measureText(line, scale).x;
		float lineX = boxPos.x + (boxSize.x - lineWidth) / 2.0f;
		textPositions.push_back({lineX, yCursor});
		if (line.empty() && separatorY < 0.0f) {
			separatorY = yCursor + lineHeight * 0.4f;
		}
			yCursor -= lineHeight + lineSpacing;
		}

		float buttonWidth = boxSize.x - paddingX * 2.f;
		float buttonY = boxPos.y + paddingY + buttonsHeight - buttonHeight;
		for (auto& btn : buttons) {
			btn.x = boxPos.x + paddingX;
			btn.y = buttonY;
			btn.w = buttonWidth;
			btn.h = buttonHeight;
			buttonY -= buttonHeight + buttonSpacing;
		}
	}

	void RenderSettlementMenuSystem::renderBox(glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {
		glm::mat4 projection = glm::ortho(
			0.f, static_cast<float>(viewport.size.x), 0.f, static_cast<float>(viewport.size.y), -1.f, 1.f
		);

		glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.f));
		model = glm::scale(model, glm::vec3(size, 1.f));

		rectShader.use()
			.setMat4("projection", projection)
			.setMat4("view", glm::identity<glm::mat4>())
			.setMat4("model", model)
			.setVec3("fcolor", color);

		glBindVertexArray(quadVao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	void RenderSettlementMenuSystem::drawSprite(Texture& tex, glm::vec2 pos, glm::vec2 size, glm::vec3 color) const noexcept {
		const float width = viewport.size.x;
		const float height = viewport.size.y;

		glm::mat4 projection = glm::ortho(0.f, width, 0.f, height, -1.f, 1.f);
		glm::mat4 view = glm::identity<glm::mat4>();
		glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.f));
		model = glm::scale(model, glm::vec3(size, 1.f));

		tex.bind(0);
		textureShader.use()
			.setMat4("projection", projection)
			.setMat4("view", view)
			.setMat4("model", model)
			.setSampler("sprite", 0)
			.setVec3("fcolor", color);

		glBindVertexArray(textureVao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	void RenderSettlementMenuSystem::renderHoveredTileHighlight(size_t tileId, float time) const noexcept {
		if (!registry || !gameState || highlightVao == 0) {
			return;
		}

		const Graph& map = gameState->getMap();
		const TileHandle tile = map.getTile(tileId);
		if (!tile) {
			return;
		}

		Camera& cam = registry->cameras.get(registry->getCamera());
		const glm::mat4 view = glm::identity<glm::mat4>();
		const glm::mat4 projection = calculateProjection(cam);

		const uint32_t columns = map.getMapWidth();
		uint32_t row = static_cast<uint32_t>(tileId / columns);
		uint32_t col = static_cast<uint32_t>(tileId % columns);
		glm::vec2 tilePos = WorldNodeMapper::getTilePosition(row, col);

		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::translate(model, glm::vec3(tilePos, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

		const glm::vec3 highlightColor = glm::vec3(0.53f, 0.73f, 0.57f);
		locationHighlightShader.use()
			.setMat4("view", view)
			.setMat4("projection", projection)
			.setMat4("model[0]", model)
			.setVec3("highlightColor", highlightColor)
			.setFloat("alpha", 0.55f)
			.setFloat("time", time)
			.setFloat("pulseStrength", 0.4f);

		glBindVertexArray(highlightVao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
	}

	const glm::mat4 RenderSettlementMenuSystem::calculateProjection(const Camera& cam) const {
		glm::uvec2 extent = window->getWindowExtent();
		glViewport(0, 0, extent.x, extent.y);

		return glm::ortho(
			cam.minX(), cam.maxX(),
			cam.minY(), cam.maxY(),
			-1.0f, 1.0f);
	}

	std::string RenderSettlementMenuSystem::formatCostLine(const std::vector<int>& cost) const {
		std::string line;
		auto addCost = [&](types::TileType type, const char* name) {
			const size_t idx = static_cast<size_t>(type);
			if (idx >= cost.size() || cost[idx] <= 0) {
				return;
			}
			if (!line.empty()) {
				line += ", ";
			}
			line += std::to_string(cost[idx]) + " " + name;
		};

		addCost(types::TileType::FOREST, "Wood");
		addCost(types::TileType::MOUNTAIN, "Stone");
		addCost(types::TileType::CLAY, "Clay");
		addCost(types::TileType::FIELD, "Grain");
		addCost(types::TileType::GRASS, "Wool");

		return line;
	}

	int RenderSettlementMenuSystem::getPotencyPercent(types::TilePotency potency) const {
		switch (potency) {
		case types::TilePotency::LOW:
			return 20;
		case types::TilePotency::MEDIUMLOW:
			return 35;
		case types::TilePotency::MEDIUM:
			return 50;
		case types::TilePotency::MEDIUMHIGH:
			return 70;
		case types::TilePotency::HIGH:
			return 90;
		default:
			return 0;
		}
	}

} // namespace df


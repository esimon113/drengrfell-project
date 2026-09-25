#pragma once

#include "registry.h"
#include <filesystem>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "graph.h"
#include "player.h"
#include "productivityBuilding.h"
#include "road.h"
#include "settlement.h"
#include "constructionCosts.h"
#include "tutorial.h"
#include "types.h"
#include "worldGeneratorConfig.h"
#include <functional>





namespace df {


	// game state information storage -> used for saving/loading and (potentially)
	// multiplayer syncing
	class GameState {
	  public:
		GameState() = default;
		GameState(Registry* reg)
			: registry(reg),
			  roadCosts(roadPlacementCost()),
			  settlementCosts(settlementPlacementCost()) {}


		Graph& getMap() { return this->map; }
		const Graph& getMap() const { return this->map; }
		void setMap(Graph newMap) { this->map = std::move(newMap); }


		// players
		size_t getPlayerCount() const { return this->players.size(); }
		Player* getPlayer(size_t playerId);
		const Player* getPlayer(size_t playerId) const;
		std::vector<Player>& getPlayers() { return this->players; }
		const std::vector<Player>& getPlayers() const { return this->players; }
		void addPlayer(const Player& player) { this->players.push_back(player); }
		void clearPlayers() { this->players.clear(); }


		// settlements
		std::vector<std::shared_ptr<Settlement>> getSettlements();
		void addSettlement(std::shared_ptr<Settlement> settlement);
		void clearSettlements() {
			settlements.clear();
			if (registry) {
				registry->settlements.clear();
			}
		}
		const std::vector<int>& getCurrentSettlementCost() const;


		// roads
		std::vector<std::shared_ptr<Road>> getRoads();
		void addRoad(std::shared_ptr<Road> road);
		void clearRoads() {
			roads.clear();
			if (registry) {
				registry->roads.clear();
			}
		}
		const std::vector<int>& getCurrentRoadCost() const;

		// productivity buildings
		std::vector<std::shared_ptr<ProductivityBuilding>> getProductivityBuildings();
		void addProductivityBuilding(std::shared_ptr<ProductivityBuilding> building);
		void clearProductivityBuildings() {
			productivityBuildings.clear();
			if (registry) {
				registry->productivityBuildings.clear();
			}
		}


		// turns
		size_t getCurrentPlayerId() const { return this->currentPlayerId; }
		void setCurrentPlayerId(size_t playerId) { this->currentPlayerId = playerId; }

		size_t getTurnCount() const { return this->turnCount; }
		void setTurnCount(size_t count) { this->turnCount = count; }

		size_t getRoundNumber() const { return this->roundNumber; }
		void setRoundNumber(size_t round) { this->roundNumber = round; }

		types::GamePhase getPhase() const { return this->phase; }
		void setPhase(types::GamePhase newPhase) {
			this->phase = newPhase;
			fmt::println("Switched to game phase {}", (int)this->phase);
		}


		// persistence
		json serialize() const;
		json serializeFor(size_t viewerPlayerId) const;
		void deserialize(const json& j);
		void applyAuthoritativeSnapshot(const json& j);

		void setWorldConfig(const WorldGeneratorConfig& config) { worldConfig = config; }
		const WorldGeneratorConfig& getWorldConfig() const { return worldConfig; }
		void syncSettlementType(size_t settlementId, types::SettlementType type);
		void save(const std::filesystem::path& filepath) const;
		void load(const std::filesystem::path& filepath);

		// Tutorial
		void initTutorial();
		void resetTutorial();
		TutorialStep* getCurrentTutorialStep();
		void completeCurrentTutorialStep();
		void completeTutorialStep(TutorialStepId id);
		void setTutorialReporter(std::function<void(TutorialStepId)> reporter) { tutorialReporter = std::move(reporter); }
		bool isTutorialActive() const;
		bool hasAuthoritativeMap() const { return authoritativeMap; }
		size_t getViewerPlayerId() const { return viewerPlayerId; }
		void setViewerPlayerId(size_t id) { viewerPlayerId = id; }
		types::WeatherType getWeather() const { return weather; }
		void setWeather(types::WeatherType type) { weather = type; }
		float getWeatherIntensity() const { return weatherIntensity; }
		void setWeatherIntensity(float intensity) { weatherIntensity = intensity; }
		std::vector<glm::vec3> computeHudResourceColor(std::string mode);
		bool isGameOver() const;

	  private:
		// TODO: discuss ownership model for game...
		Graph map;

		std::vector<Player> players;

		// Smart pointer storage for safe ownership
		std::vector<std::shared_ptr<Settlement>> settlements;
		std::vector<std::shared_ptr<Road>> roads;
		std::vector<std::shared_ptr<ProductivityBuilding>> productivityBuildings;

		// turns
		size_t currentPlayerId = 0;
		size_t turnCount = 0;
		size_t roundNumber = 0;
		types::GamePhase phase = types::GamePhase::START;
		Registry* registry = nullptr;
		// Tutorial
		std::vector<TutorialStep> tutorialSteps;
		size_t currentTutorialStep = 0;
		std::function<void(TutorialStepId)> tutorialReporter;
		size_t tutorialReportSentFor = static_cast<size_t>(-1);
		bool authoritativeMap = false;
		size_t viewerPlayerId = 0;
		types::WeatherType weather = types::WeatherType::SUNNY;
		float weatherIntensity = 0.f;

		std::vector<int> roadCosts;
		std::vector<int> settlementCosts;
		WorldGeneratorConfig worldConfig;

		bool isTileVisibleTo(size_t playerId, size_t tileId) const;
		bool isVertexVisibleTo(size_t playerId, size_t vertexId) const;
		bool isEdgeVisibleTo(size_t playerId, size_t edgeId) const;
	};

} // namespace df

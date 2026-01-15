#include "tradingSystem.h"
#include "player.h"
#include "renderNotification.h"

namespace df {
	void TradingSystem::init(RenderNotificationSystem* notif, Player* player) {
		notificationSystem = notif;
		currentPlayer = player;
	}

	void TradingSystem::startTrading() {
		if (!notificationSystem || !currentPlayer)
			return;

		isTradingActive = true;
		selectedResource.clear(); // Phase 1

		showBuyResourcePopup();
	}

	void TradingSystem::showBuyResourcePopup() {
		notificationSystem->showNotification(
			"Which resource do you want to trade?",
			"Select a resource:",
			allResources);
	}

	void TradingSystem::showPayResourcePopup() {
		std::vector<std::string> payOptions;
		for (const auto& res : allResources) {
			if (res != selectedResource) {
				payOptions.push_back(res);
			}
		}

		notificationSystem->showNotification(
			fmt::format("Buying {}", selectedResource),
			"Which resource do you want to pay with?",
			payOptions);
		}

	void TradingSystem::handleOptionClicked(const std::string& resource) {
			if (!isTradingActive || !currentPlayer)
				return;

			if (selectedResource.empty()) {
				selectedResource = resource;
				showPayResourcePopup();
			} else {
				executeTrade(resource);
				isTradingActive = false;
				selectedResource.clear();
			}
		}


	void TradingSystem::executeTrade(const std::string& payResource) {
			bool success = false;

			if (payResource == "Wood") {
				success = currentPlayer->removeResource(types::TileType::FOREST, payAmount);
			} else if (payResource == "Clay") {
				success = currentPlayer->removeResource(types::TileType::CLAY, payAmount);
			} else if (payResource == "Stone") {
				success = currentPlayer->removeResource(types::TileType::MOUNTAIN, payAmount);
			} else if (payResource == "Grain") {
				success = currentPlayer->removeResource(types::TileType::FIELD, payAmount);
			} else if (payResource == "Cattle") {
				success = currentPlayer->removeResource(types::TileType::GRASS, payAmount);
			}

			if (!success) {
				fmt::println("Not enough {} to trade!", payResource);
				return;
			}

			if (selectedResource == "Wood")
				currentPlayer->addResource(types::TileType::FOREST, gainAmount);
			if (selectedResource == "Clay")
				currentPlayer->addResource(types::TileType::CLAY, gainAmount);
			if (selectedResource == "Stone")
				currentPlayer->addResource(types::TileType::MOUNTAIN, gainAmount);
			if (selectedResource == "Grain")
				currentPlayer->addResource(types::TileType::FIELD, gainAmount);
			if (selectedResource == "Cattle")
				currentPlayer->addResource(types::TileType::GRASS, gainAmount);

			fmt::println("Traded {} {} for {} {}", payAmount, payResource, gainAmount, selectedResource);
		}

	}

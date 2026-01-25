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
		selectedResource.clear();

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
			fmt::format(
				"Which resource do you want to pay with?\n"
				"Pay {} to receive {} {}.",
				payAmount, gainAmount, selectedResource),
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
			if (currentPlayer->getResources(types::TileType::FOREST) >= payAmount) {
				currentPlayer->removeResources(types::TileType::FOREST, payAmount);
				success = true;
			}
		} else if (payResource == "Stone") {
			if (currentPlayer->getResources(types::TileType::MOUNTAIN) >= payAmount) {
				currentPlayer->removeResources(types::TileType::MOUNTAIN, payAmount);
				success = true;
			}
		} else if (payResource == "Clay") {
			if (currentPlayer->getResources(types::TileType::CLAY) >= payAmount) {
				currentPlayer->removeResources(types::TileType::CLAY, payAmount);
				success = true;
			}
		} else if (payResource == "Wool") {
			if (currentPlayer->getResources(types::TileType::GRASS) >= payAmount) {
				currentPlayer->removeResources(types::TileType::GRASS, payAmount);
				success = true;
			}
		} else if (payResource == "Grain") {
			if (currentPlayer->getResources(types::TileType::FIELD) >= payAmount) {
				currentPlayer->removeResources(types::TileType::FIELD, payAmount);
				success = true;
			}
		}

		if (!success) {
			fmt::println("Not enough {} to trade!", payResource);
			selectedResource.clear();
			showBuyResourcePopup();
			return;
		}

		if (selectedResource == "Wood")
			currentPlayer->addResources(types::TileType::FOREST, gainAmount);
		else if (selectedResource == "Stone")
			currentPlayer->addResources(types::TileType::MOUNTAIN, gainAmount);
		else if (selectedResource == "Clay")
			currentPlayer->addResources(types::TileType::CLAY, gainAmount);
		else if (selectedResource == "Wool")
			currentPlayer->addResources(types::TileType::GRASS, gainAmount);
		else if (selectedResource == "Grain")
			currentPlayer->addResources(types::TileType::FIELD, gainAmount);

		fmt::println("Traded {} {} for {} {}", payAmount, payResource, gainAmount, selectedResource);
	}

} // namespace df

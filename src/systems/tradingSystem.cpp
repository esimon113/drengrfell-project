#include "tradingSystem.h"
#include "constructionCosts.h"
#include "renderNotification.h"

#include <utility>

namespace df {
	void TradingSystem::init(RenderNotificationSystem* notif, TradeCallback callback) {
		notificationSystem = notif;
		onTrade = std::move(callback);
	}

	void TradingSystem::startTrading() {
		if (!notificationSystem || !onTrade)
			return;

		isTradingActive = true;
		selectedResource.clear();

		showBuyResourcePopup();
	}

	void TradingSystem::showBuyResourcePopup() {
		notificationSystem->showNotification(
			"Trade",
			"Which resource do you want to buy? Select a resource:",
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
				BANK_TRADE_GIVE, BANK_TRADE_RECEIVE, selectedResource),
			payOptions);
	}

	void TradingSystem::handleOptionClicked(const std::string& resource) {
		if (!isTradingActive || !onTrade)
			return;

		if (selectedResource.empty()) {
			selectedResource = resource;
			showPayResourcePopup();
			return;
		}

		const auto give = resourceType(resource);
		const auto receive = resourceType(selectedResource);
		isTradingActive = false;
		selectedResource.clear();
		if (give && receive) {
			onTrade(*give, *receive);
		}
	}

	std::optional<types::TileType> TradingSystem::resourceType(const std::string& resource) {
		if (resource == "Wood")
			return types::TileType::FOREST;
		if (resource == "Stone")
			return types::TileType::MOUNTAIN;
		if (resource == "Clay")
			return types::TileType::CLAY;
		if (resource == "Wool")
			return types::TileType::GRASS;
		if (resource == "Grain")
			return types::TileType::FIELD;
		return std::nullopt;
	}

} // namespace df

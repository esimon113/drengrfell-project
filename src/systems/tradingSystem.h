#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "renderNotification.h"
#include "types.h"

namespace df {

	class TradingSystem {
	  public:
		using TradeCallback = std::function<void(types::TileType give, types::TileType receive)>;

		TradingSystem() = default;

		void init(RenderNotificationSystem* notif, TradeCallback onTrade);
		bool getIsTradingActive() const { return isTradingActive; }

		void startTrading();
		void showBuyResourcePopup();
		void showPayResourcePopup();

		void handleOptionClicked(const std::string& resource);

	  private:
		static std::optional<types::TileType> resourceType(const std::string& resource);

		RenderNotificationSystem* notificationSystem = nullptr;
		TradeCallback onTrade;

		bool isTradingActive = false;
		std::string selectedResource;

		std::vector<std::string> allResources = {
			"Wood", "Stone", "Clay", "Wool", "Grain", "Cancel"};
	};
} // namespace df

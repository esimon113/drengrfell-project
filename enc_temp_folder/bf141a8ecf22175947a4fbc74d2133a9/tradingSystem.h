#pragma once

#include <string>
#include <vector>

#include "player.h"
#include "renderNotification.h"

namespace df {

	class TradingSystem {
	  public:
		TradingSystem() = default;

		void init(RenderNotificationSystem* notif, Player* player);
		bool getIsTradingActive() const { return isTradingActive; }

		void startTrading();
		void showBuyResourcePopup();
		void showPayResourcePopup();

		void handleOptionClicked(const std::string& resource);
		void executeTrade(const std::string& payResource);

	  private:
		RenderNotificationSystem* notificationSystem;
		Player* currentPlayer;

		const int payAmount = 5;
		const int gainAmount = 3;

		bool isTradingActive = false;
		std::string selectedResource;

		std::vector<std::string> allResources = {
			"Wood", "Stone", "Clay", "Wool", "Grain", "Cancle"};
	};
} // namespace df

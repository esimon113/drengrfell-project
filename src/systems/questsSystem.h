#pragma once

#include "../core/quest.h"
#include "renderNotification.h"
#include <map>
#include <nlohmann/json.hpp>
#include <string>

namespace df {

    class QuestsSystem {
    public:
        QuestsSystem() = default;
        ~QuestsSystem() = default;

        void init(RenderNotificationSystem* notificationSys);
        void reset();
        void bindPlayer(size_t playerId);
        void updateProgress(size_t playerId, types::QuestGoalType type, int amount);
        void activateQuest(int questId, Player* player, GameState* gameState);
        void claimQuest(int questId, Player* player, GameState* gameState);
        bool prepareClaim(size_t playerId, int questId);

        void notifyPlayer(int questId);
        void notifyNextActiveQuest(Player* player, GameState* gameState = nullptr);

        const Quest* getQuestById(int id) const;
        const std::vector<Quest>& getQuests() const { return m_quests; }
        int getCurrentShowingQuestId() const { return m_currentShowingQuestId; }
        
        void setCurrentQuest() { m_currentShowingQuestId = -1; currentQuest = 1; }

        void loadQuests(const std::string& path);
        [[nodiscard]] nlohmann::json serialize() const;
        [[nodiscard]] nlohmann::json serializeFor(size_t playerId) const;
        void applyAuthoritative(const nlohmann::json& quests);


    private:
        std::vector<Quest> m_quests;
        std::vector<Quest> questTemplate;
        std::map<size_t, std::vector<Quest>> questsByPlayer;
        size_t displayedPlayerId{0};
        RenderNotificationSystem* m_notificationSystem = nullptr;
        
        int m_currentShowingQuestId = -1;
        int activeQuests = 1;
        int currentQuest = 1;
    };

}
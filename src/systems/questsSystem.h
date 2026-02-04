#pragma once

#include "../core/quest.h"
#include "renderNotification.h"
#include <string>

namespace df {

    class QuestsSystem {
    public:
        QuestsSystem() = default;
        ~QuestsSystem() = default;

        void init(RenderNotificationSystem* notificationSys);
        void reset();
        void updateProgress(types::QuestGoalType type, int amount);
        void activateQuest(int questId, Player* player, GameState* gameState);
        void claimQuest(int questId, Player* player, GameState* gameState);

        void notifyPlayer(int questId);
        void notifyNextActiveQuest(Player* player);

        const Quest* getQuestById(int id) const;
        const std::vector<Quest>& getQuests() const { return m_quests; }
        int getCurrentShowingQuestId() const { return m_currentShowingQuestId; }
        
        void setCurrentQuest() { m_currentShowingQuestId = -1; currentQuest = 1; }

        void loadQuests(const std::string& path);


    private:
        std::vector<Quest> m_quests;
        RenderNotificationSystem* m_notificationSystem = nullptr;
        
        int m_currentShowingQuestId = -1;
        int activeQuests = 1;
        int currentQuest = 1;
    };

}
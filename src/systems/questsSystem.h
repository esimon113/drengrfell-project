#pragma once

#include "renderNotification.h"
#include <string>
#include <vector>
#include <map>

namespace df {

    enum class QuestState {
        Locked,
        Available,
        Active,
        Completed,
        Claimed
    };

    struct Quest {
        int id;
        std::string name;
        std::string goal_type; 
        int goal_amount; 
        int progress; 
        QuestState state ;
        int unlocksId;
    };

    class QuestsSystem {
        
        public:
            QuestsSystem() : m_notificationSystem(nullptr) {}
            ~QuestsSystem() = default;

            void init(RenderNotificationSystem* notificationSys);

            void updateProgress(const std::string& type, int amount);
            void notifyPlayer(int questId);
            void addQuest(const Quest& newQuest);
            void claimQuest(int questId);
            void activateQuest(int );

            void notifyNextActiveQuest();

            int getCurrentShowingQuestId() const { return m_currentShowingQuestId; }
            const std::vector<Quest> getQuests () const { return m_quests;}

            void loadQuests(const std::string& path);
            

        private:
            std::vector<Quest> m_quests;
            RenderNotificationSystem* m_notificationSystem = nullptr;
            int m_currentShowingQuestId = -1;
    };

}
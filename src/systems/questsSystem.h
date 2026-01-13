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
        std::string desc;
        std::string goal_type; 
        int goal_amount; 
        int progress; 
        int unlocksId;
        QuestState state ;
        
        Quest(int _id, std::string _name, std::string _desc,std::string _type, int _amount, int _prog, int _unlock, QuestState _state)
        : id(_id), 
          name(_name), 
          desc(_desc),
          goal_type(_type), 
          goal_amount(_amount), 
          progress(_prog), 
          unlocksId(_unlock), 
          state(_state) 
        {}
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

            //void loadQuests(const std::string& path);
            

        private:
            std::vector<Quest> m_quests;
            RenderNotificationSystem* m_notificationSystem = nullptr;
            int m_currentShowingQuestId = -1;
            //std::string& m_path;
    };

}
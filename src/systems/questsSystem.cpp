#include <iostream>
#include "questsSystem.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace df {

    //  TODO: In a future add the functions to update the json to reload a game

    void QuestsSystem::init(RenderNotificationSystem* notificationSys) {
        m_notificationSystem = notificationSys;

        m_quests.clear();

        // ID | Name | Description | Quest type (resources, building...) | Quantity | Initial progress | unblock id | Reward type | Reward | Initial state
        
        m_quests.push_back({0, "Apprentice", "Complete the tutorial", "tutorial", 1, 0, {1,3}, types::TileType::FOREST, 5, QuestState::Active});
        // Building quests
        m_quests.push_back({1, "Builder", "Have 3 settlements","settlement", 3, 1, {2}, types::TileType::CLAY,10, QuestState::Locked});
        m_quests.push_back({2, "The King's Highway", "Build 2 new roads", "road", 2 , 0, {-1}, types::TileType::MOUNTAIN,5,QuestState::Locked});
        
        // Resources quests
        m_quests.push_back({3, "Lumberjack", "Collect 10 wood" ,"forest", 10, 0, {4}, types::TileType::FIELD,10, QuestState::Locked});

        // Surcvival quests1, 
        m_quests.push_back({4, "Professional Survivor", "Survive 5 rounds", "rounds", 5, 0, {-1}, types::TileType::GRASS, 10, QuestState::Locked});


        //loadQuests("../assets/jsons/quests.json");


    }


    /*

    JSON functions

    
    void QuestsSystem::loadQuests(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json data = json::parse(file);
        m_quests.clear(); 

        for (const auto& item : data) {
            Quest q;
            q.id = item.value("id", -1);
            q.name = item.value("name", "Misión");
            q.goal_type = item.value("goal_type", "");
            q.goal_amount = item.value("goal_amount", 0);
            q.progress = 0;
            q.unlocksId = item.value("unlocks_id", -1);
            
            std::string stateStr = item.value("initial_state", "Locked");
            q.state = (stateStr == "Active") ? QuestState::Active : QuestState::Locked;

            m_quests.push_back(q); 
        }
    }
    */

    const Quest* QuestsSystem::getQuestById(int id) const {
        for (const auto& q : m_quests) {
            if (q.id == id) return &q;
        }
        return nullptr;
    }


    void QuestsSystem::updateProgress(const std::string& type, int amount) {
        for (auto& quest : m_quests) {
            if (quest.state == QuestState::Active && quest.goal_type == type) {
                
                quest.progress += amount;

                if (quest.progress >= quest.goal_amount) {
                    quest.state = QuestState::Completed;
                    notifyPlayer(quest.id); 
                }
            }
        }
    }

    void QuestsSystem::notifyPlayer(int questId) {
        for (auto& q : m_quests) {
            if (q.id == questId) {
                m_currentShowingQuestId = questId;
                std::vector<std::string> buttons;
                
                if (q.state == QuestState::Completed) {
                    buttons = {"Claim", "Next Quest"};
                } else {
                    buttons = {"Close", "Next Quest"};
                }

                if (q.state == QuestState::Active) {
                    m_notificationSystem->showNotification(q.name, q.desc, buttons);
                } else {
                    m_notificationSystem->showNotification(q.name, "Quest completed", buttons);
                }
                break;
            }
        }
    }


    void QuestsSystem::claimQuest(int questId) {
        for (auto& q : m_quests) {
            if (q.id == questId && q.state == QuestState::Completed) {
                q.state = QuestState::Claimed;

                for (int nextId : q.unlocksIds) {
                    activateQuest(nextId);
                }
                
                m_currentShowingQuestId = -1; 
                break;
            }
        }
    }

    void QuestsSystem::activateQuest(int questId) {
        for (auto& q : m_quests) {
            if (q.id == questId && q.state == QuestState::Locked) {
                q.state = QuestState::Active;
                notifyPlayer(q.id); 
            }
        }
    }

    void QuestsSystem::notifyNextActiveQuest() {
        if (m_quests.empty()) return;

        int currentIdx = -1;
        for (int i = 0; i < (int)m_quests.size(); ++i) {
            if (m_quests[i].id == m_currentShowingQuestId) {
                currentIdx = i;
                break;
            }
        }

        for (int i = 1; i <= (int)m_quests.size(); ++i) {
            int nextIdx = (currentIdx + i) % m_quests.size();
            auto& q = m_quests[nextIdx];

            if (q.state == QuestState::Active || q.state == QuestState::Completed) {
                notifyPlayer(q.id);
                return;
            }
        }
    }
}
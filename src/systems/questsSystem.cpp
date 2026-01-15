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

        // ID | Name | Description | Quest type (resources, building...) | Quantity | Initial progress (-1 if must be updated during gameplay) | unblock id | Reward type | Reward | Initial state
        
        m_quests.push_back({0, "Apprentice", "Complete the tutorial", types::QuestGoalType::TUTORIAL, 1, 0, {1,3}, types::TileType::FOREST, 5, QuestState::Active});
        // Building quests
        m_quests.push_back({1, "Builder", "Have 3 settlements",types::QuestGoalType::SETTLEMENT, 3, -1, {2}, types::TileType::CLAY,10, QuestState::Locked});
        m_quests.push_back({2, "The King's Highway", "Build 2 new roads",types::QuestGoalType::ROAD, 2 , 0, {-1}, types::TileType::MOUNTAIN,5,QuestState::Locked});
        
        // Resources quests
        m_quests.push_back({3, "Lumberjack", "Collect 10 wood" ,types::QuestGoalType::FOREST, 10, 0, {4}, types::TileType::FIELD,10, QuestState::Locked});

        // Surcvival quests1, 
        m_quests.push_back({4, "Professional Survivor", "Survive 5 rounds", types::QuestGoalType::ROUNDS, 5, 0, {-1}, types::TileType::GRASS, 10, QuestState::Locked});


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


    void QuestsSystem::updateProgress(types::QuestGoalType type, int amount) {
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


    void QuestsSystem::claimQuest(int questId, Player* player) {
        for (auto& q : m_quests) {
            if (q.id == questId && q.state == QuestState::Completed) {
                q.state = QuestState::Claimed;

                for (int nextId : q.unlocksIds) {
                    activateQuest(nextId, player);
                }
                
                m_currentShowingQuestId = -1; 
                break;
            }
        }
    }

    void QuestsSystem::activateQuest(int questId, Player* player) {
        for (auto& q : m_quests) {
            if (q.id == questId && q.state == QuestState::Locked) {
                q.state = QuestState::Active;
                if (q.progress == -1){
                    switch (q.goal_type) {
                        case types::QuestGoalType::SETTLEMENT:
                            q.progress = static_cast<int>(player->getSettlementIds().size());
                            break;
                        case types::QuestGoalType::ROAD:
                            q.progress = static_cast<int>(player->getRoadIds().size());
                            break;
                        case types::QuestGoalType::FOREST:
                            q.progress = player->getResources(types::TileType::FOREST);
                            break;
                        case types::QuestGoalType::CLAY:
                            q.progress = player->getResources(types::TileType::CLAY);
                            break;
                        case types::QuestGoalType::MOUNTAIN:
                            q.progress = player->getResources(types::TileType::MOUNTAIN);
                            break;
                        case types::QuestGoalType::FIELD:
                            q.progress = player->getResources(types::TileType::FIELD);
                            break;
                        case types::QuestGoalType::GRASS:
                            q.progress = player->getResources(types::TileType::GRASS);
                            break;
                        case types::QuestGoalType::WATER:
                            q.progress = player->getResources(types::TileType::WATER);
                            break;
                        case types::QuestGoalType::ICE:
                            q.progress = player->getResources(types::TileType::ICE);
                            break;

                        case types::QuestGoalType::ROUNDS:
                            break;

                        case types::QuestGoalType::TUTORIAL:
                        case types::QuestGoalType::NONE:
                        default:
                            break;
                    }
                }
                
                if (q.progress >= q.goal_amount) {
                    q.state = QuestState::Completed;
                }
                
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
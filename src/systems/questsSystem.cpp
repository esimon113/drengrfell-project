#include <iostream>
#include "questsSystem.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace df {

    void QuestsSystem::init(RenderNotificationSystem* notificationSys) {
        m_notificationSystem = notificationSys;
        loadQuests("../assets/jsons/quests.json");
    }

    void QuestsSystem::updateProgress(const std::string& type, int amount) {
        for (auto& quest : m_quests) {
            if (quest.state == QuestState::Active && quest.goal_type == type) {
                
                quest.progress = amount;

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

                if (m_notificationSystem) {
                    m_notificationSystem->showNotification(q.name, "Objetivo en curso...", buttons);
                }
                break;
            }
        }
    }

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

    void QuestsSystem::claimQuest(int questId) {
        for (auto& q : m_quests) {
            if (q.id == questId && q.state == QuestState::Completed) {
                q.state = QuestState::Claimed;
                fmt::println("Misión {} reclamada.", q.name);

                if (q.unlocksId != -1) {
                    activateQuest(q.unlocksId); 
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
                fmt::println("¡Nueva misión activada: {}!", q.name);

                notifyPlayer(q.id); 
                break;
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
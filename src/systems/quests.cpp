#include "quests.h"
#include <iostream>

namespace df {

    void QuestsSystem::updateProgress(const std::string& type, int amount) {
        for (auto& quest : m_quests) {
            if (quest.state == QuestState::Active) {
                
                if (quest.goal.count(type)) {
                    quest.progress[type] += amount;
                    
                    if (quest.progress[type] >= quest.goal[type]) {
                        quest.state = QuestState::Completed;
                        notifyPlayer(quest.id);
                    }
                }
            }
        }
    }

    void QuestsSystem::notifyPlayer(int questId) {
        std::cout << "¡Mision " << questId << " completada!" << std::endl;
    }

    void QuestsSystem::addQuest(const Quest& newQuest) {
        m_quests.push_back(newQuest);
    }
}
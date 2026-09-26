#include <iostream>
#include "questsSystem.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace df {
namespace {

    int countOpenQuests(const std::vector<Quest>& quests) {
        int count = 0;
        for (const auto& quest : quests) {
            if (quest.state == QuestState::Active || quest.state == QuestState::Completed) {
                ++count;
            }
        }
        return count;
    }

}

    //  TODO: In a future add the functions to update the json to reload a game

    void QuestsSystem::init(RenderNotificationSystem* notificationSys) {
        m_notificationSystem = notificationSys;
        activeQuests = 1;
        m_quests.clear();
        // ID | Name | Description | Quest type (resources, building...) | Quantity | Initial progress (-1 if must be updated during gameplay) | unblock id | Reward type | Reward | Initial state
        auto path = assets::getAssetPath(assets::JsonFile::QUESTS);        
        loadQuests(path);
        questTemplate = m_quests;
        questsByPlayer.clear();
    }
   
    void QuestsSystem::loadQuests(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            fmt::println("Error: Could not open quest file at {}", path);
            return;
        }
        try {
            json data = json::parse(file);
            m_quests.clear(); 

            for (const auto& item : data) {
                int id = item.value("id", -1);
                std::string name = item.value("name", "Unnamed Quest");
                std::string desc = item.value("desc", "");
                int amount = item.value("goal_amount", 0);
                int progress = item.value("progress", 0);
                int rewardAmt = item.value("reward_amount", 0);
                
                std::vector<int> unlocks = item.value("unlocks_ids", std::vector<int>{});

                types::QuestGoalType gType = df::types::stringToGoalType(item.value("goal_type", "NONE"));
                types::TileType rType = df::types::stringToTileType(item.value("reward_resource", "NONE"));
                
                std::string stateStr = item.value("initial_state", "Locked");
                QuestState state = (stateStr == "Active") ? QuestState::Active : QuestState::Locked;

                m_quests.emplace_back(id, name, desc, gType, amount, progress, unlocks, rType, rewardAmt, state);
            }
        } catch (json::parse_error& e) {
            fmt::println("JSON Parse Error: {}", e.what());
        }
    }
    

    const Quest* QuestsSystem::getQuestById(int id) const {
        for (const auto& q : m_quests) {
            if (q.id == id) return &q;
        }
        return nullptr;
    }


    void QuestsSystem::bindPlayer(size_t playerId) {
        if (!questsByPlayer.contains(playerId)) {
            questsByPlayer.emplace(playerId, questTemplate);
        }
        displayedPlayerId = playerId;
        m_quests = questsByPlayer[playerId];
        activeQuests = countOpenQuests(m_quests);
    }

    void QuestsSystem::updateProgress(size_t playerId, types::QuestGoalType type, int amount) {
        const auto it = questsByPlayer.find(playerId);
        if (it == questsByPlayer.end()) {
            return;
        }
        for (auto& quest : it->second) {
            if (quest.state == QuestState::Active && quest.goal_type == type) {
                
                quest.progress += amount;

                if (quest.progress >= quest.goal_amount) {
                    quest.state = QuestState::Completed;
                    notifyPlayer(quest.id); 
                }
            }
        }
        if (playerId == displayedPlayerId) {
            m_quests = it->second;
        }
    }

    void QuestsSystem::notifyPlayer(int questId) {
        if (!m_notificationSystem) {
            return;
        }
        if(questId == 100){
            m_notificationSystem->showNotification("CONGRATULATIONS", "You have been awarded with 5 points.\nNo more quests", {"Close"});
            return;
        }

        for (auto& q : m_quests) {
            if (q.id == questId) {
                m_currentShowingQuestId = questId;

                int visualIndex = 1;
                for(const auto& checkQ : m_quests) {
                    if (checkQ.id == q.id) break; 
                    if (checkQ.state == QuestState::Active || checkQ.state == QuestState::Completed) {
                        visualIndex++;
                    }
                }
                currentQuest = visualIndex;

                std::string title = q.name;
                std::string dynamicDesc;
                std::vector<std::string> buttons;
                
                if (q.state == QuestState::Completed) {
                    dynamicDesc = fmt::format("Quest Completed! \nYou'll be rewarded with {} {}",q.reward_amount,types::resourceName(q.reward_resource));
                    buttons = { "Claim" };
                    
                } 
                else {
                    buttons = { "Close" };
                    
                    int remaining = q.goal_amount - q.progress;
                    if (remaining < 0) remaining = 0; 

                    std::string rewardName = types::resourceName(q.reward_resource);

                    dynamicDesc = fmt::format(
                        "\n{}\n\n"
                        "Remaining: {}\n"
                        "Reward: {} units of {}\n\n"
                        "--- Quest {} of {} ---\n", 
                        q.desc, 
                        remaining, 
                        q.reward_amount,
                        rewardName,
                        currentQuest,
                        activeQuests
                    );


                    if(q.goal_type==types::QuestGoalType::TUTORIAL){
                        dynamicDesc = fmt::format(
                            "\n{}\n\n"
                            "Reward: {} units of {}\n\n"
                            "--- Quest {} of {} ---\n", 
                            q.desc,
                            q.reward_amount,
                            rewardName,
                            currentQuest,
                            activeQuests    
                        );
                        buttons = {
                            "Close"
                        };
                    }
                    
                
                    if (currentQuest < activeQuests) {
                        buttons.insert(buttons.begin(), "Next Quest");
                    } 
                }

                m_notificationSystem->showNotification(q.name, dynamicDesc, buttons);
                
                break;
            }
        }
    }


    bool QuestsSystem::prepareClaim(size_t playerId, int questId) {
        const auto it = questsByPlayer.find(playerId);
        if (it == questsByPlayer.end()) {
            return false;
        }
        for (auto& quest : it->second) {
            if (quest.id != questId) {
                continue;
            }
            if (quest.state == QuestState::Claimed || quest.state == QuestState::Locked) {
                return false;
            }
            if (quest.state != QuestState::Completed) {
                quest.state = QuestState::Completed;
            }
            displayedPlayerId = playerId;
            m_quests = it->second;
            return true;
        }
        return false;
    }

    void QuestsSystem::claimQuest(int questId, Player* player,GameState* gameState) {
        if (!player) {
            return;
        }
        const size_t playerId = player->getId();
        const auto it = questsByPlayer.find(playerId);
        if (it == questsByPlayer.end()) {
            return;
        }
        for (auto& q : it->second) {
            if (q.id == questId && q.state == QuestState::Completed) {
                q.state = QuestState::Claimed;
                m_currentShowingQuestId = -1; 
                for (int nextId : q.unlocksIds) {
                    activateQuest(nextId, player, gameState);
                }
                
                
                break;
            }
        }
        if (playerId == displayedPlayerId) {
            m_quests = it->second;
            activeQuests = countOpenQuests(it->second);
        }
    }

    void QuestsSystem::activateQuest(int questId, Player* player, GameState* gameState) {
        if (!player) {
            return;
        }
        const size_t playerId = player->getId();
        const auto it = questsByPlayer.find(playerId);
        if (it == questsByPlayer.end()) {
            return;
        }
        for (auto& q : it->second) {
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
                            q.progress = gameState->getTurnCount();
                            break;
                        case types::QuestGoalType::DISCOVER:
                            q.progress = player->retExploredCount(gameState->getMap());  
                            fmt::println("Already diuscovered ", q.progress);
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
        if (playerId == displayedPlayerId) {
            m_quests = it->second;
            activeQuests = countOpenQuests(it->second);
        }
    }

    void QuestsSystem::notifyNextActiveQuest(Player* player, GameState* gameState) {
        if (m_quests.empty()) return;

        int openCount = countOpenQuests(m_quests);
        if (player) {
            const auto it = questsByPlayer.find(player->getId());
            if (it != questsByPlayer.end()) {
                openCount = countOpenQuests(it->second);
            }
        }
        if (openCount == 0) {
            if (player && !(gameState && gameState->hasAuthoritativeMap())) {
            const int COMPLETION_BONUS = 5;
            player->addHeroPoints(COMPLETION_BONUS);
            
            fmt::println("[QuestsSystem] All quests finished! Player {} awarded {} points.", 
                         player->getId(), COMPLETION_BONUS);
            }

            notifyPlayer(100); 
            return;
        }

        int currentIdx = -1;
        for (int i = 0; i < (int)m_quests.size(); ++i) {
            if (m_quests[i].id == m_currentShowingQuestId) {
                currentIdx = i;
                break;
            }
        }

        for (int i = currentIdx+1; i < (int)m_quests.size(); ++i) {
            auto& q = m_quests[i];

            if (q.state == QuestState::Active || q.state == QuestState::Completed) {
                notifyPlayer(q.id);
                currentQuest++;
                return;
            }
        }
        currentQuest = 1;
        if (m_notificationSystem) {
            m_notificationSystem->close();
        }
        m_currentShowingQuestId = -1;

    }

    void QuestsSystem::reset(){
        init(m_notificationSystem);
    }

    nlohmann::json QuestsSystem::serialize() const {
        nlohmann::json quests = nlohmann::json::array();
        for (const auto& quest : m_quests) {
            quests.push_back({
                {"id", quest.id},
                {"progress", quest.progress},
                {"state", static_cast<int>(quest.state)},
            });
        }
        return quests;
    }

    nlohmann::json QuestsSystem::serializeFor(size_t playerId) const {
        nlohmann::json quests = nlohmann::json::array();
        const auto it = questsByPlayer.find(playerId);
        if (it == questsByPlayer.end()) {
            return quests;
        }
        for (const auto& quest : it->second) {
            quests.push_back({
                {"id", quest.id},
                {"progress", quest.progress},
                {"state", static_cast<int>(quest.state)},
            });
        }
        return quests;
    }

    void QuestsSystem::applyAuthoritative(const nlohmann::json& quests) {
        if (!quests.is_array()) {
            return;
        }

        bool refreshShowing = false;
        for (const auto& item : quests) {
            if (!item.is_object() || !item.contains("id")) {
                continue;
            }
            const int id = item.value("id", -1);
            for (auto& quest : m_quests) {
                if (quest.id != id) {
                    continue;
                }
                const QuestState previousState = quest.state;
                const int previousProgress = quest.progress;
                quest.progress = item.value("progress", quest.progress);
                if (item.contains("state")) {
                    quest.state = static_cast<QuestState>(item.value("state", static_cast<int>(quest.state)));
                }
                if (previousState != QuestState::Completed && previousState != QuestState::Claimed &&
                    quest.state == QuestState::Completed) {
                    notifyPlayer(quest.id);
                    refreshShowing = false;
                } else if (quest.id == m_currentShowingQuestId &&
                    (quest.progress != previousProgress || quest.state != previousState)) {
                    refreshShowing = true;
                }
                break;
            }
        }

        activeQuests = 0;
        for (const auto& quest : m_quests) {
            if (quest.state == QuestState::Active || quest.state == QuestState::Completed) {
                activeQuests++;
            }
        }
        if (refreshShowing && m_currentShowingQuestId >= 0) {
            notifyPlayer(m_currentShowingQuestId);
        }
        questsByPlayer[displayedPlayerId] = m_quests;
    }
}

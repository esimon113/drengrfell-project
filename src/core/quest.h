#pragma once

#include <string>
#include <vector>
#include "../core/types.h" 

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
        types::QuestGoalType goal_type;
        int goal_amount; 
        int progress; 
        std::vector<int> unlocksIds;
        types::TileType reward_resource; 
        int reward_amount;
        QuestState state;
        
        Quest(int _id, std::string _name, std::string _desc, types::QuestGoalType _goalType, 
              int _amount, int _prog, std::vector<int> _unlock, types::TileType _res, 
              int _reward, QuestState _state)
            : id(_id), name(_name), desc(_desc), goal_type(_goalType), 
              goal_amount(_amount), progress(_prog), unlocksIds(_unlock), 
              reward_resource(_res), reward_amount(_reward), state(_state) 
        {}
    };

}
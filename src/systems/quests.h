#pragma once


namespace df{

    enum class QuestState {
        Locked,
        Available,
        Active,
        Completed,
        Finished
    };

    struct Quest{
        int id;
        std::string name;

        QuestState state = QuestState::Locked;

        std::map<std::string, int> goal;
        std::map<std::string, int> progress;

    };

    class QuestsSystem{
        public:
            void updateProgress(const std::string& type, int amount);
            void notifyPlayer(int questId);
            void setQuest();


        private:
            std::vector<Quest> m_quests;

    }

}
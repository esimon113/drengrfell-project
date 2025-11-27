#pragma once
#include <vector>
#include <string>

namespace df {

    class Animation {
    public:
        Animation();
        Animation(const std::vector<int>& frames, float frameDuration, bool loop = true);

        // Update frames based on elapsed time
        void step(float deltaTime);

        //get current frame
        const std::string& getCurrentFrame() const;

        // change frames
        void setFrames(const std::vector<int>& newFrames);

        void setLoop(bool l);
        bool isLooping() const;

        int getCurrentFrameIndex() const noexcept;
        int getCurrentFrameTextureIndex() const noexcept;


    private:
        std::vector<int> frames; 
        // reference to texture/path
        float frameDuration;              
        // duration of frames in seconds
        bool loop;

        size_t currentFrameIndex;        
        // current Frame
        float elapsedTime;               
        // time since last frame change
    };

}

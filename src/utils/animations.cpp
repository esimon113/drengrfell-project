#pragma once
#include <vector>
#include <string>

namespace df {

    class Animation {
    public:
        Animation() = default;

        // constructor
        Animation(const std::vector<std::string>& frames, float frameDuration, bool loop = true)
            : frames(frames), frameDuration(frameDuration), loop(loop), currentFrameIndex(0), elapsedTime(0.f)
        {
        }

        // refresh frames based on elapsed time
        void step(float deltaTime) {
            if (frames.empty()) return;

            elapsedTime += deltaTime;

            // if enough time elapsed
            while (elapsedTime >= frameDuration) {
                elapsedTime -= frameDuration;
                currentFrameIndex++;
                if (currentFrameIndex >= frames.size()) {
                    if (loop) {
                        currentFrameIndex = 0;
                        // go back to first frame
                    }
                    else {
                        currentFrameIndex = frames.size() - 1;
                        // stay at last frame
                    }
                }
            }
        }

        // get current frame
        const std::string& getCurrentFrame() const {
            return frames.at(currentFrameIndex);
        }

        // change frames
        void setFrames(const std::vector<std::string>& newFrames) {
            frames = newFrames;
            currentFrameIndex = 0;
            elapsedTime = 0.f;
        }

        void setLoop(bool l) { loop = l; }
        bool isLooping() const { return loop; }

    private:
        std::vector<std::string> frames;
        // reference to texture
        float frameDuration = 0.1f;
        // duration of frame in seconds (0.1f = 0.1s)
        bool loop = true;

        size_t currentFrameIndex = 0;
        // current frame
        float elapsedTime = 0.f;
        // elapsed time since last frame
    };

}

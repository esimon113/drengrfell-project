#include "animations.h"

namespace df {

    Animation::Animation()
        : frameDuration(0.1f), loop(true), currentFrameIndex(0), elapsedTime(0.f)
    {
    }

    Animation::Animation(const std::vector<std::string>& frames, float frameDuration, bool loop)
        : frames(frames), frameDuration(frameDuration), loop(loop), currentFrameIndex(0), elapsedTime(0.f)
    { }
    

    void Animation::step(float deltaTime) {
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


    const std::string& Animation::getCurrentFrame() const {
        return frames.at(currentFrameIndex);
    }


    void Animation::setFrames(const std::vector<std::string>& newFrames) {
        frames = newFrames;
        currentFrameIndex = 0;
        elapsedTime = 0.f;
    }


    void Animation::setLoop(bool l) { loop = l; }
    bool Animation::isLooping() const { return loop; }

}

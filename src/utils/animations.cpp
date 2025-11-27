#include "animations.h"

namespace df {

    Animation::Animation()
        : frameDuration(0.1f), loop(true), currentFrameIndex(0), elapsedTime(0.f) {
    }

    // constructor
    Animation::Animation(const std::vector<int>& frames, float frameDuration, bool loop)
        : frames(frames), frameDuration(frameDuration), loop(loop),
        currentFrameIndex(0), elapsedTime(0.f) {
    }




    // updated animations based on elapsedtime
    void Animation::step(float deltaTime) {
        if (frames.empty()) return;
        elapsedTime += deltaTime;
        while (elapsedTime >= frameDuration) {
            elapsedTime -= frameDuration;
            currentFrameIndex++;
            if (currentFrameIndex >= frames.size()) {
                currentFrameIndex = loop ? 0 : frames.size() - 1;
            }
        }
    }
    // getter
    int Animation::getCurrentFrameIndex() const noexcept {
        return currentFrameIndex;
    }

    int Animation::getCurrentFrameTextureIndex() const noexcept {
        return frames.at(currentFrameIndex);
    }



    void Animation::setFrames(const std::vector<int>& newFrames) {
        frames = newFrames;
        currentFrameIndex = 0;
        elapsedTime = 0.f;
    }



    void Animation::setLoop(bool l) { loop = l; }
    bool Animation::isLooping() const { return loop; }

}

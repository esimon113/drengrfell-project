#pragma once
#include <vector>
#include <string>

namespace df {

    class Animation {
    public:
        Animation() = default;

        // Konstruktor: Übergabe von Frames und Dauer pro Frame
        Animation(const std::vector<std::string>& frames, float frameDuration, bool loop = true)
            : frames(frames), frameDuration(frameDuration), loop(loop), currentFrameIndex(0), elapsedTime(0.f)
        {
        }

        // Frames aktualisieren basierend auf vergangener Zeit
        void step(float deltaTime) {
            if (frames.empty()) return;

            elapsedTime += deltaTime;

            // Wenn genug Zeit für nächsten Frame vergangen ist
            while (elapsedTime >= frameDuration) {
                elapsedTime -= frameDuration;
                currentFrameIndex++;
                if (currentFrameIndex >= frames.size()) {
                    if (loop) {
                        currentFrameIndex = 0; 
                        // zurück zum ersten Frame
                    }
                    else {
                        currentFrameIndex = frames.size() - 1; 
                        // am letzten Frame bleiben
                    }
                }
            }
        }

        // Aktuellen Frame abfragen
        const std::string& getCurrentFrame() const {
            return frames.at(currentFrameIndex);
        }

        // Frames ändern
        void setFrames(const std::vector<std::string>& newFrames) {
            frames = newFrames;
            currentFrameIndex = 0;
            elapsedTime = 0.f;
        }

        void setLoop(bool l) { loop = l; }
        bool isLooping() const { return loop; }

    private:
        std::vector<std::string> frames; 
        // Referenzen zu Texturen/Dateipfaden
        float frameDuration = 0.1f;      
        // Dauer pro Frame in Sekunden (0.1f = 0.1 sekunden)
        bool loop = true;

        size_t currentFrameIndex = 0;    // aktueller Frame
        float elapsedTime = 0.f;         // Zeit seit letztem Framewechsel
    };

}
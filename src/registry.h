#pragma once

#include <common.h>
#include <tiny_ecs.hpp>
#include "components.h"




namespace df {
	struct Player {};



	// NOTE: For your own project you may want to move the registry to a different file, as it grows in size.
	//	 Depending on the requirements of your projects it may also make sense to have multiple registries.
    class Registry {
    public:
        static Registry* init() noexcept;

        Entity createEntity() noexcept { return Entity{}; }
        void destroyEntity(Entity e) noexcept {
            for (auto* c : containers) c->remove(e);
        }

        void clear() noexcept;
        void clear(Entity e) noexcept;

        ComponentContainer<glm::vec2> positions;
        ComponentContainer<glm::vec2> scales;
        ComponentContainer<float> angles;
        ComponentContainer<Player> players;
        ComponentContainer<float> collisionRadius;
        ComponentContainer<glm::vec3> colors;
        ComponentContainer<AnimationComponent> animations;

        inline Entity getPlayer() noexcept { return player; }
        inline float& getScreenDarkness() noexcept { return screenDarkness; }

    private:
        std::array<ContainerInterface*, 7> containers;
        Entity player;
        float screenDarkness = 0.f;
    };

} // namespace df

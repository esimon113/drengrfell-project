#pragma once

#include <common.h>
#include <tiny_ecs.hpp>
#include <core/road.h>
#include <core/settlement.h>
#include "components.h"

#include "core/player.h"
#include <core/camera.h>
#include <core/cameraInput.h>



namespace df {

	class Player;



	// NOTE: For your own project you may want to move the registry to a different file, as it grows in size.
	//	 Depending on the requirements of your projects it may also make sense to have multiple registries.
    class Registry {
    public:
        static Registry* init() noexcept;

        void clear() noexcept;
        void clear(Entity e) noexcept;

			ComponentContainer<glm::vec2> positions;
			ComponentContainer<glm::vec2> velocities;
			ComponentContainer<glm::vec2> scales;
			ComponentContainer<float> angles;

			ComponentContainer<Player> players;

			ComponentContainer<float> collisionRadius;

			ComponentContainer<glm::vec3> colors;
			ComponentContainer<Road> roads;
			ComponentContainer<Settlement> settlements;

			ComponentContainer<Camera> cameras;
			ComponentContainer<CameraInput> cameraInputs;
			ComponentContainer<AnimationComponent> animations;


			inline Entity getPlayer() noexcept { return player; }
			inline Entity getCamera() noexcept { return camera; }
			inline float& getScreenDarkness() noexcept { return screenDarkness; }


		private:
			std::array<ContainerInterface*, 12> containers;

			Entity player;
			Entity camera;
			float screenDarkness;
	};
} // namespace df

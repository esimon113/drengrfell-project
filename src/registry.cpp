#include "registry.h"



namespace df {
	Registry* Registry::init() noexcept {
		Registry* self = new Registry;

		// Unsure, if this is the right place to initialize this
		Entity camera;
		self->camera = camera;
		self->cameras.emplace(camera);
		Camera& cam = self->cameras.get(self->camera);
		cam.position = glm::vec2(0.0f, 0.0f);
		cam.zoom = 1.0f;
		cam.isActive = true;
		cam.scrollSpeed = 5.0f;

		CameraInput& input = self->cameraInputs.emplace(camera);
		input = {};
		//

		self->containers[0] = &self->positions;
		self->containers[1] = &self->velocities;
		self->containers[2] = &self->scales;
		self->containers[3] = &self->angles;

		self->containers[4] = &self->players;

		self->containers[5] = &self->collisionRadius;

		self->containers[6] = &self->colors;
		self->containers[7] = &self->roads;
		self->containers[8] = &self->settlements;

		self->containers[9] = &self->cameras;
		self->containers[10] = &self->cameraInputs;

		return self;
	}


	void Registry::clear() noexcept {
		for (ContainerInterface* container : containers) {
			container->clear();
		}
	}


	void Registry::clear(const Entity entity) noexcept {
		for (ContainerInterface* container : containers) {
			container->remove(entity);
		}
	}
} // namespace df

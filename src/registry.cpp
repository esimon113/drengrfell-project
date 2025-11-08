#include "registry.h"



namespace df {
	Registry* Registry::init() noexcept {
		Registry* self = new Registry;

		self->containers[0] = &self->positions;
		self->containers[1] = &self->velocities;
		self->containers[2] = &self->scales;
		self->containers[3] = &self->angles;

		self->containers[4] = &self->players;

		self->containers[9] = &self->collisionRadius;

		self->containers[10] = &self->colors;

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

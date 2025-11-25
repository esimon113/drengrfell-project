#include "registry.h"



namespace df {
	Registry* Registry::init() noexcept {
		Registry* self = new Registry;

		self->containers[0] = &self->positions;
		self->containers[1] = &self->scales;
		self->containers[2] = &self->angles;
		self->containers[3] = &self->players;
		self->containers[4] = &self->collisionRadius;
		self->containers[5] = &self->colors;
		self->containers[6] = &self->animations;

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

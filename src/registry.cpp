#include "registry.h"

Registry* Registry::init() noexcept {
	Registry* self = new Registry;

	self->m_containers[0] = &self->m_positions;
	self->m_containers[1] = &self->m_velocities;
	self->m_containers[2] = &self->m_scales;
	self->m_containers[3] = &self->m_angles;

	self->m_containers[4] = &self->m_players;
	self->m_containers[5] = &self->m_eagles;
	self->m_containers[6] = &self->m_eggs;
	self->m_containers[7] = &self->m_bugs;

	self->m_containers[8] = &self->m_death_timers;
	self->m_containers[9] = &self->m_collision_radius;

	self->m_containers[10] = &self->m_colors;

	return self;
}

void Registry::clear() noexcept {
	for (ContainerInterface* container : m_containers) {
		container->clear();
	}
}

void Registry::clear(const Entity entity) noexcept {
	for (ContainerInterface* container : m_containers) {
		container->remove(entity);
	}
}

#include "renderSnow.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "../core/camera.h"
#include <iostream>

namespace df {

// The VBO containing the 4 vertices of the particles (billboard quad)
static const GLfloat g_vertex_buffer_data[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f,
     0.5f,  0.5f, 0.0f,
};

void RenderSnowSystem::reset(){}

RenderSnowSystem RenderSnowSystem::init(Window* window, Registry* registry) noexcept {
    RenderSnowSystem self;
    
    self.window = window;
    self.registry = registry;
    
    self.maxParticles = 10000;
    self.particlesCount = 0;
    
    self.particlesContainer.resize(self.maxParticles);
    self.g_particule_position_size_data.resize(self.maxParticles * 4);
    self.g_particule_color_data.resize(self.maxParticles * 4);

    for(int i = 0; i < self.maxParticles; i++){
        self.particlesContainer[i].life = -1.0f;
    }
    
    // Load shader - make sure particle.vert and particle.frag exist in your shaders folder
    self.particleShader = Shader::init(assets::Shader::particle).value();
    
    self.initBuffers();
    
    return self;
}

void RenderSnowSystem::initBuffers() noexcept {
    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // The VBO containing the 4 vertices of the particles (shared by all particles)
    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    
    // The VBO containing the positions and sizes of the particles
    glGenBuffers(1, &particles_position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    // Initialize with empty (NULL) buffer : it will be updated later, each frame.
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
    
    // The VBO containing the colors of the particles
    glGenBuffers(1, &particles_color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    // Initialize with empty (NULL) buffer : it will be updated later, each frame.
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
    
    glBindVertexArray(0);
}

void RenderSnowSystem::deinit() noexcept {
    glDeleteBuffers(1, &billboard_vertex_buffer);
    glDeleteBuffers(1, &particles_position_buffer);
    glDeleteBuffers(1, &particles_color_buffer);
    glDeleteVertexArrays(1, &vao);
    
    particleShader.deinit();
}

int RenderSnowSystem::findUnusedParticle() noexcept {
    static int lastUsedParticle = 0;
    for(int i = lastUsedParticle; i < maxParticles; i++){
        if (particlesContainer[i].life < 0){
            lastUsedParticle = i;
            return i;
        }
    }
    
    for(int i = 0; i < lastUsedParticle; i++){
        if (particlesContainer[i].life < 0){
            lastUsedParticle = i;
            return i;
        }
    }
    
    return 0; // All particles are taken, override the first one
}

void RenderSnowSystem::sortParticles() noexcept {
    std::sort(&particlesContainer[0], &particlesContainer[maxParticles]);
}

void RenderSnowSystem::step(float deltaTime) noexcept {
    // Get camera position from registry
    Camera& cam = registry->cameras.get(registry->getCamera());
    glm::vec3 cameraPos = glm::vec3(cam.position.x, cam.position.y, 0.0f);
    
    // Fix deltaTime if it's 0
    if (deltaTime < 0.001f || deltaTime > 0.1f) {
        deltaTime = 0.016f; // 60 FPS
    }

   
    
    int newparticles = 1;
    

    for(int i = 0; i < newparticles; i++){
        int particleIndex = findUnusedParticle();
        Particle& p = particlesContainer[particleIndex];
        
        p.life = 10.0f + (rand() % 20); // Vida entre 30 y 40 segundos
        p.pos = glm::vec3(
            cameraPos.x + (rand() % 8000 - 2000.0f) / 100.0f,
            cameraPos.y + 35.0f,
            0.0f
        );
        
        p.speed = glm::vec3(
            (rand() % 60 - 30.0f) / 500.0f,
            -1.0f,
            0.0f
        );
        
        p.r = 255;
        p.g = 255;
        p.b = 255;
        p.a = 160 + (rand() % 75);
        
        p.size = 0.10f;
    }
    
    particlesCount = 0;
    for(int i = 0; i < maxParticles; i++){
        Particle& p = particlesContainer[i];
        
        if(p.life > 0.0f){
            p.life -= deltaTime;
            if (p.life > 0.0f){
                p.pos += p.speed * deltaTime;
                p.pos.x += 0.04f * sin(p.life * 2.0f) * deltaTime;
                p.cameradistance = glm::length(p.pos - cameraPos);
                
                g_particule_position_size_data[4*particlesCount+0] = p.pos.x;
                g_particule_position_size_data[4*particlesCount+1] = p.pos.y;
                g_particule_position_size_data[4*particlesCount+2] = p.pos.z;
                g_particule_position_size_data[4*particlesCount+3] = p.size;
                
                g_particule_color_data[4*particlesCount+0] = p.r;
                g_particule_color_data[4*particlesCount+1] = p.g;
                g_particule_color_data[4*particlesCount+2] = p.b;
                g_particule_color_data[4*particlesCount+3] = p.a;
                
                particlesCount++;
            } else {
                p.cameradistance = -1.0f;
            }
        }
    }
    
    sortParticles();
    
    // --- RENDERIZAR AL FINAL DEL STEP (igual que tiles) ---
    // Usar EXACTAMENTE la misma proyección que renderTiles
    const glm::vec2 worldDimensions = calculateWorldDimensions(10, 10);
    
    glm::vec2 camPos2D = cam.position;
    float camZoom = cam.zoom;
    
    const glm::mat4 projection = glm::ortho(
        camPos2D.x, camPos2D.x + worldDimensions.x / camZoom,
        camPos2D.y, camPos2D.y + worldDimensions.y / camZoom,
        -1.0f, 1.0f
    );
    
    glm::mat4 view = glm::mat4(1.0f); // View matrix identidad para 2D
    std::cout << "Rendering snow with " << particlesCount << " particles." << std::endl;
    // Llamar a render
    render(view, projection);
}
void RenderSnowSystem::render(const glm::mat4& view, const glm::mat4& projection) noexcept {
    if (particlesCount == 0) return;
    
    // Update buffers
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLfloat) * 4, g_particule_position_size_data.data());
    
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, particlesCount * sizeof(GLubyte) * 4, g_particule_color_data.data());
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    particleShader.use();
    particleShader.setMat4("V", view);
    particleShader.setMat4("P", projection);
    
    glBindVertexArray(vao);
    
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
    
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);
    
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particlesCount);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    
    // CRÍTICO: Limpiar todo el estado
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);  // Dejar de usar el shader
    
    // Dejar blending activado porque los tiles también lo usan
    // glDisable(GL_BLEND);  // NO desactivar si tiles lo necesitan
}

}
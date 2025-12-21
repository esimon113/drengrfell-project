#pragma once

#include <glm/glm.hpp>

namespace df {

    struct Camera {
        glm::vec2 position = { 0.0f, 0.0f };
        float zoom = 1.0f;          // Magnifying factor of how much bigger the map is displayed in relation to the standard camera
        bool isActive = false;      // sets if the camera is active, may be usefull if we have other cameras later
        float scrollSpeed = 10.0f;   // how fast the camera moves
        float camOffset = 2.5f;     // how much more beyond the map borders is visible; may need further testing to determine a good value for all zoom levels
        float zoomMaxValue = 3.0f;  // value for highest possible zoom
        float zoomMinValue = 0.5f;  // value for lowest possible zoom
        float baseViewHeight = 10.0f;    // base value of how much of the world is visible in the camera
        float viewWidth = 0.0f;     // how much of the world is visible in the camera horizontally, relative to the windowExtend
        float viewHeight = 0.0f;    // how much of the world is visible in the camera vertically, relative to the windowExtend

        // calculates the camera size based on the windowExtend
        void updateView(glm::uvec2 windowExtent) {
            float aspect = static_cast<float>(windowExtent.x) / windowExtent.y;
            viewHeight = baseViewHeight / zoom;
            viewWidth = viewHeight * aspect;
        }

        // positions of the four corners of the camera
        float minX() const { return position.x; }
        float maxX() const { return position.x + viewWidth; }
        float minY() const { return position.y; }
        float maxY() const { return position.y + viewHeight; }

    };

}

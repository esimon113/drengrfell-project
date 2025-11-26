#pragma once

#include <glm/glm.hpp>

namespace df {

    struct Camera {
        glm::vec2 position = { 0.0f, 0.0f };
        float zoom = 1.0f;          // Magnifying factor of how much bigger the map is displayed in relation to the standard camera
        bool isActive = false;      // sets if the camera is active, may be usefull if we have other cameras later
        float scrollSpeed = 5.0f;   // how fast the camera moves
        float camOffset = 2.5f;     // how much more beyond the map borders is visible; may need further testing to determine a good value for all zoom levels
        float zoomMaxValue = 3.0f;  // value for highest possible zoom
        float zoomMinValue = 0.5f;  // value for lowest possible zoom
    };

}

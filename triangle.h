#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include "color.h"
#include "framebuffer.h"

// Dibujar una línea usando el algoritmo de Bresenham (con depth = 0 por defecto)
void line(int x1, int y1, int x2, int y2, const Color& color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        point(x1, y1, 0.0f, color);  // Profundidad 0 para líneas
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

// Función auxiliar para encontrar el bounding box del triángulo
void getBoundingBox(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                    int& minX, int& minY, int& maxX, int& maxY) {
    minX = static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x})));
    minY = static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y})));
    maxX = static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x})));
    maxY = static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y})));

    // Clamp al tamaño de la pantalla para no salirse de los límites
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(SCREEN_WIDTH - 1, maxX);
    maxY = std::min(SCREEN_HEIGHT - 1, maxY);
}

// Calcular coordenadas baricéntricas de un punto P respecto al triángulo ABC
glm::vec3 barycentric(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, const glm::vec3& P) {
    glm::vec3 v0 = B - A;
    glm::vec3 v1 = C - A;
    glm::vec3 v2 = P - A;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    
    // Verificar triángulo degenerado
    if (std::abs(denom) < 1e-8f) {
        return glm::vec3(-1, -1, -1);
    }

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

// Dibujar un triángulo relleno con z-buffer
void triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const Color& color) {
    int minX, minY, maxX, maxY;
    getBoundingBox(v0, v1, v2, minX, minY, maxX, maxY);

    // Iterar sobre cada pixel en el bounding box
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            // Punto en el centro del píxel
            glm::vec3 P(x + 0.5f, y + 0.5f, 0.0f);
            
            // Calcular coordenadas baricéntricas
            glm::vec3 bary = barycentric(v0, v1, v2, P);

            // Si todas las coordenadas baricéntricas son >= 0, el punto está dentro del triángulo
            if (bary.x >= 0 && bary.y >= 0 && bary.z >= 0) {
                // Interpolar la profundidad usando coordenadas baricéntricas
                float depth = bary.x * v0.z + bary.y * v1.z + bary.z * v2.z;
                
                // Dibujar el píxel con verificación de profundidad
                point(x, y, depth, color);
            }
        }
    }
}

// Dibujar solo los bordes del triángulo (wireframe)
void triangleWireframe(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const Color& color) {
    line(static_cast<int>(v0.x), static_cast<int>(v0.y),
         static_cast<int>(v1.x), static_cast<int>(v1.y), color);
    line(static_cast<int>(v1.x), static_cast<int>(v1.y),
         static_cast<int>(v2.x), static_cast<int>(v2.y), color);
    line(static_cast<int>(v2.x), static_cast<int>(v2.y),
         static_cast<int>(v0.x), static_cast<int>(v0.y), color);
}
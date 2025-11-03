#include <SDL2/SDL.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include "color.h"
#include "framebuffer.h"
#include "triangle.h"
#include "objloader.h"

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Vectores para almacenar el modelo
std::vector<glm::vec3> vertices;
std::vector<Face> faces;

// Variables de cámara
float cameraAngleX = 0.3f;  // Ángulo inicial para ver mejor la nave
float cameraAngleY = 0.0f;
float cameraDistance = 3.5f;

// Variables del modelo
float modelScale = 1.0f;
glm::vec3 modelCenter;
float modelRotationY = 0.0f;  // Rotación Y del modelo (Q/E)

// Luz direccional
glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, -0.3f, 1.0f));

// Estructura para almacenar un triángulo con su profundidad promedio
struct TriangleData {
    glm::vec3 v0, v1, v2;
    Color color;
    float avgDepth;
};

void init() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Software Renderer - Spaceship", 
                               SDL_WINDOWPOS_CENTERED, 
                               SDL_WINDOWPOS_CENTERED, 
                               SCREEN_WIDTH, 
                               SCREEN_HEIGHT, 
                               SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

// Calcular el bounding box del modelo y normalizarlo
void calculateModelBounds() {
    if (vertices.empty()) return;
    
    glm::vec3 modelMin = vertices[0];
    glm::vec3 modelMax = vertices[0];
    
    for (const auto& v : vertices) {
        modelMin.x = std::min(modelMin.x, v.x);
        modelMin.y = std::min(modelMin.y, v.y);
        modelMin.z = std::min(modelMin.z, v.z);
        
        modelMax.x = std::max(modelMax.x, v.x);
        modelMax.y = std::max(modelMax.y, v.y);
        modelMax.z = std::max(modelMax.z, v.z);
    }
    
    modelCenter = (modelMin + modelMax) * 0.5f;
    
    // Calcular escala para normalizar el modelo
    glm::vec3 modelSize = modelMax - modelMin;
    float maxDimension = std::max({modelSize.x, modelSize.y, modelSize.z});
    
    if (maxDimension > 0) {
        modelScale = 2.0f / maxDimension;
    }
    
    std::cout << "Tamaño del modelo: " << modelSize.x << " x " << modelSize.y << " x " << modelSize.z << std::endl;
    std::cout << "Centro del modelo: " << modelCenter.x << ", " << modelCenter.y << ", " << modelCenter.z << std::endl;
    std::cout << "Escala del modelo: " << modelScale << std::endl;
}

// Crear matriz de modelo con rotación inicial para corregir orientación
glm::mat4 createModelMatrix() {
    glm::mat4 model = glm::mat4(1.0f);
    
    // Rotar 180 grados en Y para voltear la nave
    model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Rotar 90 grados en X para orientar correctamente
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Aplicar rotación controlada por el usuario en el eje Y (Q/E)
    model = glm::rotate(model, modelRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    
    return model;
}

// Crear matriz de vista
glm::mat4 createViewMatrix() {
    float x = cameraDistance * sin(cameraAngleY) * cos(cameraAngleX);
    float y = cameraDistance * sin(cameraAngleX);
    float z = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
    
    glm::vec3 eye = glm::vec3(x, y, z);
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    return glm::lookAt(eye, center, up);
}

// Crear matriz de proyección en perspectiva
glm::mat4 createProjectionMatrix() {
    float fov = glm::radians(45.0f);
    float aspect = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    float near = 0.1f;
    float far = 100.0f;
    
    return glm::perspective(fov, aspect, near, far);
}

// Crear matriz de viewport
glm::mat4 createViewportMatrix() {
    glm::mat4 viewport = glm::mat4(1.0f);
    
    viewport[0][0] = SCREEN_WIDTH / 2.0f;
    viewport[1][1] = SCREEN_HEIGHT / 2.0f;
    viewport[2][2] = 1.0f;
    
    viewport[3][0] = SCREEN_WIDTH / 2.0f;
    viewport[3][1] = SCREEN_HEIGHT / 2.0f;
    viewport[3][2] = 0.0f;
    
    return viewport;
}

// Transformar vértice con matrices
glm::vec3 transformVertex(const glm::vec3& vertex, const glm::mat4& mvp, const glm::mat4& viewport) {
    glm::vec3 centered = (vertex - modelCenter) * modelScale;
    glm::vec4 v = glm::vec4(centered, 1.0f);
    
    glm::vec4 transformed = mvp * v;
    
    if (transformed.w != 0.0f) {
        transformed /= transformed.w;
    }
    
    glm::vec4 screen = viewport * transformed;
    
    return glm::vec3(screen.x, screen.y, transformed.z);
}

// Calcular la normal de un triángulo
glm::vec3 calculateNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::cross(edge1, edge2);
    
    float length = glm::length(normal);
    if (length > 0.0001f) {
        normal = normal / length;
    }
    
    return normal;
}

// Generar color tipo transbordador espacial basado en posición
Color generateSpaceshipColor(const glm::vec3& worldPos, const glm::vec3& normal, float lightIntensity) {
    Color baseColor;
    float lateralDistance = abs(worldPos.x);

    // === Colores del modelo ===

    // 🔥 Parte trasera (propulsores y zona roja/naranja) — sin cambios
    if (worldPos.z < -0.5f) {
        if (worldPos.z < -0.7f) {
            baseColor = Color(255, 180, 0);  // Naranja brillante (propulsores)
        } else {
            baseColor = Color(143, 43, 14);  // Rojo oscuro trasero
        }
    }
    // ⚪ Cabina / Ventanas (zona blanca justo detrás de la punta)
    else if (worldPos.z > 0.4f && worldPos.z <= 0.75f && lateralDistance < 0.4f) {
        baseColor = Color(240, 240, 240);  // Blanco brillante (ventanas)
    }
    // 🔴⚪ Punta frontal: degradado rojo-blanco
    else if (worldPos.z > 0.75f) {
        if (worldPos.z > 1.0f) {
            baseColor = Color(143, 43, 14);  // Rojo oscuro extremo delantero
        } else {
            baseColor = Color(230, 220, 220);  // Blanco claro intermedio (transición dentro de la punta)
        }
    }
    // 🔵 Cuerpo principal azul-verde metálico
    else {
        baseColor = Color(78, 120, 122);  // Azul-verde metálico
    }

    // === Iluminación ambiental + difusa ===
    float ambientLight = 0.4f;
    float diffuseLight = 0.6f * std::max(0.0f, lightIntensity);
    float finalIntensity = ambientLight + diffuseLight;
    finalIntensity = std::clamp(finalIntensity, 0.0f, 1.0f);

    // Aplicar la iluminación al color final
    return Color(
        static_cast<int>(baseColor.r * finalIntensity),
        static_cast<int>(baseColor.g * finalIntensity),
        static_cast<int>(baseColor.b * finalIntensity)
    );
}



void render() {
    // Crear matrices de transformación
    glm::mat4 model = createModelMatrix();
    glm::mat4 view = createViewMatrix();
    glm::mat4 projection = createProjectionMatrix();
    glm::mat4 viewport = createViewportMatrix();
    glm::mat4 mvp = projection * view * model;
    glm::mat4 mv = view * model;
    
    // Vector para almacenar todos los triángulos
    std::vector<TriangleData> triangles;
    triangles.reserve(faces.size());
    
    // Transformar todos los triángulos
    for (size_t i = 0; i < faces.size(); i++) {
        const auto& face = faces[i];
        
        if (face.vertexIndices.size() >= 3) {
            glm::vec3 v0_original = vertices[face.vertexIndices[0]];
            glm::vec3 v1_original = vertices[face.vertexIndices[1]];
            glm::vec3 v2_original = vertices[face.vertexIndices[2]];
            
            // Transformar vértices
            glm::vec3 v0 = transformVertex(v0_original, mvp, viewport);
            glm::vec3 v1 = transformVertex(v1_original, mvp, viewport);
            glm::vec3 v2 = transformVertex(v2_original, mvp, viewport);
            
            // Calcular normal en espacio de vista
            glm::vec3 v0_centered = (v0_original - modelCenter) * modelScale;
            glm::vec3 v1_centered = (v1_original - modelCenter) * modelScale;
            glm::vec3 v2_centered = (v2_original - modelCenter) * modelScale;
            
            glm::vec3 v0_view = glm::vec3(mv * glm::vec4(v0_centered, 1.0f));
            glm::vec3 v1_view = glm::vec3(mv * glm::vec4(v1_centered, 1.0f));
            glm::vec3 v2_view = glm::vec3(mv * glm::vec4(v2_centered, 1.0f));
            
            glm::vec3 normal = calculateNormal(v0_view, v1_view, v2_view);
            
            // Backface culling
            glm::vec3 faceCenter = (v0_view + v1_view + v2_view) / 3.0f;
            glm::vec3 viewDir = glm::normalize(-faceCenter);
            
            if (glm::dot(normal, viewDir) > 0.0f) {
                float avgDepth = (v0.z + v1.z + v2.z) / 3.0f;
                
                // Calcular posición promedio en espacio mundo para determinar color
                glm::vec3 worldPos = (v0_centered + v1_centered + v2_centered) / 3.0f;
                
                // Calcular intensidad de luz
                float intensity = glm::dot(normal, lightDir);
                
                // Generar color tipo transbordador
                Color color = generateSpaceshipColor(worldPos, normal, intensity);
                
                TriangleData tri;
                tri.v0 = v0;
                tri.v1 = v1;
                tri.v2 = v2;
                tri.color = color;
                tri.avgDepth = avgDepth;
                triangles.push_back(tri);
            }
        }
    }
    
    // Ordenar por profundidad
    std::sort(triangles.begin(), triangles.end(), 
              [](const TriangleData& a, const TriangleData& b) {
                  return a.avgDepth > b.avgDepth;
              });
    
    // Renderizar
    for (const auto& tri : triangles) {
        triangle(tri.v0, tri.v1, tri.v2, tri.color);
    }
}

void handleInput(SDL_Event& event, bool& running) {
    const float rotationSpeed = 0.08f;
    const float zoomSpeed = 0.15f;
    const float modelRotationSpeed = 0.1f;
    
    if (event.type == SDL_QUIT) {
        running = false;
    }
    
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                running = false;
                break;
                
            // Rotación de cámara
            case SDLK_UP:
                cameraAngleX += rotationSpeed;
                if (cameraAngleX > glm::radians(89.0f)) cameraAngleX = glm::radians(89.0f);
                break;
            case SDLK_DOWN:
                cameraAngleX -= rotationSpeed;
                if (cameraAngleX < glm::radians(-89.0f)) cameraAngleX = glm::radians(-89.0f);
                break;
            case SDLK_LEFT:
                cameraAngleY -= rotationSpeed;
                break;
            case SDLK_RIGHT:
                cameraAngleY += rotationSpeed;
                break;
            
            // Rotación del modelo en eje Y (Q/E) - gira sobre sí misma
            case SDLK_q:
                modelRotationY -= modelRotationSpeed;
                std::cout << "Rotación nave: " << glm::degrees(modelRotationY) << "°" << std::endl;
                break;
            case SDLK_e:
                modelRotationY += modelRotationSpeed;
                std::cout << "Rotación nave: " << glm::degrees(modelRotationY) << "°" << std::endl;
                break;
                
            // Zoom
            case SDLK_w:
                cameraDistance -= zoomSpeed;
                if (cameraDistance < 0.5f) cameraDistance = 0.5f;
                break;
            case SDLK_s:
                cameraDistance += zoomSpeed;
                if (cameraDistance > 10.0f) cameraDistance = 10.0f;
                break;
                
            // Reset
            case SDLK_r:
                cameraAngleX = 0.3f;
                cameraAngleY = 0.0f;
                cameraDistance = 3.5f;
                modelRotationY = 0.0f;
                std::cout << "Reset cámara y rotación" << std::endl;
                break;
                
            // Cambiar dirección de luz
            case SDLK_1:
                lightDir = glm::normalize(glm::vec3(0.5f, -0.3f, 1.0f));
                std::cout << "Luz: Frontal" << std::endl;
                break;
            case SDLK_2:
                lightDir = glm::normalize(glm::vec3(1.0f, 0.0f, 0.5f));
                std::cout << "Luz: Derecha" << std::endl;
                break;
            case SDLK_3:
                lightDir = glm::normalize(glm::vec3(-1.0f, 0.0f, 0.5f));
                std::cout << "Luz: Izquierda" << std::endl;
                break;
            case SDLK_4:
                lightDir = glm::normalize(glm::vec3(0.0f, 1.0f, 0.5f));
                std::cout << "Luz: Superior" << std::endl;
                break;
            case SDLK_5:
                lightDir = glm::normalize(glm::vec3(0.0f, -1.0f, 0.5f));
                std::cout << "Luz: Inferior" << std::endl;
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    init();
    
    std::cout << "Cargando modelo..." << std::endl;
    if (!loadOBJ("Modelo3D.obj", vertices, faces)) {
        std::cerr << "Error: No se pudo cargar el modelo Modelo3D.obj" << std::endl;
        return 1;
    }
    
    calculateModelBounds();
    
    std::cout << "\n=== SPACESHIP RENDERER ===" << std::endl;
    std::cout << "Vértices: " << vertices.size() << std::endl;
    std::cout << "Caras: " << faces.size() << std::endl;
    std::cout << "\n=== CONTROLES ===" << std::endl;
    std::cout << "Flechas: Rotar cámara" << std::endl;
    std::cout << "Q/E: Girar nave sobre sí misma" << std::endl;
    std::cout << "W/S: Zoom" << std::endl;
    std::cout << "R: Reset" << std::endl;
    std::cout << "1-5: Cambiar luz" << std::endl;
    std::cout << "ESC: Salir\n" << std::endl;
    
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleInput(event, running);
        }

        clear(Color(10, 10, 15));
        render();
        renderBuffer(renderer);
        
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
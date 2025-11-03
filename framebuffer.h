#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <algorithm>
#include <limits>
#include "color.h"

// Dimensiones de la pantalla
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Buffer de píxeles
std::vector<Color> framebuffer(SCREEN_WIDTH * SCREEN_HEIGHT);

// Z-buffer para manejo de profundidad
std::vector<float> zbuffer(SCREEN_WIDTH * SCREEN_HEIGHT);

// Limpiar el framebuffer con un color específico
void clear(const Color& clearColor = Color(0, 0, 0)) {
    std::fill(framebuffer.begin(), framebuffer.end(), clearColor);
    std::fill(zbuffer.begin(), zbuffer.end(), std::numeric_limits<float>::max());
}

// Colocar un punto (píxel) en el framebuffer con verificación de profundidad
void point(int x, int y, float depth, const Color& color) {
    // Verificar que el punto esté dentro de los límites de la pantalla
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        int index = y * SCREEN_WIDTH + x;
        
        // Solo dibujar si este píxel está más cerca que el anterior
        if (depth < zbuffer[index]) {
            zbuffer[index] = depth;
            framebuffer[index] = color;
        }
    }
}

// Renderizar el framebuffer en la ventana de SDL
void renderBuffer(SDL_Renderer* renderer) {
    // Crear una textura para el framebuffer
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    // Bloquear la textura para escribir en ella
    void* texturePixels;
    int texturePitch;
    SDL_LockTexture(texture, NULL, &texturePixels, &texturePitch);

    // Copiar el framebuffer a la textura
    Uint32* pixelPtr = static_cast<Uint32*>(texturePixels);
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        // Formato RGBA8888: combinar los componentes en un Uint32
        pixelPtr[i] = (framebuffer[i].r << 24) | 
              (framebuffer[i].g << 16) | 
              (framebuffer[i].b << 8) | 
              framebuffer[i].a;
    }

    SDL_UnlockTexture(texture);

    // Renderizar la textura en la ventana
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // Liberar la textura
    SDL_DestroyTexture(texture);
}
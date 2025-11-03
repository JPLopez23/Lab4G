#pragma once
#include <SDL2/SDL.h>
#include <algorithm>

struct Color {
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;

    Color() : r(0), g(0), b(0), a(255) {}
    
    Color(int red, int green, int blue, int alpha = 255) 
        : r(static_cast<Uint8>(red)), g(static_cast<Uint8>(green)), b(static_cast<Uint8>(blue)), a(static_cast<Uint8>(alpha)) {}

    // Operadores para mezclar colores
    Color operator+(const Color& other) const {
        return Color(
            std::min(255, static_cast<int>(r) + static_cast<int>(other.r)),
            std::min(255, static_cast<int>(g) + static_cast<int>(other.g)),
            std::min(255, static_cast<int>(b) + static_cast<int>(other.b)),
            std::min(255, static_cast<int>(a) + static_cast<int>(other.a))
        );
    }

    Color operator*(float factor) const {
        return Color(
            static_cast<int>(std::clamp(r * factor, 0.0f, 255.0f)),
            static_cast<int>(std::clamp(g * factor, 0.0f, 255.0f)),
            static_cast<int>(std::clamp(b * factor, 0.0f, 255.0f)),
            a
        );
    }
};

// Colores predefinidos
namespace Colors {
    const Color Black(0, 0, 0);
    const Color White(255, 255, 255);
    const Color Red(255, 0, 0);
    const Color Green(0, 255, 0);
    const Color Blue(0, 0, 255);
    const Color Yellow(255, 255, 0);
    const Color Cyan(0, 255, 255);
    const Color Magenta(255, 0, 255);
}
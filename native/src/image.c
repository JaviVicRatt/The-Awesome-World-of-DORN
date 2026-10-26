#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include "image.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

SDL_Texture *dorn_load_texture(SDL_Renderer *renderer, const char *path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels = stbi_load(path, &width, &height, &channels, 4);
    if (pixels == NULL) {
        fprintf(stderr, "No se pudo cargar la imagen '%s': %s\n",
                path, stbi_failure_reason());
        return NULL;
    }
    /* EN: SDL wraps the decoded RGBA buffer; free the surface before its pixel buffer.
     * ES: SDL envuelve el buffer RGBA; se libera la superficie antes que sus pixeles.
     */
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        pixels, width, height, 32, width * 4, SDL_PIXELFORMAT_RGBA32);
    SDL_Texture *texture = surface != NULL
                               ? SDL_CreateTextureFromSurface(renderer, surface)
                               : NULL;
    if (surface == NULL || texture == NULL) {
        fprintf(stderr, "No se pudo crear textura '%s': %s\n", path, SDL_GetError());
    }
    if (surface != NULL) {
        SDL_FreeSurface(surface);
    }
    stbi_image_free(pixels);
    return texture;
}

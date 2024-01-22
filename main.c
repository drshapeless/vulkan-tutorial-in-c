#include "config.h"

#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

static const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
#ifdef NDEBUG
static const bool enableValidationLayers = false;
#else
static const bool enableValidationLayers = true;
#endif

void error_log(const char *s);
bool checkValidationLayerSupport();

int main(int argc, char *argv[]) {
    int rc = 0;
    bool running = true;

    VkInstance instance;

    /* init sdl */
    rc = SDL_Init(SDL_INIT_VIDEO);
    if (rc != 0) {
        error_log(SDL_GetError());
        return 1;
    }

    /* init window */
    SDL_Window *window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

    if (window == NULL) {
        error_log(SDL_GetError());
        return 1;
    }

    /* init vulkan */
    /* create instance */
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        error_log("validation layers requested, but not available");
        return 1;
    }
    VkApplicationInfo appInfo = { 0 };
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t sdlExtensionCount = 0;
    const char **sdlExtensions = NULL;

    if (SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount,
                                         sdlExtensions) == SDL_FALSE) {
        error_log("cannot get instance extensions");
        error_log(SDL_GetError());
        return 1;
    }

    uint32_t extensionCount = 0;
    /* This should be big enough to hold all the extensions. */
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    const char **extensions = malloc(sizeof(char **) * extensionCount);
    int extensionIndex = 0;

#ifdef __APPLE__
    extensions[0] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    extensions[1] = "VK_KHR_get_physical_device_properties2";
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    extensionIndex += 2;
#endif
    for (int i = 0; i < sdlExtensionCount; i++) {
        extensions[extensionIndex + i] = sdlExtensions[i];
    }

    extensionIndex += sdlExtensionCount;

    createInfo.enabledExtensionCount = extensionIndex;
    createInfo.ppEnabledExtensionNames = extensions;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            sizeof(validationLayers) / sizeof(char *);
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        error_log("failed to create instance");
        return 1;
    }

    free(extensions);

    /* main loop */
    while (running) {
        /* process event */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                running = false;
                break;
            }
        }
    }

    /* clean up */
    vkDestroyInstance(instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void error_log(const char *s) {
    fprintf(stderr, "%s\n", s);
}

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties *availableLayers =
        malloc(sizeof(VkLayerProperties) * layerCount);

    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (int i = 0; i < sizeof(validationLayers) / sizeof(char *); i++) {
        bool layerFound = false;

        for (int j = 0; j < layerCount; j++) {
            if (strcmp(validationLayers[i], availableLayers[j].layerName) ==
                0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            free(availableLayers);
            return false;
        }
    }

    free(availableLayers);
    return true;
}

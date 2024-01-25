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
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator);

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo);
bool isDeviceSuitable(VkPhysicalDevice device);

int main(int argc, char *argv[]) {
    int rc = 0;
    bool running = true;

    VkInstance instance = NULL;
    VkDebugUtilsMessengerEXT debugMessenger = NULL;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

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
    if (enableValidationLayers) {
        /* This is from the original getRequiredExtensions() function,
           c++ hide a lot of stuffs by extracting one-use function. */
        extensions[extensionIndex] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        extensionIndex += 1;
    }

    createInfo.enabledExtensionCount = extensionIndex;
    createInfo.ppEnabledExtensionNames = extensions;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { 0 };
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            sizeof(validationLayers) / sizeof(char *);
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext =
            (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    if (vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
        error_log("failed to create instance");
        return 1;
    }

    free(extensions);

    /* setup debug messenger */
    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = { 0 };
        /* populateDebugMessengerCreateInfo(&createInfo); */
        createInfo.sType =
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        VkResult result = CreateDebugUtilsMessengerEXT(instance, &createInfo,
                                                       NULL, &debugMessenger);
        if (result != VK_SUCCESS) {
            printf("result: %d\n", result);
            error_log("failed to set up debug messenger!");
            return 1;
        }
    }

    /* pick physical device */
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        error_log("failed to find GPUs with Vulkan support!");
        return 1;
    }

    VkPhysicalDevice *devices =
        malloc(sizeof(VkPhysicalDevice *) * deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    for (int i = 0; i < deviceCount; i++) {
        if (isDeviceSuitable(devices[i])) {
            physicalDevice = devices[i];
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        error_log("failed to find a suitable GPU!");
        return 1;
    }

    free(devices);

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
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    }

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

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    char s[MSG_LEN] = { 0 };
    sprintf(s, "validation layer: %s", pCallbackData->pMessage);
    error_log(s);

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    printf("%p\n", func);
    if (func != NULL) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT *createInfo) {
    memset(createInfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    return true;
}

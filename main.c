#include "config.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
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

void error_log(const char *format, ...);
void add_to_unique_set(uint32_t *set, int *size, uint32_t value) {
    /* TODO: make a binary search here, like a real set */
    bool should_add = true;

    for (int i = 0; i < *size; i++) {
        if (set[i] == value) {
            should_add = false;
            break;
        }
    }

    if (should_add) {
        set[*size] = value;
        *size += 1;
    }
}

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
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
struct QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                            VkSurfaceKHR surface);

struct QueueFamilyIndices {
    /* the vulkan tutorial uses a optional type in C++17 which we
       don't have in C */

    /* use a bool to indicate whether it has value */
    uint32_t graphicsFamily;
    bool graphicsFamilyHasValue;

    uint32_t presentFamily;
    bool presentFamilyHasValue;

    /* there is a isComplete member function here, we don't have it in
       C */
};

/* ugly workaround of the OO features in the original tutorial */
bool QueueFamilyIndicesIsComplete(
    struct QueueFamilyIndices *queueFamiliyIndices) {
    return queueFamiliyIndices->graphicsFamilyHasValue &&
           queueFamiliyIndices->presentFamilyHasValue;
}

int main(int argc, char *argv[]) {
    int rc = 0;
    bool running = true;

    VkInstance instance = NULL;
    VkDebugUtilsMessengerEXT debugMessenger = NULL;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = NULL;
    VkQueue graphicsQueue = NULL;
    VkSurfaceKHR surface = NULL;

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
    {
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
#elif defined __linux__
        extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
        extensions[1] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
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
    }
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

    /* create surface */
    if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE) {
        error_log("failed to create window surface! %s", SDL_GetError());
        return 1;
    }

    /* pick physical device */
    {
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
            if (isDeviceSuitable(devices[i], surface)) {
                physicalDevice = devices[i];
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            error_log("failed to find a suitable GPU!");
            return 1;
        }

        free(devices);
    }

    /* create logical device */
    {
        struct QueueFamilyIndices indices =
            findQueueFamilies(physicalDevice, surface);
        VkDeviceQueueCreateInfo queueCreateInfo = { 0 };
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = { 0 };

        VkDeviceCreateInfo createInfo = { 0 };
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount =
                (uint32_t)(sizeof(validationLayers) / sizeof(char *));
            createInfo.ppEnabledLayerNames = validationLayers;
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) !=
            VK_SUCCESS) {
            error_log("failed to create logical device!");
            return 1;
        }
    }
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
    vkDestroyDevice(device, NULL);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void error_log(const char *s, ...) {
    va_list argptr;
    va_start(argptr, s);
    vfprintf(stderr, s, argptr);
    va_end(argptr);
    fprintf(stderr, "\n");
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

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    struct QueueFamilyIndices indices = findQueueFamilies(device, surface);

    return QueueFamilyIndicesIsComplete(&indices);
}
struct QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device,
                                            VkSurfaceKHR surface) {
    struct QueueFamilyIndices indices = { 0 };
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies =
        malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                             queueFamilies);
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            /* work around of optional */
            indices.graphicsFamilyHasValue = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (QueueFamilyIndicesIsComplete(&indices)) {
            break;
        }
    }

    free(queueFamilies);
    return indices;
}

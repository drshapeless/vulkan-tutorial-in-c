#include "config.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_metal.h>
#include <vulkan/vulkan_beta.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

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
bool checkDeviceExtensionSupport(VkPhysicalDevice device);
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

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    int formatsSize;
    VkPresentModeKHR *presentModes;
    int presentModesSize;
};

struct SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                     VkSurfaceKHR surface);
void DestroySwapChainSupportDetails(struct SwapChainSupportDetails *details) {
    free(details->formats);
    free(details->presentModes);
}

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats, int size);
VkPresentModeKHR
chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, int size);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities,
                            SDL_Window *window);

char *readFile(const char *filename, uint32_t *size);
VkShaderModule createShaderModule(VkDevice device, const char *code,
                                  uint32_t size);

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         VkRenderPass renderPass,
                         VkFramebuffer *swapChainFramebuffers,
                         VkExtent2D swapChainExtent,
                         VkPipeline graphicsPipeline);

/* imitation of object oriented */
struct sl_oo {
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    VkImage *swapChainImages;
    uint32_t swapChainImagesCount;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkImageView *swapChainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkFramebuffer *swapChainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer *commandBuffers;
    VkSemaphore *imageAvailableSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;
    uint32_t currentFrame;

    SDL_Window *window;
};

void createInstance(struct sl_oo *oo);
void setupDebugMessenger(struct sl_oo *oo);
void createSurface(struct sl_oo *oo);
void pickPhysicalDevice(struct sl_oo *oo);
void createLogicalDevice(struct sl_oo *oo);
void createSwapChain(struct sl_oo *oo);
void createImageViews(struct sl_oo *oo);
void createRenderPass(struct sl_oo *oo);
void createGraphicsPipeline(struct sl_oo *oo);
void createFrameBuffers(struct sl_oo *oo);
void createCommandPool(struct sl_oo *oo);
void createCommandBuffer(struct sl_oo *oo);
void createSyncObjects(struct sl_oo *oo);

void recreateSwapChain(struct sl_oo *oo);

void cleanUp(struct sl_oo *oo);

int main(int argc, char *argv[]) {
    int rc = 0;
    bool running = true;

    struct sl_oo oo = { 0 };

    /* init sdl */
    rc = SDL_Init(SDL_INIT_VIDEO);
    if (rc != 0) {
        error_log(SDL_GetError());
        return 1;
    }

    /* init window */
    oo.window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
                                 SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

    if (oo.window == NULL) {
        error_log(SDL_GetError());
        return 1;
    }

    /* init vulkan */
    /* create instance */
    createInstance(&oo);

    /* setup debug messenger */
    setupDebugMessenger(&oo);

    /* create surface */
    createSurface(&oo);

    /* pick physical device */
    pickPhysicalDevice(&oo);

    /* create logical device */
    createLogicalDevice(&oo);

    /* create swap chain */
    createSwapChain(&oo);

    /* create image views */
    createImageViews(&oo);

    /* create render pass */
    createRenderPass(&oo);

    /* create graphics pipeline */
    createGraphicsPipeline(&oo);

    /* create framebuffers */
    createFrameBuffers(&oo);

    /* create command pool */
    createCommandPool(&oo);

    /* create command buffer */
    createCommandBuffer(&oo);

    /* create sync objects */
    createSyncObjects(&oo);

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
        drawFrame(&oo);
    }
    vkDeviceWaitIdle(oo.device);

    /* clean up */
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

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        struct SwapChainSupportDetails swapChainSupport =
            querySwapChainSupport(device, surface);
        /* originally the tutorial check for empty vector here */
        /* in my case, the formats and presentModes will be NULL if
           the malloc did not happened */
        swapChainAdequate = swapChainSupport.formats != NULL &&
                            swapChainSupport.presentModes != NULL;

        /* since we used malloc before, free it using a custom function */
        DestroySwapChainSupportDetails(&swapChainSupport);
    }

    return QueueFamilyIndicesIsComplete(&indices) && extensionsSupported &&
           swapChainAdequate;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

    VkExtensionProperties *availableExtensions =
        malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount,
                                         availableExtensions);

    /* I hate seeing the set in C++ */
    /* again, the tutorial uses a set to eliminate duplicated data */

    /* luckily, this time, it says we can use a loop like
       checkValidationLayerSupport */
    for (int i = 0; i < sizeof(deviceExtensions) / sizeof(char *); i++) {
        bool found = false;
        for (int j = 0; j < extensionCount; j++) {
            if (strcmp(deviceExtensions[i],
                       availableExtensions[j].extensionName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            free(availableExtensions);
            return false;
        }
    }

    free(availableExtensions);

    return true;
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
            /* work around of optional */
            indices.presentFamilyHasValue = true;
        }

        if (QueueFamilyIndicesIsComplete(&indices)) {
            break;
        }
    }

    free(queueFamilies);
    return indices;
}

struct SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                     VkSurfaceKHR surface) {
    struct SwapChainSupportDetails details = { 0 };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);
    if (formatCount != 0) {
        details.formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats);
        details.formatsSize = formatCount;
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, NULL);
    if (presentModeCount != 0) {
        details.presentModes =
            malloc(sizeof(VkPresentModeKHR) * presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes);
        details.presentModesSize = presentModeCount;
    }

    return details;
}

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats, int size) {
    /* make sure availableFormats is not NULL */
    for (int i = 0; i < size; i++) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR
chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, int size) {
    /* make sure availablePresentModes is not NULL */
    for (int i = 0; i < size; i++) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

/* a custom clamp function from https://stackoverflow.com/questions/427477/fastest-way-to-clamp-a-real-fixed-floating-point-value */
uint32_t clamp(uint32_t v, uint32_t lo, uint32_t hi) {
    const uint32_t t = v < lo ? lo : v;
    return t > hi ? hi : t;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities,
                            SDL_Window *window) {
    /* std::numeric_limits<uint32_t>::max() should equal to UINT32_MAX */
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        int width = 0;
        int height = 0;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);

        VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
        actualExtent.width = clamp(actualExtent.width,
                                   capabilities->minImageExtent.width,
                                   capabilities->maxImageExtent.width);
        actualExtent.height = clamp(actualExtent.height,
                                    capabilities->minImageExtent.height,
                                    capabilities->maxImageExtent.height);

        return actualExtent;
    }
}

char *readFile(const char *filename, uint32_t *size) {
    /* this function in not null-terminated */
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        error_log("failed to open file!");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *s = malloc(sizeof(char) * *size);

    int r = fread(s, sizeof(char), *size, fp);
    if (r == 0) {
        error_log("fread error!");
    }

    fclose(fp);

    return s;
}

VkShaderModule createShaderModule(VkDevice device, const char *code,
                                  uint32_t size) {
    VkShaderModuleCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = (uint32_t *)code;

    VkShaderModule shaderModule = NULL;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) !=
        VK_SUCCESS) {
        error_log("failed to create shader module!");
        exit(1);
    }

    return shaderModule;
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                         VkRenderPass renderPass,
                         VkFramebuffer *swapChainFramebuffers,
                         VkExtent2D swapChainExtent,
                         VkPipeline graphicsPipeline) {
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = NULL; // Optional
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        error_log("failed to begin recording command buffer!");
        exit(1);
    }

    VkRenderPassBeginInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    VkOffset2D offset = { 0, 0 };
    renderPassInfo.renderArea.offset = offset;
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = { 0 };
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        error_log("failed to record command buffer!");
        exit(1);
    }
}

void drawFrame(struct sl_oo *oo) {
    vkWaitForFences(oo->device, 1, &oo->inFlightFences[oo->currentFrame],
                    VK_TRUE, UINT64_MAX);
    vkResetFences(oo->device, 1, &oo->inFlightFences[oo->currentFrame]);
    uint32_t imageIndex;
    vkAcquireNextImageKHR(oo->device, oo->swapChain, UINT64_MAX,
                          oo->imageAvailableSemaphores[oo->currentFrame],
                          VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(oo->commandBuffers[oo->currentFrame], 0);
    recordCommandBuffer(oo->commandBuffers[oo->currentFrame], imageIndex,
                        oo->renderPass, oo->swapChainFramebuffers,
                        oo->swapChainExtent, oo->graphicsPipeline);

    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {
        oo->imageAvailableSemaphores[oo->currentFrame]
    };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &oo->commandBuffers[oo->currentFrame];
    VkSemaphore signalSemaphores[] = {
        oo->renderFinishedSemaphores[oo->currentFrame]
    };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(oo->graphicsQueue, 1, &submitInfo,
                      oo->inFlightFences[oo->currentFrame]) != VK_SUCCESS) {
        error_log("failed to submit draw command buffer!");
        exit(1);
    }

    VkPresentInfoKHR presentInfo = { 0 };
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = { oo->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL; // Optional
    vkQueuePresentKHR(oo->presentQueue, &presentInfo);

    oo->currentFrame = (oo->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/****** separation line */

void createInstance(struct sl_oo *oo) {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        error_log("validation layers requested, but not available");
        exit(1);
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

    if (SDL_Vulkan_GetInstanceExtensions(oo->window, &sdlExtensionCount,
                                         sdlExtensions) == SDL_FALSE) {
        error_log("cannot get instance extensions %s", SDL_GetError());
        exit(1);
    }

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    const char **extensions = malloc(sizeof(char **) * extensionCount);
    int extensionIndex = 0;

    extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
    extensionIndex += 1;

#ifdef __APPLE__
    extensions[1] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    extensions[2] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
    extensions[3] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    extensionIndex += 3;
#elif defined __linux__
    extensions[1] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
    extensionIndex += 1;
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

    if (vkCreateInstance(&createInfo, NULL, &oo->instance) != VK_SUCCESS) {
        error_log("failed to create instance");
        exit(1);
    }

    free(extensions);
}

void setupDebugMessenger(struct sl_oo *oo) {
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

        VkResult result = CreateDebugUtilsMessengerEXT(
            oo->instance, &createInfo, NULL, &oo->debugMessenger);
        if (result != VK_SUCCESS) {
            printf("result: %d\n", result);
            error_log("failed to set up debug messenger!");
            exit(1);
        }
    }
}

void createSurface(struct sl_oo *oo) {
    if (SDL_Vulkan_CreateSurface(oo->window, oo->instance, &oo->surface) !=
        SDL_TRUE) {
        error_log("failed to create window surface! %s", SDL_GetError());
        exit(1);
    }
}

void pickPhysicalDevice(struct sl_oo *oo) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(oo->instance, &deviceCount, NULL);
    if (deviceCount == 0) {
        error_log("failed to find GPUs with Vulkan support!");
        exit(1);
    }

    VkPhysicalDevice *devices =
        malloc(sizeof(VkPhysicalDevice *) * deviceCount);
    vkEnumeratePhysicalDevices(oo->instance, &deviceCount, devices);

    for (int i = 0; i < deviceCount; i++) {
        if (isDeviceSuitable(devices[i], oo->surface)) {
            oo->physicalDevice = devices[i];
            break;
        }
    }

    if (oo->physicalDevice == VK_NULL_HANDLE) {
        error_log("failed to find a suitable GPU!");
        exit(1);
    }

    free(devices);
}

void createLogicalDevice(struct sl_oo *oo) {
    struct QueueFamilyIndices indices =
        findQueueFamilies(oo->physicalDevice, oo->surface);

    /* originally here use a set */
    /* we cannot do it in C */
    VkDeviceQueueCreateInfo queueCreateInfos[QUEUE_CREATE_INFOS_SIZE] = { 0 };

    /* the reason why the original tutorial use a set here is
           because the graphicsFamily and the presentFamily may be the
           same, and using a set eliminate the duplicated one */

    /* use a stupid loop to check whether the newly added value is
           unique, we have to separate it into another function */
    int uniqueQueueFamiliesSize = 0;
    uint32_t uniqueQueueFamilies[QUEUE_CREATE_INFOS_SIZE] = { 0 };
    add_to_unique_set(uniqueQueueFamilies, &uniqueQueueFamiliesSize,
                      indices.graphicsFamily);
    add_to_unique_set(uniqueQueueFamilies, &uniqueQueueFamiliesSize,
                      indices.presentFamily);

    float queuePriority = 1.0f;
    for (int i = 0; i < uniqueQueueFamiliesSize; i++) {
        VkDeviceQueueCreateInfo queueCreateInfo = { 0 };
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos[i] = queueCreateInfo;
    }

    VkPhysicalDeviceFeatures deviceFeatures = { 0 };

    VkDeviceCreateInfo createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = QUEUE_CREATE_INFOS_SIZE;
    createInfo.pQueueCreateInfos = queueCreateInfos;
    createInfo.queueCreateInfoCount = uniqueQueueFamiliesSize;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = DEVICE_EXTENSIONS_COUNT;
    createInfo.ppEnabledExtensionNames = deviceExtensions;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            (uint32_t)(sizeof(validationLayers) / sizeof(char *));
        createInfo.ppEnabledLayerNames = validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(oo->physicalDevice, &createInfo, NULL, &oo->device) !=
        VK_SUCCESS) {
        error_log("failed to create logical device!");
        exit(1);
    }

    vkGetDeviceQueue(oo->device, indices.graphicsFamily, 0, &oo->graphicsQueue);
    vkGetDeviceQueue(oo->device, indices.presentFamily, 0, &oo->presentQueue);
}

void createSwapChain(struct sl_oo *oo) {
    struct SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(oo->physicalDevice, oo->surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
        swapChainSupport.formats, swapChainSupport.formatsSize);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(
        swapChainSupport.presentModes, swapChainSupport.presentModesSize);
    VkExtent2D extent =
        chooseSwapExtent(&swapChainSupport.capabilities, oo->window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = { 0 };
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = oo->surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    struct QueueFamilyIndices indices =
        findQueueFamilies(oo->physicalDevice, oo->surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily,
                                      indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = NULL; // Optional
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(oo->device, &createInfo, NULL, &oo->swapChain) !=
        VK_SUCCESS) {
        error_log("failed to create swap chain!");
        exit(1);
    }

    vkGetSwapchainImagesKHR(oo->device, oo->swapChain, &imageCount, NULL);
    oo->swapChainImages = malloc(sizeof(VkImage) * imageCount);
    vkGetSwapchainImagesKHR(oo->device, oo->swapChain, &imageCount,
                            oo->swapChainImages);
    oo->swapChainImagesCount = imageCount;

    oo->swapChainImageFormat = surfaceFormat.format;
    oo->swapChainExtent = extent;
}

void createImageViews(struct sl_oo *oo) {
    oo->swapChainImageViews =
        malloc(sizeof(VkImageView) * oo->swapChainImagesCount);
    /* this malloc is free by vkDestroyImageView */
    for (int i = 0; i < oo->swapChainImagesCount; i++) {
        VkImageViewCreateInfo createInfo = { 0 };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = oo->swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = oo->swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(oo->device, &createInfo, NULL,
                              &oo->swapChainImageViews[i]) != VK_SUCCESS) {
            error_log("failed to create image views!");
            exit(1);
        }
    }
}

void createRenderPass(struct sl_oo *oo) {
    VkAttachmentDescription colorAttachment = { 0 };
    colorAttachment.format = oo->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = { 0 };
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = { 0 };
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(oo->device, &renderPassInfo, NULL,
                           &oo->renderPass) != VK_SUCCESS) {
        error_log("failed to create render pass!");
        exit(1);
    }
}

void createGraphicsPipeline(struct sl_oo *oo) {
    uint32_t vertShaderSize = 0;
    char *vertShaderCode = readFile("shaders/vert.spv", &vertShaderSize);
    uint32_t fragShaderSize = 0;
    char *fragShaderCode = readFile("shaders/frag.spv", &fragShaderSize);

    VkShaderModule vertShaderModule =
        createShaderModule(oo->device, vertShaderCode, vertShaderSize);
    VkShaderModule fragShaderModule =
        createShaderModule(oo->device, fragShaderCode, fragShaderSize);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = { 0 };
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = { 0 };
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo,
                                                       fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { 0 };
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = NULL; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = NULL; // Optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 0 };
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = { 0 };
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = { 0 };
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling = { 0 };
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment = { 0 };
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending = { 0 };
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = { 0 };
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        (uint32_t)2; /* TODO: size of dynamic states */
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = { 0 };
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = NULL; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = NULL; // Optional

    if (vkCreatePipelineLayout(oo->device, &pipelineLayoutInfo, NULL,
                               &oo->pipelineLayout) != VK_SUCCESS) {
        error_log("failed to create pipeline layout!");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = { 0 };
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = NULL; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = oo->pipelineLayout;
    pipelineInfo.renderPass = oo->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(oo->device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  NULL, &oo->graphicsPipeline) != VK_SUCCESS) {
        error_log("failed to create graphics pipeline!");
        exit(1);
    }

    vkDestroyShaderModule(oo->device, fragShaderModule, NULL);
    vkDestroyShaderModule(oo->device, vertShaderModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
}

void createFrameBuffers(struct sl_oo *oo) {
    oo->swapChainFramebuffers =
        malloc(sizeof(VkFramebuffer) * oo->swapChainImagesCount);

    for (int i = 0; i < oo->swapChainImagesCount; i++) {
        VkImageView attachments[] = { oo->swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo = { 0 };
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = oo->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = oo->swapChainExtent.width;
        framebufferInfo.height = oo->swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(oo->device, &framebufferInfo, NULL,
                                &oo->swapChainFramebuffers[i]) != VK_SUCCESS) {
            error_log("failed to create framebuffer!");
            exit(1);
        }
    }
}

void createCommandPool(struct sl_oo *oo) {
    struct QueueFamilyIndices queueFamilyIndices =
        findQueueFamilies(oo->physicalDevice, oo->surface);

    VkCommandPoolCreateInfo poolInfo = { 0 };
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    if (vkCreateCommandPool(oo->device, &poolInfo, NULL, &oo->commandPool) !=
        VK_SUCCESS) {
        error_log("failed to create command pool!");
        exit(1);
    }
}

void createCommandBuffer(struct sl_oo *oo) {
    oo->commandBuffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = oo->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

    if (vkAllocateCommandBuffers(oo->device, &allocInfo, oo->commandBuffers) !=
        VK_SUCCESS) {
        error_log("failed to allocate command buffers!");
        exit(1);
    }
}

void createSyncObjects(struct sl_oo *oo) {
    oo->imageAvailableSemaphores =
        malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    oo->renderFinishedSemaphores =
        malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    oo->inFlightFences = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = { 0 };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(oo->device, &semaphoreInfo, NULL,
                              &oo->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(oo->device, &semaphoreInfo, NULL,
                              &oo->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(oo->device, &fenceInfo, NULL,
                          &oo->inFlightFences[i]) != VK_SUCCESS) {
            error_log("failed to create semaphores!");
            exit(1);
        }
    }
}

void cleanUp(struct sl_oo *oo) {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(oo->device, oo->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(oo->device, oo->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(oo->device, oo->inFlightFences[i], NULL);
    }
    free(oo->imageAvailableSemaphores);
    free(oo->renderFinishedSemaphores);
    free(oo->inFlightFences);

    vkDestroyCommandPool(oo->device, oo->commandPool, NULL);
    free(oo->commandBuffers);

    for (int i = 0; i < oo->swapChainImagesCount; i++) {
        vkDestroyFramebuffer(oo->device, oo->swapChainFramebuffers[i], NULL);
    }
    free(oo->swapChainFramebuffers);

    vkDestroyPipeline(oo->device, oo->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(oo->device, oo->pipelineLayout, NULL);
    vkDestroyRenderPass(oo->device, oo->renderPass, NULL);

    for (int i = 0; i < oo->swapChainImagesCount; i++) {
        vkDestroyImageView(oo->device, oo->swapChainImageViews[i], NULL);
    }
    free(oo->swapChainImageViews);

    free(oo->swapChainImages);
    vkDestroySwapchainKHR(oo->device, oo->swapChain, NULL);
    vkDestroyDevice(oo->device, NULL);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(oo->instance, oo->debugMessenger, NULL);
    }

    vkDestroySurfaceKHR(oo->instance, oo->surface, NULL);
    vkDestroyInstance(oo->instance, NULL);

    SDL_DestroyWindow(oo->window);
    SDL_Quit();
}

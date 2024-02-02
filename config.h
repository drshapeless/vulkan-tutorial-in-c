#ifndef CONFIG_H
#define CONFIG_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

#define WINDOW_NAME "vulkan-tutorial"
#define MSG_LEN 1024
static const int WIDTH = 800;
static const int HEIGHT = 600;

#define QUEUE_CREATE_INFOS_SIZE 2
#define DEVICE_EXTENSIONS_COUNT 1

static const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
static const char *deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#endif /* CONFIG_H */

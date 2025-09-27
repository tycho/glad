{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_VULKAN

GLAD_API_CALL int gladLoaderLoadVulkanContext({{ template_utils.context_arg(',') }} VkInstance instance, VkPhysicalDevice physical_device, VkDevice device);
GLAD_API_CALL void gladLoaderUnloadVulkanContext({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderResetVulkanContext({{ template_utils.context_arg() }});

GLAD_API_CALL int gladLoaderLoadVulkan(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device);
GLAD_API_CALL void gladLoaderUnloadVulkan(void);
GLAD_API_CALL void gladLoaderResetVulkan(void);

#endif

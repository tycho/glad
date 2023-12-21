{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_VULKAN

GLAD_API_CALL int gladLoaderLoadVulkan{{ 'Context' if options.mx }}({{ template_utils.context_arg(',') }} VkInstance instance, VkPhysicalDevice physical_device, VkDevice device);
GLAD_API_CALL void gladLoaderUnloadVulkan{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
GLAD_API_CALL void gladLoaderResetVulkan{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }});
{% if options.mx_global %}
GLAD_API_CALL int gladLoaderLoadVulkan(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device);
GLAD_API_CALL void gladLoaderUnloadVulkan(void);
GLAD_API_CALL void gladLoaderResetVulkan(void);
{% endif %}

#endif

{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_VULKAN

{% set loader_handle = template_utils.handle('vulkan') %}
{% include 'loader/library.c' %}


static uint64_t DEVICE_COMMANDS[] = {
{% for command in device_commands | sort(attribute=hash_sort_key) %}
    {{ command.hash }}, /* {{ command.name }} */
{% endfor %}
};

static int glad_vulkan_is_device_command(uint64_t nameHash) {
    /* Exists as a workaround for:
     * https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/issues/2323
     *
     * `vkGetDeviceProcAddr` does not return NULL for non-device functions.
     */
    return glad_hash_search(DEVICE_COMMANDS, GLAD_ARRAYSIZE(DEVICE_COMMANDS), nameHash);
}

static uint64_t GLOBAL_COMMANDS[] = {
{% for command in global_commands | sort(attribute=hash_sort_key) %}
    {{ command.hash }}, /* {{ command.name }} */
{% endfor %}
};

static int glad_vulkan_is_global_command(uint64_t nameHash) {
    return glad_hash_search(GLOBAL_COMMANDS, GLAD_ARRAYSIZE(GLOBAL_COMMANDS), nameHash);
}

struct _glad_vulkan_userptr {
    void *vk_handle;
    VkInstance vk_instance;
    VkDevice vk_device;
    PFN_vkGetInstanceProcAddr get_instance_proc_addr;
    PFN_vkGetDeviceProcAddr get_device_proc_addr;
};

static GLADapiproc glad_vulkan_get_proc(void *vuserptr, const char *name) {
    struct _glad_vulkan_userptr userptr = *(struct _glad_vulkan_userptr*) vuserptr;
    uint64_t nameHash = glad_hash_string(name, strlen(name));
    PFN_vkVoidFunction result = NULL;

    if (userptr.vk_device != NULL && glad_vulkan_is_device_command(nameHash)) {
        result = userptr.get_device_proc_addr(userptr.vk_device, name);
    } else {
        bool is_global_command = glad_vulkan_is_global_command(nameHash);
        if (is_global_command) {
            result = userptr.get_instance_proc_addr(NULL, name);
        } else if (userptr.vk_instance != NULL) {
            result = userptr.get_instance_proc_addr(userptr.vk_instance, name);
        }
    }

    return (GLADapiproc) result;
}


{% if not options.mx %}
static void* {{ loader_handle }} = NULL;
{% endif %}

static void* glad_vulkan_dlopen_handle({{ template_utils.context_arg(def='void') }}) {
    static const char *NAMES[] = {
#if GLAD_PLATFORM_APPLE
        "libvulkan.dylib",
        "libvulkan.1.dylib",
        "libMoltenVK.dylib",
#elif GLAD_PLATFORM_WIN32
        "vulkan-1.dll",
#else
        "libvulkan.so.1",
        "libvulkan.so",
#endif
    };

    if ({{ loader_handle }} == NULL) {
        {{ loader_handle }} = glad_get_dlopen_handle(NAMES, GLAD_ARRAYSIZE(NAMES));
    }

    return {{ loader_handle }};
}

static struct _glad_vulkan_userptr glad_vulkan_build_userptr(void *handle, VkInstance instance, VkDevice device) {
    struct _glad_vulkan_userptr userptr;
    userptr.vk_handle = handle;
    userptr.vk_instance = instance;
    userptr.vk_device = device;
    userptr.get_instance_proc_addr = (PFN_vkGetInstanceProcAddr) glad_dlsym_handle(handle, "vkGetInstanceProcAddr");
    userptr.get_device_proc_addr = (PFN_vkGetDeviceProcAddr) glad_dlsym_handle(handle, "vkGetDeviceProcAddr");
    return userptr;
}

int gladLoaderLoadVulkan{{ 'Context' if options.mx }}({{ template_utils.context_arg(',') }} VkInstance instance, VkPhysicalDevice physical_device, VkDevice device) {
    int version = 0;
    void *handle = NULL;
    int did_load = 0;
    struct _glad_vulkan_userptr userptr;

    did_load = {{ loader_handle }} == NULL;
    handle = glad_vulkan_dlopen_handle({{ 'context' if options.mx }});
    if (handle != NULL) {
        userptr = glad_vulkan_build_userptr(handle, instance, device);

        if (userptr.get_instance_proc_addr != NULL && userptr.get_device_proc_addr != NULL) {
            version = gladLoadVulkan{{ 'Context' if options.mx }}UserPtr({{ 'context,' if options.mx }} physical_device, glad_vulkan_get_proc, &userptr);
        }

        if (!version && did_load) {
            gladLoaderUnloadVulkan{{ 'Context' if options.mx }}({{ 'context' if options.mx }});
        }
    }

    return version;
}

void gladLoaderResetVulkan{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
{% if options.mx %}
    memset(context, 0, sizeof(GladVulkanContext));
{% else %}
{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = 0;
{% endfor %}

{% for extension in feature_set.extensions %}
{% call template_utils.protect(extension) %}
    {{ ('GLAD_' + extension.name)|ctx(name_only=True) }} = 0;
{% endcall %}
{% endfor %}

{% for extension, commands in loadable() %}
{% for command in commands %}
{% call template_utils.protect(command) %}
    {{ command.name|ctx }} = NULL;
{% endcall %}
{% endfor %}
{% endfor %}
{% endif %}
}

{% if options.mx_global %}
void gladLoaderResetVulkan(void) {
    gladLoaderResetVulkanContext(gladGetVulkanContext());
}
{% endif %}

{% if options.mx_global %}
int gladLoaderLoadVulkan(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device) {
    return gladLoaderLoadVulkanContext(gladGetVulkanContext(), instance, physical_device, device);
}
{% endif %}

void gladLoaderUnloadVulkan{{ 'Context' if options.mx }}({{ template_utils.context_arg(def='void') }}) {
    if ({{ loader_handle }} != NULL) {
        glad_close_dlopen_handle({{ loader_handle }});
        {{ loader_handle }} = NULL;
    }

{% if not options.mx %}
    gladLoaderResetVulkan();
{% else %}
    gladLoaderResetVulkanContext(context);
{% endif %}
}

{% if options.mx_global %}
void gladLoaderUnloadVulkan(void) {
    gladLoaderUnloadVulkanContext(gladGetVulkanContext());
}
{% endif %}

#endif /* GLAD_VULKAN */

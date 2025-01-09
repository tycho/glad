{% extends 'base_template.c' %}

{% block commandidx %}
{%if options.no_extension_detection %}
static uint16_t glad_vk_global_commands[] = {
{% for command in global_commands | sort(attribute='index') %}
    {{ "{:4}".format(command.index) }}{% if not loop.last %},{% else %}{{" "}}{% endif %} /* {{ command.name }} */
{% endfor %}
};

static uint16_t glad_vk_instance_commands[] = {
{% for command in instance_commands | sort(attribute='index') %}
    {{ "{:4}".format(command.index) }}{% if not loop.last %},{% else %}{{" "}}{% endif %} /* {{ command.name }} */
{% endfor %}
};

static uint16_t glad_vk_device_commands[] = {
{% for command in device_commands | sort(attribute='index') %}
    {{ "{:4}".format(command.index) }}{% if not loop.last %},{% else %}{{" "}}{% endif %} /* {{ command.name }} */
{% endfor %}
};

{% endif %}
{% endblock %}
{% block extnames %}
{% endblock %}
{% block loader %}
{% if not options.no_extension_detection %}
static int glad_vk_get_extensions({{ template_utils.context_arg(',') }} VkPhysicalDevice physical_device, uint32_t *out_extension_count, uint64_t **out_extensions) {
    uint32_t i;
    uint32_t instance_extension_count = 0;
    uint32_t device_extension_count = 0;
    uint32_t max_extension_count = 0;
    uint32_t total_extension_count = 0;
    uint64_t *extensions = NULL;
    VkExtensionProperties *ext_properties = NULL;
    VkResult result;

    if ({{ 'vkEnumerateInstanceExtensionProperties'|ctx }} == NULL || (physical_device != NULL && {{ 'vkEnumerateDeviceExtensionProperties'|ctx }} == NULL)) {
        return 0;
    }

    result = {{ 'vkEnumerateInstanceExtensionProperties'|ctx }}(NULL, &instance_extension_count, NULL);
    if (result != VK_SUCCESS) {
        return 0;
    }

    if (physical_device != NULL) {
        result = {{ 'vkEnumerateDeviceExtensionProperties'|ctx }}(physical_device, NULL, &device_extension_count, NULL);
        if (result != VK_SUCCESS) {
            return 0;
        }
    }

    total_extension_count = instance_extension_count + device_extension_count;
    if (total_extension_count <= 0) {
        return 0;
    }

    max_extension_count = instance_extension_count > device_extension_count
        ? instance_extension_count : device_extension_count;

    ext_properties = (VkExtensionProperties*) malloc(max_extension_count * sizeof(VkExtensionProperties));
    if (ext_properties == NULL) {
        goto glad_vk_get_extensions_error;
    }

    result = {{ 'vkEnumerateInstanceExtensionProperties'|ctx }}(NULL, &instance_extension_count, ext_properties);
    if (result != VK_SUCCESS) {
        goto glad_vk_get_extensions_error;
    }

    extensions = (uint64_t *)calloc(total_extension_count, sizeof(uint64_t));
    if (extensions == NULL) {
        goto glad_vk_get_extensions_error;
    }

    for (i = 0; i < instance_extension_count; ++i) {
        VkExtensionProperties ext = ext_properties[i];
        size_t extension_name_length = strlen(ext.extensionName);
        extensions[i] = glad_hash_string(ext.extensionName, extension_name_length * sizeof(char));
    }

    if (physical_device != NULL) {
        result = {{ 'vkEnumerateDeviceExtensionProperties'|ctx }}(physical_device, NULL, &device_extension_count, ext_properties);
        if (result != VK_SUCCESS) {
            goto glad_vk_get_extensions_error;
        }

        for (i = 0; i < device_extension_count; ++i) {
            VkExtensionProperties ext = ext_properties[i];
            size_t extension_name_length = strlen(ext.extensionName);
            extensions[instance_extension_count + i] = glad_hash_string(ext.extensionName, extension_name_length * sizeof(char));
        }
    }

{% if search_type == 0 %}
    qsort(extensions, total_extension_count, sizeof(uint64_t), compare_uint64);

{% endif %}
    if (instance_extension_count)
        {{ 'glad_found_instance_exts'|ctx }} = 1;
    if (device_extension_count)
        {{ 'glad_found_device_exts'|ctx }} = 1;

    free((void*) ext_properties);

    *out_extension_count = total_extension_count;
    *out_extensions = extensions;

    return 1;

glad_vk_get_extensions_error:
    free((void*) ext_properties);
    free(extensions);
    return 0;
}

static void glad_vk_free_extensions(uint64_t *extensions) {
    free((void*) extensions);
}

static int glad_vk_has_extension(uint64_t *extensions, uint64_t extension_count, uint64_t name) {
    return glad_hash_search(extensions, extension_count, name);
}

{% endif %}
static GLADapiproc glad_vk_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
{% if not options.no_extension_detection %}
static int glad_vk_find_extensions_{{ api|lower }}({{ template_utils.context_arg(',') }} VkPhysicalDevice physical_device) {
{% if feature_set.extensions|length > 0 %}
{% if not feature_set.extensions|index_consecutive_0_to_N %}
    static const uint16_t extIdx[] = {
{% for extension in feature_set.extensions %}
        {{ "{:>4}".format(extension.index) }}, /* {{ extension.name }} */
{% endfor %}
    };
{% endif %}
    uint32_t extension_count = 0;
    uint32_t i;
    uint64_t *extensions = NULL;

    if (physical_device && {{ 'glad_found_device_exts'|ctx }})
        return 1;
    if (!physical_device && {{ 'glad_found_instance_exts'|ctx }})
        return 1;

    if (!glad_vk_get_extensions({{'context, ' if options.mx }}physical_device, &extension_count, &extensions)) return 0;

    #ifdef __clang__
    #pragma nounroll
    #endif
{# If the list is a consecutive 0 to N list, we can just scan the whole thing without emitting an array. #}
{% if feature_set.extensions|index_consecutive_0_to_N %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_hashes); ++i)
        context->extArray[i] = glad_vk_has_extension(extensions, extension_count, GLAD_{{ feature_set.name|api }}_ext_hashes[i]);
{% else %}
    for (i = 0; i < GLAD_ARRAYSIZE(extIdx); ++i)
        context->extArray[extIdx[i]] = glad_vk_has_extension(extensions, extension_count, GLAD_{{ feature_set.name|api }}_ext_hashes[extIdx[i]]);
{% endif %}

    {# Special case: only one extension which is protected -> unused at compile time only on some platforms #}
    GLAD_UNUSED(glad_vk_has_extension);

    glad_vk_free_extensions(extensions);
{% else %}
{% if options.mx %}
    GLAD_UNUSED(context);
{% endif %}
    GLAD_UNUSED(physical_device);
    GLAD_UNUSED(glad_vk_get_extensions);
    GLAD_UNUSED(glad_vk_has_extension);
    GLAD_UNUSED(glad_vk_free_extensions);
    GLAD_UNUSED(GLAD_{{ feature_set.name|api }}_ext_hashes);
{% endif %}
    return 1;
}

static int glad_vk_find_core_{{ api|lower }}({{ template_utils.context_arg(',') }} VkPhysicalDevice physical_device) {
    int major = 1;
    int minor = 0;

#ifdef VK_VERSION_1_1
    if (!{{ 'glad_vk_instance_version'|ctx }} && {{ 'vkEnumerateInstanceVersion'|ctx }} != NULL) {
        VkResult result;

        result = {{ 'vkEnumerateInstanceVersion'|ctx }}(&{{ 'glad_vk_instance_version'|ctx }});
        if (result != VK_SUCCESS)
            {{ 'glad_vk_instance_version'|ctx }} = 0;
    }
    major = (int) VK_VERSION_MAJOR({{ 'glad_vk_instance_version'|ctx }});
    minor = (int) VK_VERSION_MINOR({{ 'glad_vk_instance_version'|ctx }});
#endif

    if (!{{ 'glad_vk_device_version'|ctx }}) {
        if (physical_device != NULL && {{ 'vkGetPhysicalDeviceProperties'|ctx }} != NULL) {
            VkPhysicalDeviceProperties properties;
            {{ 'vkGetPhysicalDeviceProperties'|ctx }}(physical_device, &properties);
            {{ 'glad_vk_device_version'|ctx }} = properties.apiVersion;
        }
    }
    if ({{'glad_vk_device_version'|ctx}}) {
        major = (int) VK_VERSION_MAJOR({{ 'glad_vk_device_version'|ctx }});
        minor = (int) VK_VERSION_MINOR({{ 'glad_vk_device_version'|ctx }});
    }

{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = (major == {{ feature.version.major }} && minor >= {{ feature.version.minor }}) || major > {{ feature.version.major }};
{% endfor %}

    return GLAD_MAKE_VERSION(major, minor);
}

{% endif %}
GLAD_NO_INLINE int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADuserptrloadfunc load, void *userptr) {
    int version;
{% if options.no_extension_detection %}
    uint32_t i;

    (void)physical_device;

    version = 1;

#ifdef VK_VERSION_1_0
    {{ 'vkGetInstanceProcAddr'|ctx }} = (PFN_vkGetInstanceProcAddr)load(userptr, "vkGetInstanceProcAddr");
    {{ 'vkGetDeviceProcAddr'|ctx }} = (PFN_vkGetDeviceProcAddr)load(userptr, "vkGetDeviceProcAddr");
#endif

    if ({{ 'vkGetInstanceProcAddr'|ctx }}) {
        #ifdef __clang__
        #pragma nounroll
        #endif
        for (i = 0; i < GLAD_ARRAYSIZE(glad_vk_global_commands); ++i) {
            const uint16_t pfnIdx = glad_vk_global_commands[i];
            context->pfnArray[pfnIdx] = {{ 'vkGetInstanceProcAddr'|ctx }}(NULL, GLAD_{{ feature_set.name|api}}_fn_names[pfnIdx]);
        }
    }
{% else %}

    (void)instance;
    (void)device;

#ifdef VK_VERSION_1_1
    {{ 'vkEnumerateInstanceVersion'|ctx }} = (PFN_vkEnumerateInstanceVersion)load(userptr, "vkEnumerateInstanceVersion");
#endif

    version = glad_vk_find_core_{{ api|lower }}({{ 'context,' if options.mx }} physical_device);
    if (!version) {
        return 0;
    }
{% endif %}

{% if options.no_extension_detection %}
    if (instance != NULL && {{ 'vkGetInstanceProcAddr'|ctx }} != NULL) {
        #ifdef __clang__
        #pragma nounroll
        #endif
        for (i = 0; i < GLAD_ARRAYSIZE(glad_vk_instance_commands); ++i) {
            const uint16_t pfnIdx = glad_vk_instance_commands[i];
            context->pfnArray[pfnIdx] = {{ 'vkGetInstanceProcAddr'|ctx }}(instance, GLAD_{{ feature_set.name|api}}_fn_names[pfnIdx]);
        }
    }

    if (device != NULL && {{ 'vkGetDeviceProcAddr'|ctx }} != NULL) {
        #ifdef __clang__
        #pragma nounroll
        #endif
        for (i = 0; i < GLAD_ARRAYSIZE(glad_vk_device_commands); ++i) {
            const uint16_t pfnIdx = glad_vk_device_commands[i];
            context->pfnArray[pfnIdx] = {{ 'vkGetDeviceProcAddr'|ctx }}(device, GLAD_{{ feature_set.name|api}}_fn_names[pfnIdx]);
        }
    }

{% else %}
{% for feature, _ in loadable(feature_set.features) %}
    glad_vk_load_{{ feature.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

    if (!glad_vk_find_extensions_{{ api|lower }}({{ 'context,' if options.mx }} physical_device)) return 0;
{% for extension, _ in loadable(feature_set.extensions) %}
{% call template_utils.protect(extension) %}
    glad_vk_load_{{ extension.name }}({{'context, ' if options.mx }}load, userptr);
{% endcall %}
{% endfor %}

{% endif %}
{% if options.mx_global %}
    gladSet{{ api|api }}Context(context);
{% endif %}

{%- if options.alias %}
    glad_vk_resolve_aliases({{ 'context' if options.mx }});
{% endif %}

    return version;
}

{% if options.mx_global %}
int gladLoad{{ api|api }}UserPtr(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ api|api }}Context(), instance, physical_device, device, load, userptr);
}
{% endif %}

int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADloadfunc load) {
    return gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{'context, ' if options.mx }}instance, physical_device, device, glad_vk_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

{% if options.mx_global %}
int gladLoad{{ api|api }}(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ api|api }}Context(), instance, physical_device, device, load);
}
{% endif %}
{% endfor %}

{% if options.mx_global %}
Glad{{ feature_set.name|api }}Context* gladGet{{ feature_set.name|api }}Context() {
    return &{{ global_context }};
}

void gladSet{{ feature_set.name|api }}Context(Glad{{ feature_set.name|api }}Context *context) {
    if (!context) return;
    if (&{{ global_context }} == context) return;
    {{ global_context }} = *context;
}
{% endif %}

{% endblock %}

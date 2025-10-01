{% extends 'base_template.c' %}

{% block funcscopes %}
static const uint8_t GLAD_{{ feature_set.name|api }}_fn_scopes[] = {
{% for command in feature_set.commands %}
    /* {{ "{:>4}".format(command.index)}} */ CommandScope{{ "{:<8s}".format(command.scope) }}, /* {{ command.name }} */
{% endfor %}
};

{% endblock %}

{% block extnames %}
{% endblock %}
{% block pfn_loader %}
{% if options.use_pfn_ranges %}
static void glad_{{ spec.name }}_load_pfn_range({{ template_utils.context_arg(', ') }}GLADvkuserptrloadfunc load, void* userptr, uint16_t pfnStart, uint32_t numPfns)
{
    uint32_t pfnIdx;

    for (pfnIdx = pfnStart; pfnIdx < pfnStart + numPfns; ++pfnIdx) {
        const char *name = GLAD_{{ feature_set.name|api }}_fn_names[pfnIdx];
        const enum GLADcommandscope scope = (enum GLADcommandscope)GLAD_{{ feature_set.name|api }}_fn_scopes[pfnIdx];
        context->pfnArray[pfnIdx] = (void *)load(userptr, name, scope);
    }
}
{% else  %}
static void glad_{{ spec.name }}_load_pfns({{ template_utils.context_arg(', ') }}GLADvkuserptrloadfunc load, void* userptr, const uint16_t *pPfnIdx, uint32_t numPfns)
{
    uint32_t i;

    for (i = 0; i < numPfns; ++i) {
        const uint16_t pfnIdx = pPfnIdx[i];
        const char *name = GLAD_{{ feature_set.name|api }}_fn_names[pfnIdx];
        const enum GLADcommandscope scope = (enum GLADcommandscope)GLAD_{{ feature_set.name|api }}_fn_scopes[pfnIdx];
        context->pfnArray[pfnIdx] = (void *)load(userptr, name, scope);
    }
}

{% endif %}
{% endblock %}
{% block extension_loaders %}
{% if not options.use_pfn_ranges %}
{% for extension, commands in loadable() %}
{% call template_utils.protect(extension) %}
static void glad_{{ spec.name }}_load_{{ extension.name }}({{ template_utils.context_arg(', ') }}GLADvkuserptrloadfunc load, void* userptr) {
    static const uint16_t s_pfnIdx[] = {
{% for command in commands|sort(attribute='index') %}
        {{ "{:>4}".format(command.index) }}{% if not loop.last %},{% else %} {% endif %} /* {{ command.name }} */
{% endfor %}
    };
    if (!{{ ('GLAD_' + extension.name)|ctx(name_only=True) }}) return;
    glad_{{ spec.name }}_load_pfns(context, load, userptr, s_pfnIdx, GLAD_ARRAYSIZE(s_pfnIdx));
}

{% endcall %}
{% endfor %}
{% endif %}
{% endblock %}
{% block loader %}
{% if feature_set.extensions|length > 0 %}
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
    /* Sort extension list for binary search */
    glad_sort_hashes(extensions, total_extension_count);

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
static GLADapiproc glad_vk_get_proc_from_userptr(void *userptr, const char* name, enum GLADcommandscope scope) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *, enum GLADcommandscope)) userptr)(name, scope);
}

{% for api in feature_set.info.apis %}
{% if feature_set.extensions|length > 0 %}
static int glad_vk_find_extensions_{{ api|lower }}({{ template_utils.context_arg(',') }} VkPhysicalDevice physical_device) {
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

    if (!glad_vk_get_extensions(context, physical_device, &extension_count, &extensions)) return 0;

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
    return 1;
}

{% endif %}
static int glad_vk_find_core_{{ api|lower }}({{ template_utils.context_arg(',') }} VkPhysicalDevice physical_device) {
    const uint32_t API_VARIANT_MASK = 0xe0000000;
    int major = 1;
    int minor = 0;
    uint16_t version_value;

#ifdef VK_VERSION_1_1
    if (!{{ 'glad_vk_instance_version'|ctx }} && {{ 'vkEnumerateInstanceVersion'|ctx }} != NULL) {
        VkResult result;

        result = {{ 'vkEnumerateInstanceVersion'|ctx }}(&{{ 'glad_vk_instance_version'|ctx }});
        if (result != VK_SUCCESS)
            {{ 'glad_vk_instance_version'|ctx }} = 0;
        {{ 'glad_vk_instance_version'|ctx }} &= ~API_VARIANT_MASK;
    }
    major = (int) VK_VERSION_MAJOR({{ 'glad_vk_instance_version'|ctx }});
    minor = (int) VK_VERSION_MINOR({{ 'glad_vk_instance_version'|ctx }});
#endif

    if (!{{ 'glad_vk_device_version'|ctx }}) {
        if (physical_device != NULL && {{ 'vkGetPhysicalDeviceProperties'|ctx }} != NULL) {
            VkPhysicalDeviceProperties properties;
            {{ 'vkGetPhysicalDeviceProperties'|ctx }}(physical_device, &properties);
            {{ 'glad_vk_device_version'|ctx }} = properties.apiVersion;
            {{ 'glad_vk_device_version'|ctx }} &= ~API_VARIANT_MASK;
        }
    }
    if ({{'glad_vk_device_version'|ctx}}) {
        major = (int) VK_VERSION_MAJOR({{ 'glad_vk_device_version'|ctx }});
        minor = (int) VK_VERSION_MINOR({{ 'glad_vk_device_version'|ctx }});
    }

    version_value = (major << 8U) | minor;

{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = version_value >= 0x{{ '%02x%02x'|format(feature.version.major, feature.version.minor) }};
{% endfor %}

    return GLAD_MAKE_VERSION(major, minor);
}

GLAD_NO_INLINE int gladLoad{{ api|api }}ContextUserPtr({{ template_utils.context_arg(', ') }}VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkuserptrloadfunc load, void *userptr) {
    int version;
{% if options.use_pfn_ranges %}
    uint32_t i;
{% endif %}

    (void)instance;
    (void)device;

#ifdef VK_VERSION_1_1
    {{ 'vkEnumerateInstanceVersion'|ctx }} = (PFN_vkEnumerateInstanceVersion)load(userptr, "vkEnumerateInstanceVersion", CommandScopeGlobal);
#endif

    version = glad_vk_find_core_{{ api|lower }}(context, physical_device);
    if (!version) {
        return 0;
    }

{% if options.use_pfn_ranges %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_feature_pfn_ranges); ++i) {
        const GladPfnRange_t *range = &GLAD_{{ feature_set.name|api }}_feature_pfn_ranges[i];
        if (context->featArray[range->extension]) {
            glad_{{ spec.name }}_load_pfn_range(context, load, userptr, range->start, range->count);
        }
    }
{% else %}
{% for feature, _ in loadable(feature_set.features) %}
    glad_vk_load_{{ feature.name }}(context, load, userptr);
{% endfor %}
{% endif %}

{% if feature_set.extensions|length > 0 %}
    if (!glad_vk_find_extensions_{{ api|lower }}(context, physical_device)) return 0;

{% if options.use_pfn_ranges %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ api|lower }}_ext_pfn_ranges); ++i) {
        const GladPfnRange_t *range = &GLAD_{{ api|lower }}_ext_pfn_ranges[i];
        if (context->extArray[range->extension]) {
            glad_{{ spec.name }}_load_pfn_range(context, load, userptr, range->start, range->count);
        }
    }
{% else %}
{% for extension, _ in loadable(feature_set.extensions) %}
{% call template_utils.protect(extension) %}
    glad_vk_load_{{ extension.name }}(context, load, userptr);
{% endcall %}
{% endfor %}
{% endif %}

{% endif %}
    gladSet{{ api|api }}Context(context);

{% if options.alias %}
    glad_vk_resolve_aliases(context);

{% endif %}
    return version;
}

int gladLoad{{ api|api }}UserPtr(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ api|api }}Context(), instance, physical_device, device, load, userptr);
}

int gladLoad{{ api|api }}Context({{ template_utils.context_arg(', ') }}VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkloadfunc load) {
    return gladLoad{{ api|api }}ContextUserPtr(context, instance, physical_device, device, glad_vk_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

int gladLoad{{ api|api }}(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ api|api }}Context(), instance, physical_device, device, load);
}
{% endfor %}

Glad{{ feature_set.name|api }}Context* gladGet{{ feature_set.name|api }}Context() {
    return &{{ global_context }};
}

void gladSet{{ feature_set.name|api }}Context(Glad{{ feature_set.name|api }}Context *context) {
    if (!context) return;
    if (&{{ global_context }} == context) return;
    {{ global_context }} = *context;
}

{% endblock %}

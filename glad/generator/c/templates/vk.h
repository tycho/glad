{% extends 'base_template.h' %}

{% block extracontextmembers %}
    uint32_t glad_vk_instance_version;
    uint32_t glad_vk_device_version;
    int glad_found_instance_exts;
    int glad_found_device_exts;
{% endblock %}

{% block header %}
{{ template_utils.header_error(feature_set.name, feature_set.name.upper() + '_H_', name) }}
{{ template_utils.header_error(feature_set.name, feature_set.name.upper() + '_CORE_H_', name) }}
{% endblock %}


{% block custom_declarations %}

/* The scope of this command, i.e. whether it's a global, device or instance
 * function.
 */
enum GLADcommandscope {
    CommandScopeUnknown = 0,
    CommandScopeGlobal,
    CommandScopeDevice,
    CommandScopeInstance,
};

typedef GLADapiproc (*GLADvkuserptrloadfunc)(void *userptr, const char *name, enum GLADcommandscope type);
typedef GLADapiproc (*GLADvkloadfunc)(const char *name, enum GLADcommandscope type);

{% for api in feature_set.info.apis %}
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkloadfunc load);
{% endfor %}

{% if options.mx_global %}
GLAD_API_CALL int gladLoad{{ feature_set.name|api }}UserPtr(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ feature_set.name|api }}(VkInstance instance, VkPhysicalDevice physical_device, VkDevice device, GLADvkloadfunc load);
{% endif %}
{% endblock %}

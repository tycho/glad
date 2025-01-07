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
{% for api in feature_set.info.apis %}
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}VkPhysicalDevice physical_device, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}VkPhysicalDevice physical_device, GLADloadfunc load);
{% endfor %}

{% if options.mx_global %}
GLAD_API_CALL int gladLoad{{ feature_set.name|api }}UserPtr(VkPhysicalDevice physical_device, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ feature_set.name|api }}(VkPhysicalDevice physical_device, GLADloadfunc load);
{% endif %}
{% endblock %}

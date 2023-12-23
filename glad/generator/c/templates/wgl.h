{% extends 'base_template.h' %}
{% block preheader %}
#ifdef _WIN32
{% endblock %}
{% block header %}
#include <windows.h>
#include <glad/gl.h>
{% endblock %}
{% block beforecommands %}
/* These are defined in wingdi.h and we're redefining them. */
#undef wglUseFontBitmaps
#undef wglUseFontOutlines
{% endblock %}
{% block commands %}
{% set blacklist = feature_set.features[0].get_requirements(spec, feature_set=feature_set).commands %}
{{ template_utils.write_function_typedefs(feature_set.commands) }}
{% if not options.mx %}
#ifdef __INTELLISENSE__
{{ template_utils.write_function_definitions(feature_set.commands|reject('existsin', blacklist)) }}
#else
{{ template_utils.write_function_declarations(feature_set.commands|reject('existsin', blacklist)) }}
#endif
{% else %}
typedef struct Glad{{ feature_set.name|api }}Context {
    void* userptr;

{% for extension in feature_set.features %}
    unsigned char {{ extension.name|ctx(member=True) }};
{% endfor %}

    union {
        unsigned char extArray[{{feature_set.extensions|length}}];
        struct {
{% for extension in feature_set.extensions %}
        /* {{ "{:>4}".format(extension.index)}} */ unsigned char {{ extension.name|ctx(member=True) }};
{% endfor %}
        };
    };

    union {
        void *pfnArray[{{ feature_set.commands|length }}];
        struct {
{% for command in feature_set.commands %}
{% call template_utils.protect_pfn(command) %}
        /* {{ "{:>4}".format(command.index) }} */ {{ command.name|pfn }} {{ command.name|ctx(member=True) }};
{% endcall %}
{% endfor %}
        };
    };

{% if options.loader %}
    void* glad_loader_handle;
{% endif %}
} Glad{{ feature_set.name|api }}Context;

{% if options.mx_global %}
GLAD_API_CALL Glad{{ feature_set.name|api }}Context glad_{{ feature_set.name }}_context;

{% for extension in chain(feature_set.features, feature_set.extensions) %}
#define GLAD_{{ extension.name }} (glad_{{ feature_set.name }}_context.{{ extension.name|no_prefix }})
{% endfor %}

#ifdef __INTELLISENSE__
{{ template_utils.write_function_definitions(feature_set.commands|reject('existsin', blacklist)) }}
#else
{% for command in feature_set.commands|reject('existsin', blacklist) %}
#define {{ command.name }} (glad_{{ feature_set.name }}_context.{{ command.name|no_prefix }})
{% endfor %}
#endif
{% endif %}

{% endif %}
{% endblock %}{# commands #}
{% block custom_declarations %}
{% for api in feature_set.info.apis %}
GLAD_API_CALL int gladLoad{{ api|api }}UserPtr(HDC hdc, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}(HDC hdc, GLADloadfunc load);
{% endfor %}
{% endblock %}
{% block postheader %}
#endif
{% endblock %}

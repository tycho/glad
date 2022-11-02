{% extends 'base_template.h' %}

{% block custom_declarations %}
{% for api in feature_set.info.apis %}
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(',') }} EGLDisplay display, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(',') }} EGLDisplay display, GLADloadfunc load);

{% if options.mx_global %}
GLAD_API_CALL int gladLoad{{ api|api }}UserPtr(EGLDisplay display, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}(EGLDisplay display, GLADloadfunc load);
{% endif %}
{% endfor %}
{% endblock %}

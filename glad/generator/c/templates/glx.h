{% extends 'base_template.h' %}

{% block preheader %}
#ifdef __linux__
{% endblock %}

{% block header %}
{{ template_utils.header_error(feature_set.name, feature_set.name|upper + '_H', feature_set.name|api) }}

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <glad/gl.h>
{% endblock %}


{% block custom_declarations %}
{% for api in feature_set.info.apis %}
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}Display *display, int screen, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}Display *display, int screen, GLADloadfunc load);

{% if options.mx_global %}
GLAD_API_CALL int gladLoad{{ api|api }}UserPtr(Display *display, int screen, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}(Display *display, int screen, GLADloadfunc load);
{% endif %}
{% endfor %}
{% endblock %}

{% block postheader %}
#endif
{% endblock %}

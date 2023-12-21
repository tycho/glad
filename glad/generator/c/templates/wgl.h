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
{% block custom_declarations %}
{% for api in feature_set.info.apis %}
GLAD_API_CALL int gladLoad{{ api|api }}UserPtr(HDC hdc, GLADuserptrloadfunc load, void *userptr);
GLAD_API_CALL int gladLoad{{ api|api }}(HDC hdc, GLADloadfunc load);
{% endfor %}
{% endblock %}
{% block postheader %}
#endif
{% endblock %}

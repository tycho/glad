{% extends 'base_template.c' %}

{% block preimpl %}
#ifdef __linux__
{% endblock %}

{% block hashsearch %}
{% endblock %}
{% block exthashes %}
{% endblock %}

{% block loader %}
#ifdef GLX_VERSION_1_1
static int glad_glx_has_extension(const char *extensions, const char *ext) {
    const char *terminator;
    const char *loc;

    if (extensions == NULL || ext == NULL)
        return -1;

    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL)
            break;

        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }

    return 0;
}
#endif

static GLADapiproc glad_glx_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
static int glad_glx_find_extensions({{ template_utils.context_arg(', ') }}Display *display, int screen) {
{% if feature_set.extensions|length > 0 %}
#ifdef GLX_VERSION_1_1
{% if not feature_set.extensions|index_consecutive_0_to_N %}
    static const uint16_t extIdx[] = {
{% for extension in feature_set.extensions %}
        {{ "{:>4}".format(extension.index) }}, /* {{ extension.name }} */
{% endfor %}
    };
{% endif %}
    const char *extensions;
    uint32_t i;

    if ({{'GLAD_glXQueryExtensionsString'|ctx}} == NULL) {
        return -2;
    }

    extensions = {{'GLAD_glXQueryExtensionsString'|ctx}}(display, screen);

    #pragma nounroll
{# If the list is a consecutive 0 to N list, we can just scan the whole thing without emitting an array. #}
{% if feature_set.extensions|index_consecutive_0_to_N %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_names); ++i)
        context->extArray[i] = glad_glx_has_extension(extensions, GLAD_{{ feature_set.name|api }}_ext_names[i]);
{% else %}
    for (i = 0; i < GLAD_ARRAYSIZE(extIdx); ++i)
        context->extArray[extIdx[i]] = glad_glx_has_extension(extensions, GLAD_{{ feature_set.name|api }}_ext_names[extIdx[i]]);
{% endif %}
#else
{% if options.mx %}
    GLAD_UNUSED(context);
{% endif %}
    GLAD_UNUSED(display);
    GLAD_UNUSED(screen);
    GLAD_UNUSED(GLAD_{{ feature_set.name|api }}_ext_names);
#endif
{% else %}
{% if options.mx %}
    GLAD_UNUSED(context);
{% endif %}
    GLAD_UNUSED(display);
    GLAD_UNUSED(screen);
    GLAD_UNUSED(GLAD_{{ feature_set.name|api }}_ext_names);
{% endif %}

    return 1;
}

static int glad_glx_find_core_{{ api|lower }}({{ template_utils.context_arg(', ') }}Display **display, int *screen) {
    int major = 0, minor = 0;
    if(*display == NULL) {
#ifdef GLAD_GLX_NO_X11
        GLAD_UNUSED(screen);
        return 0;
#else
        *display = XOpenDisplay(0);
        if (*display == NULL) {
            return 0;
        }
        *screen = XScreenNumberOfScreen(XDefaultScreenOfDisplay(*display));
#endif
    }
    {{'GLAD_glXQueryVersion'|ctx}}(*display, &major, &minor);
{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx }} = (major == {{ feature.version.major }} && minor >= {{ feature.version.minor }}) || major > {{ feature.version.major }};
{% endfor %}
    return GLAD_MAKE_VERSION(major, minor);
}

GLAD_NO_INLINE int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}Display *display, int screen, GLADuserptrloadfunc load, void *userptr) {
    int version;
    {{ 'GLAD_glXQueryVersion'|ctx }} = (PFNGLXQUERYVERSIONPROC) load(userptr, "glXQueryVersion");
    if({{'GLAD_glXQueryVersion'|ctx }} == NULL) return 0;
    version = glad_glx_find_core_{{ api|lower }}({{'context, ' if options.mx }}&display, &screen);

{% for feature, _ in loadable(feature_set.features) %}
    glad_glx_load_{{ feature.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

    if (!glad_glx_find_extensions({{'context, ' if options.mx }}display, screen)) return 0;
{% for extension, _ in loadable(feature_set.extensions) %}
    glad_glx_load_{{ extension.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

{% if options.alias %}
    glad_glx_resolve_aliases({{'context' if options.mx }});
{% endif %}

    return version;
}

{% if options.mx_global %}
int gladLoad{{ api|api }}UserPtr(Display *display, int screen, GLADuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ feature_set.name|api }}Context(), display, screen, load, userptr);
}
{% endif %}

int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}Display *display, int screen, GLADloadfunc load) {
    return gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{'context,' if options.mx }} display, screen, glad_glx_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

{% if options.mx_global %}
int gladLoad{{ api|api }}(Display *display, int screen, GLADloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ feature_set.name|api }}Context(), display, screen, GLAD_GNUC_EXTENSION (void*) load);
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

{% block postimpl %}
#endif
{% endblock %}

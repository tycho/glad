{% extends 'base_template.c' %}

{% block loader %}
static int glad_egl_get_extensions({{ template_utils.context_arg(', ') }}EGLDisplay display, char **extensions) {
    size_t clientLen, displayLen;
    char *concat;
    const char *clientExtensions = {{ 'eglQueryString'|ctx }}(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    const char *displayExtensions = (display == EGL_NO_DISPLAY) ? "" : {{ 'eglQueryString'|ctx }}(display, EGL_EXTENSIONS);

    if (!clientExtensions) return 0;
    if (!displayExtensions) return 0;

    clientLen = strlen(clientExtensions);
    displayLen = strlen(displayExtensions);

    concat = (char *)malloc(clientLen + displayLen + 2);
    if (!concat) return 0;

    concat[0] = 0;
    strcat(concat, clientExtensions);
    if (displayLen) {
        if (concat[clientLen - 1] != ' ')
            strcat(concat, " ");
        strcat(concat, displayExtensions);
    }

    *extensions = concat;

    return 1;
}

static int glad_egl_has_extension(const char *extensions, const char *ext) {
    const char *loc;
    const char *terminator;
    if(extensions == NULL) {
        return 0;
    }
    while(1) {
        loc = strstr(extensions, ext);
        if(loc == NULL) {
            return 0;
        }
        terminator = loc + strlen(ext);
        if((loc == extensions || *(loc - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0')) {
            return 1;
        }
        extensions = terminator;
    }
}

static GLADapiproc glad_egl_get_proc_from_userptr(void *userptr, const char *name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
static int glad_egl_find_extensions_{{ api|lower }}({{ template_utils.context_arg(', ') }}EGLDisplay display) {
    char *extensions;
    if (!glad_egl_get_extensions({{'context, ' if options.mx }}display, &extensions)) return 0;

{% for extension in feature_set.extensions %}
    {{ ('GLAD_' + extension.name)|ctx(name_only=True) }} = glad_egl_has_extension(extensions, "{{ extension.name }}");
{% else %}
    GLAD_UNUSED(glad_egl_has_extension);
{% endfor %}

    free(extensions);

    return 1;
}

static int glad_egl_find_core_{{ api|lower }}({{ template_utils.context_arg(', ') }}EGLDisplay display) {
    int major, minor;
    const char *version;

    if (display == NULL) {
        display = EGL_NO_DISPLAY; /* this is usually NULL, better safe than sorry */
    }

    version = {{ 'eglQueryString'|ctx }}(display, EGL_VERSION);
    (void) {{ 'eglGetError'|ctx }}();

    if (version == NULL) {
        major = 1;
        minor = 0;
    } else {
        GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);
    }

{% for feature in feature_set.features %}
    {{ ('GLAD_' + feature.name)|ctx(name_only=True) }} = (major == {{ feature.version.major }} && minor >= {{ feature.version.minor }}) || major > {{ feature.version.major }};
{% endfor %}

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}EGLDisplay display, GLADuserptrloadfunc load, void *userptr) {
    int version;
    {{ 'eglGetDisplay'|ctx }} = (PFNEGLGETDISPLAYPROC) load(userptr, "eglGetDisplay");
    {{ 'eglGetCurrentDisplay'|ctx }} = (PFNEGLGETCURRENTDISPLAYPROC) load(userptr, "eglGetCurrentDisplay");
    {{ 'eglQueryString'|ctx }} = (PFNEGLQUERYSTRINGPROC) load(userptr, "eglQueryString");
    {{ 'eglGetError'|ctx }} = (PFNEGLGETERRORPROC) load(userptr, "eglGetError");
    if ({{ 'eglGetDisplay'|ctx }} == NULL || {{ 'eglGetCurrentDisplay'|ctx }} == NULL || {{ 'eglQueryString'|ctx }} == NULL || {{ 'eglGetError'|ctx }} == NULL) return 0;

    version = glad_egl_find_core_{{ api|lower }}({{'context, ' if options.mx }}display);
    if (!version) return 0;
{% for feature, _ in loadable(feature_set.features, api=api) %}
    glad_egl_load_{{ feature.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

    if (!glad_egl_find_extensions_{{ api|lower }}({{'context, ' if options.mx }}display)) return 0;
{% for extension, _ in loadable(feature_set.extensions, api=api) %}
    glad_egl_load_{{ extension.name }}({{'context, ' if options.mx }}load, userptr);
{% endfor %}

{% if options.mx_global %}
    gladSet{{ feature_set.name|api }}Context(context);
{% endif %}

{% if options.alias %}
    glad_egl_resolve_aliases({{ 'context' if options.mx }});
{% endif %}

    return version;
}

{% if options.mx_global %}
int gladLoad{{ api|api }}UserPtr(EGLDisplay display, GLADuserptrloadfunc load, void *userptr) {
    return gladLoad{{ api|api }}ContextUserPtr(gladGet{{ feature_set.name|api }}Context(), display, load, userptr);
}
{% endif %}

int gladLoad{{ api|api }}{{ 'Context' if options.mx }}({{ template_utils.context_arg(', ') }}EGLDisplay display, GLADloadfunc load) {
    return gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{'context,' if options.mx }} display, glad_egl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}

{% if options.mx_global %}
int gladLoad{{ api|api }}(EGLDisplay display, GLADloadfunc load) {
    return gladLoad{{ api|api }}Context(gladGet{{ feature_set.name|api }}Context(), display, load);
}
{% endif %}
{% endfor %}

{% if options.mx_global %}
Glad{{ feature_set.name|api }}Context* gladGet{{ feature_set.name|api }}Context() {
    return {{ global_context }};
}

void gladSet{{ feature_set.name|api }}Context(Glad{{ feature_set.name|api }}Context *context) {
    {{ global_context }} = context;
}
{% endif %}

{% endblock %}

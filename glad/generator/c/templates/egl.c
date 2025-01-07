{% extends 'base_template.c' %}

{% block extnames %}
{% endblock %}

{% block loader %}
static int glad_egl_get_extensions({{ template_utils.context_arg(', ') }}EGLDisplay display, uint64_t **out_exts, uint32_t *out_num_exts) {
    size_t clientLen, displayLen;
    char *concat;
    uint32_t num_exts = 0, j = 0, len = 0;
    uint64_t *exts = NULL;
    const char *cur = NULL;
    const char *next = NULL;

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

    /* This is done in two passes. The first pass counts up the number of
    * extensions. The second pass copies them into an allocated block of memory. */
    for (j = 0; j < 2; ++j) {
        num_exts = 0;
        cur = concat;
        next = cur + strcspn(cur, " ");
        while (1) {
            cur += strspn(cur, " ");

            if (!cur[0])
                break;

            len = next - cur;

            if (exts != NULL) {
                exts[num_exts++] = glad_hash_string(cur, len);
            } else {
                num_exts++;
            }

            cur = next + strspn(next, " ");
            next = cur + strcspn(cur, " ");
        }

        if (!exts)
            exts = (uint64_t *)calloc(num_exts, sizeof(uint64_t));
    }

    free(concat);

{% if search_type == 0 %}
    qsort(exts, num_exts, sizeof(uint64_t), compare_uint64);

{% endif %}
    *out_exts = exts;
    *out_num_exts = num_exts;

    return 1;
}

static int glad_egl_has_extension(uint64_t *exts, uint32_t num_exts, uint64_t extension) {
    return glad_hash_search(exts, num_exts, extension);
}

static GLADapiproc glad_egl_get_proc_from_userptr(void *userptr, const char *name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

{% for api in feature_set.info.apis %}
static int glad_egl_find_extensions_{{ api|lower }}({{ template_utils.context_arg(', ') }}EGLDisplay display) {
{% if feature_set.extensions|length > 0 %}
{% if not feature_set.extensions|index_consecutive_0_to_N %}
    static const uint16_t s_extIdx[] = {
{% for extension in feature_set.extensions %}
        {{ "{:>4}".format(extension.index) }}, /* {{ extension.name }} */
{% endfor %}
    };
{% endif %}
    uint64_t *exts = NULL;
    uint32_t num_exts = 0;
    uint32_t i;

    if (!glad_egl_get_extensions({{'context, ' if options.mx }}display, &exts, &num_exts)) return 0;

    #pragma nounroll
{# If the list is a consecutive 0 to N list, we can just scan the whole thing without emitting an array. #}
{% if feature_set.extensions|index_consecutive_0_to_N %}
    for (i = 0; i < GLAD_ARRAYSIZE(GLAD_{{ feature_set.name|api }}_ext_hashes); ++i)
        context->extArray[i] = glad_egl_has_extension(exts, num_exts, GLAD_{{ feature_set.name|api }}_ext_hashes[i]);
{% else %}
    for (i = 0; i < GLAD_ARRAYSIZE(s_extIdx); ++i) {
        const uint32_t extIdx = s_extIdx[i];
        context->extArray[extIdx] = glad_egl_has_extension(exts, num_exts, GLAD_{{ feature_set.name|api }}_ext_hashes[extIdx]);
    }
{% endif %}

    free(exts);
{% endif %}
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

GLAD_NO_INLINE int gladLoad{{ api|api }}{{ 'Context' if options.mx }}UserPtr({{ template_utils.context_arg(', ') }}EGLDisplay display, GLADuserptrloadfunc load, void *userptr) {
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
    return &{{ global_context }};
}

void gladSet{{ feature_set.name|api }}Context(Glad{{ feature_set.name|api }}Context *context) {
    if (!context) return;
    if (&{{ global_context }} == context) return;
    {{ global_context }} = *context;
}
{% endif %}

{% endblock %}

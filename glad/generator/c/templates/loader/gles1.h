{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_GLES1
{#
#ifndef __egl_h_
#error "gles1 loader requires egl.h, include egl.h (<glad/egl.h>) before including the loader."
#endif
#}

GLAD_API_CALL int gladLoaderLoadGLES1Context({{ template_utils.context_arg() }});
GLAD_API_CALL int gladLoaderLoadGLES1(void);
GLAD_API_CALL void gladLoaderUnloadGLES1Context({{ template_utils.context_arg() }});

#endif /* GLAD_GLES1 */

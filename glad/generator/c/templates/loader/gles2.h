{% import "template_utils.h" as template_utils with context %}
#ifdef GLAD_GLES2
{#
#ifndef __egl_h_
#error "gles2 loader requires egl.h, include egl.h (<glad/egl.h>) before including the loader."
#endif
#}

GLAD_API_CALL int gladLoaderLoadGLES2Context({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderUnloadGLES2Context({{ template_utils.context_arg() }});
GLAD_API_CALL void gladLoaderResetGLES2Context({{ template_utils.context_arg() }});

GLAD_API_CALL int gladLoaderLoadGLES2(void);
GLAD_API_CALL void gladLoaderUnloadGLES2(void);
GLAD_API_CALL void gladLoaderResetGLES2(void);

#endif /* GLAD_GLES2 */


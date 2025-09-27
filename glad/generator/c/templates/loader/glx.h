#ifdef GLAD_GLX

GLAD_API_CALL int gladLoaderLoadGLXContext(GladGLXContext *context, Display *display, int screen);
GLAD_API_CALL void gladLoaderUnloadGLXContext(GladGLXContext *context);
GLAD_API_CALL void gladLoaderResetGLXContext(GladGLXContext *context);

GLAD_API_CALL int gladLoaderLoadGLX(Display *display, int screen);
GLAD_API_CALL void gladLoaderUnloadGLX(void);
GLAD_API_CALL void gladLoaderResetGLX(void);

#endif

// Add more functions here. PF....PROC must be defined and gl... must not be defined for the GLPROC macro to work.
#if defined(__WIN32__) || defined(__LINUX__)
GLPROC(glStencilMaskSeparate, PFNGLSTENCILMASKSEPARATEPROC, "glStencilMaskSeparate")
#endif

// Add more functions here. PF....PROC must be defined and gl... must not be defined for the GLPROC macro to work.
#if defined(_MSCVER) || defined(_MSC_VER)
GLPROC(glStencilMaskSeparate, PFNGLSTENCILMASKSEPARATEPROC, "glStencilMaskSeparate")
#endif
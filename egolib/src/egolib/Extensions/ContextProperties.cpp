#include "egolib/Extensions/ContextProperties.hpp"

namespace Ego {

MultisamplingProperties::MultisamplingProperties() :
    m_buffers(1), m_samples(4)
{}

MultisamplingProperties::MultisamplingProperties(const MultisamplingProperties& other) :
    m_buffers(other.m_buffers), m_samples(other.m_samples)
{}

MultisamplingProperties& MultisamplingProperties::operator=(const MultisamplingProperties& other)
{
    m_buffers = other.m_buffers;
    m_samples = other.m_samples;
    return *this;
}

void MultisamplingProperties::setSamples(int samples)
{
    m_samples = samples;
}

int MultisamplingProperties::getSamples() const
{
    return m_samples;
}

void MultisamplingProperties::setBuffers(int buffers)
{
    m_buffers = buffers;
}

int MultisamplingProperties::getBuffers() const
{
    return m_buffers;
}

bool MultisamplingProperties::equal_to(const MultisamplingProperties& other) const
{
    return m_buffers == other.m_buffers
        && m_samples == other.m_samples;
}

void MultisamplingProperties::upload() const
{
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, m_buffers);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, m_samples);
}

void MultisamplingProperties::download()
{
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &m_buffers);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &m_samples);
}

Log::Entry& operator<<(Log::Entry& e, const MultisamplingProperties& s)
{
    e << "  " << "multisampling" << Log::EndOfLine;
    e << "  " << "  " << "multisampling buffers = " << s.m_buffers << Log::EndOfLine;
    e << "  " << "  " << "multisampling samples = " << s.m_samples << Log::EndOfLine;
    return e;
}

ContextProperties::ContextProperties() :
    colourBufferDepth(32, 8, 8, 8, 8),
    accumulationBufferDepth(0, 0, 0, 0, 0),
    doublebuffer(0),
    stencilBufferDepth(0),
    stereo(0),
    swap_control(0),
    multisampling(),
    accelerated_visual(1),
    buffer_size(32),
    depthBufferDepth(8)
{}

ContextProperties::ContextProperties(const ContextProperties& other) :
    colourBufferDepth(other.colourBufferDepth),
    accumulationBufferDepth(other.accumulationBufferDepth),
    doublebuffer(other.doublebuffer),
    stencilBufferDepth(other.stencilBufferDepth),
    stereo(other.stereo),
    swap_control(other.swap_control),
    multisampling(other.multisampling),
    accelerated_visual(other.accelerated_visual),
    buffer_size(other.buffer_size),
    depthBufferDepth(other.depthBufferDepth)
{}

ContextProperties& ContextProperties::operator=(const ContextProperties& other)
{
    colourBufferDepth = other.colourBufferDepth;
    accumulationBufferDepth = other.accumulationBufferDepth;
    doublebuffer = other.doublebuffer;
    stencilBufferDepth = other.stencilBufferDepth;
    stereo = other.stereo;
    swap_control = other.swap_control;
    multisampling = other.multisampling;
    accelerated_visual = other.accelerated_visual;
    buffer_size = other.buffer_size;
    depthBufferDepth = other.depthBufferDepth;
    return *this;
}

bool ContextProperties::equal_to(const ContextProperties& other) const
{
    return colourBufferDepth == other.colourBufferDepth
        && accumulationBufferDepth == other.accumulationBufferDepth
        && doublebuffer == other.doublebuffer
        && stencilBufferDepth == other.stencilBufferDepth
        && stereo == other.stereo
        && swap_control == other.swap_control
        && multisampling == other.multisampling
        && accelerated_visual == other.accelerated_visual
        && buffer_size == other.buffer_size
        && depthBufferDepth == other.depthBufferDepth;
}

void ContextProperties::validate(ContextProperties& self)
{
    int frameBufferSize = self.buffer_size;
    int colorBufferRedDepth = self.colourBufferDepth.getRedDepth(),
        colorBufferGreenDepth = self.colourBufferDepth.getGreenDepth(),
        colorBufferBlueDepth = self.colourBufferDepth.getBlueDepth();
    if (0 == frameBufferSize) frameBufferSize = self.colourBufferDepth.getDepth();
    if (0 == frameBufferSize) frameBufferSize = 32;
    if (frameBufferSize > 32) frameBufferSize = 32;

    // Fix bad colour depths.
    if ((0 == colorBufferRedDepth &&
         0 == colorBufferGreenDepth &&
         0 == colorBufferBlueDepth) ||
         (colorBufferRedDepth + colorBufferGreenDepth + colorBufferBlueDepth > frameBufferSize))
    {
        if (frameBufferSize > 24)
        {
            colorBufferRedDepth = colorBufferGreenDepth = colorBufferBlueDepth = frameBufferSize / 4;
        }
        else
        {
            // Do a kludge in case we have something silly like 16 bit "highcolor" mode.
            colorBufferRedDepth = colorBufferBlueDepth = frameBufferSize / 3;
            colorBufferGreenDepth = frameBufferSize - colorBufferRedDepth - colorBufferBlueDepth;
        }

        colorBufferRedDepth = (colorBufferRedDepth > 8) ? 8 : colorBufferRedDepth;
        colorBufferGreenDepth = (colorBufferGreenDepth > 8) ? 8 : colorBufferGreenDepth;
        colorBufferBlueDepth = (colorBufferBlueDepth > 8) ? 8 : colorBufferBlueDepth;
    }

    // Fix the alpha alpha depth.
    int colorBufferAlphaDepth;// = self.colourBufferDepth.getAlphaDepth();
    colorBufferAlphaDepth = frameBufferSize - colorBufferRedDepth - colorBufferGreenDepth - colorBufferBlueDepth;
    colorBufferAlphaDepth = (colorBufferAlphaDepth < 0) ? 0 : colorBufferAlphaDepth;

    // Fix the frame buffer depth.
    frameBufferSize = colorBufferRedDepth + colorBufferGreenDepth + colorBufferBlueDepth + colorBufferAlphaDepth;
    frameBufferSize &= ~7;

    // Recompute the frame buffer depth and colour buffer depth.
    self.buffer_size = frameBufferSize;
    self.colourBufferDepth = Ego::ColourDepth(colorBufferRedDepth + colorBufferGreenDepth + colorBufferBlueDepth + colorBufferAlphaDepth,
                                              colorBufferRedDepth, colorBufferGreenDepth, colorBufferBlueDepth, colorBufferAlphaDepth);

}

void ContextProperties::upload() const
{
    // (1) Set the colour buffer depth.
    const int colourBufferDepth_sdl[4]
        = {colourBufferDepth.getRedDepth(),
        colourBufferDepth.getGreenDepth(),
        colourBufferDepth.getBlueDepth(),
        colourBufferDepth.getAlphaDepth()};
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colourBufferDepth_sdl[0]);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colourBufferDepth_sdl[1]);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, colourBufferDepth_sdl[2]);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, colourBufferDepth_sdl[3]);

    // (2) Set the frame buffer depth.
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, buffer_size);

    // (3) Enable/disable double buffering.
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, doublebuffer);

    // (4) Set the depth buffer and stencil buffer depths.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBufferDepth);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBufferDepth);

    // (5) Set the accumulation buffer depth.
    const int accumulationBufferDepth_sdl[4]{
        accumulationBufferDepth.getRedDepth(),
        accumulationBufferDepth.getGreenDepth(),
        accumulationBufferDepth.getBlueDepth(),
        accumulationBufferDepth.getAlphaDepth()
    };
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, accumulationBufferDepth_sdl[0]);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, accumulationBufferDepth_sdl[1]);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, accumulationBufferDepth_sdl[2]);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, accumulationBufferDepth_sdl[3]);

    // (6) Enable/disable stereoscopic rendering.
    SDL_GL_SetAttribute(SDL_GL_STEREO, stereo);

    // (7) Set multisampling.
    multisampling.upload();

    // (8) Enable/disable hardware acceleration.
#if !defined(ID_LINUX)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, accelerated_visual);
#endif
}

void ContextProperties::download()
{
    int temporary[4];

    // (1) Get the colour buffer depth.
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &(temporary[0]));
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &(temporary[1]));
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &(temporary[2]));
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &(temporary[3]));
    colourBufferDepth = Ego::ColourDepth(temporary[0] + temporary[1] + temporary[2] + temporary[3],
                                         temporary[0], temporary[1], temporary[2], temporary[3]);

    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &(buffer_size));
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &(doublebuffer));
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &(depthBufferDepth));
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &(stencilBufferDepth));

    // (2) Get the accumulation buffer depth.
    SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE, &(temporary[0]));
    SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &(temporary[1]));
    SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE, &(temporary[1]));
    SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &(temporary[2]));
    accumulationBufferDepth = Ego::ColourDepth(temporary[0] + temporary[1] + temporary[2] + temporary[3],
                                               temporary[0], temporary[1], temporary[2], temporary[3]);

    SDL_GL_GetAttribute(SDL_GL_STEREO, &(stereo));

    swap_control = SDL_GL_GetSwapInterval();

    multisampling.download();

#if !defined(ID_LINUX)
    SDL_GL_GetAttribute(SDL_GL_ACCELERATED_VISUAL, &(accelerated_visual));
#endif
}

Log::Entry& operator<<(Log::Entry& e, const ContextProperties& s)
{
    e << "context attributes" << Log::EndOfLine;

    // Framebuffer depth.
    e << "  " << "framebuffer depth = " << s.buffer_size << Log::EndOfLine;

    // Colour buffer depth.
    e << "  " << "colour buffer" << Log::EndOfLine
        << "  " << "  " << "depth = " << s.colourBufferDepth.getDepth() << Log::EndOfLine
        << "  " << "  " << "red depth = " << s.colourBufferDepth.getRedDepth() << Log::EndOfLine
        << "  " << "  " << "green depth = " << s.colourBufferDepth.getGreenDepth() << Log::EndOfLine
        << "  " << "  " << "blue depth = " << s.colourBufferDepth.getBlueDepth() << Log::EndOfLine
        << "  " << "  " << "alpha depth = " << s.colourBufferDepth.getAlphaDepth() << Log::EndOfLine;

    // Depth buffer depth.
    e << "  " << "depth buffer depth = " << s.depthBufferDepth << Log::EndOfLine;

    // Stencil buffer depth.
    e << "  " << "stencil buffer depth = " << s.stencilBufferDepth << Log::EndOfLine;

    // Accumulation buffer depth.
    e << "  " << "accumulation buffer" << Log::EndOfLine
        << "  " << "  " << "depth = " << s.accumulationBufferDepth.getDepth() << Log::EndOfLine
        << "  " << "  " << "red depth = " << s.accumulationBufferDepth.getRedDepth() << Log::EndOfLine
        << "  " << "  " << "green depth = " << s.accumulationBufferDepth.getGreenDepth() << Log::EndOfLine
        << "  " << "  " << "blue depth = " << s.accumulationBufferDepth.getBlueDepth() << Log::EndOfLine
        << "  " << "  " << "alpha depth = " << s.accumulationBufferDepth.getAlphaDepth() << Log::EndOfLine;

    // Double buffering, stereoscopic rendering.
    e << "  " << "double buffer = " << s.doublebuffer << Log::EndOfLine;
    e << "  " << "stereoscopic rendering = " << s.stereo << Log::EndOfLine;

    // Multisampling.
    e << s.multisampling;

#if !defined(ID_LINUX)
    e << "  " << "accelerated visual = " << s.accelerated_visual << Log::EndOfLine;
#endif
    e << "  " << "swap control = " << s.swap_control << Log::EndOfLine;
    return e;
}

} // namespace Ego

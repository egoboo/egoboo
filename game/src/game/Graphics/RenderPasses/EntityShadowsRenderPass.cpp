#include "game/Graphics/RenderPasses/EntityShadowsRenderPass.hpp"
#include "game/Module/Module.hpp"
#include "game/graphic.h"
#include "egolib/Entities/_Include.hpp"

namespace Ego {
namespace Graphics {

EntityShadowsRenderPass::EntityShadowsRenderPass() :
    RenderPass("entity shadows"),
    _vertexDescriptor(VertexFormatFactory::get<VertexFormat::P3FT2F>()),
    _vertexBuffer(4, _vertexDescriptor.getVertexSize())
{}

void EntityShadowsRenderPass::doRun(::Camera& camera, const TileList& tl, const EntityList& el)
{
    // If shadows are not enabled, return immediatly.
    if (!gfx.shadows_enable)
    {
        return;
    }
    // Get the renderer.
    auto& renderer = Renderer::get();
    // Do not write into the depth buffer.
    renderer.setDepthWriteEnabled(false);
    // Enable depth tests.
    renderer.setDepthTestEnabled(true);
    // Enable blending.
    renderer.setBlendingEnabled(true);
    renderer.setBlendFunction(id::blend_function::zero, id::blend_function::one_minus_source_color);

    // Keep track of the number of rendered shadows.
    size_t count = 0;

    if (gfx.shadows_highQuality_enable)
    {
        // Render high-quality shadows.
        for (size_t i = 0; i < el.getSize(); ++i)
        {
            ObjectRef ichr = el.get(i).iobj;
            if (ObjectRef::Invalid == ichr) continue;
            if (0 == _currentModule->getObjectHandler().get(ichr)->shadow_size) continue;
            doHighQualityShadow(ichr);
            count++;
        }
    }
    else
    {
        // Render low-quality shadows.
        for (size_t i = 0; i < el.getSize(); ++i)
        {
            ObjectRef ichr = el.get(i).iobj;
            if (ObjectRef::Invalid == ichr) continue;
            if (0 == _currentModule->getObjectHandler().get(ichr)->shadow_size) continue;
            doLowQualityShadow(ichr);
            count++;
        }
    }
}

void EntityShadowsRenderPass::doLowQualityShadow(const ObjectRef character)
{
    Object *pchr = _currentModule->getObjectHandler().get(character);
    if (pchr->isBeingHeld()) return;

    // If the object is hidden it is not drawn at all, so it has no shadow.
    // If the object's shadow size is qa 0, then it has no shadow.
    if (pchr->isHidden() || 0 == pchr->shadow_size)
    {
        return;
    }

    // No shadow if completely transparent or completely glowing.
    float alpha = (255 == pchr->inst.light) ? pchr->inst.alpha  * id::fraction<float, 1, 255>() : (pchr->inst.alpha - pchr->inst.light) * id::fraction<float, 1, 255>();

    /// @test ZF@> previous test didn't work, but this one does
    //if ( alpha * 255 < 1 ) return;
    if (pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE) return;

    // much reduced shadow if on a reflective tile
    auto mesh = _currentModule->getMeshPointer();
    if (0 != mesh->test_fx(pchr->getTile(), MAPFX_REFLECTIVE))
    {
        alpha *= 0.1f;
    }
    if (alpha < id::fraction<float, 1, 255>()) return;

    // Original points
    float level = pchr->getObjectPhysics().getGroundElevation() + SHADOWRAISE;
    float height = pchr->inst.getMatrix()(2, 3) - level;
    float height_factor = 1.0f - height / (pchr->shadow_size * 5.0f);
    if (height_factor <= 0.0f) return;

    // how much transparency from height
    alpha *= height_factor * 0.5f + 0.25f;
    if (alpha < id::fraction<float, 1, 255>()) return;

    float x = pchr->inst.getMatrix()(0, 3); ///< @todo MH: This should be the x/y position of the model.
    float y = pchr->inst.getMatrix()(1, 3); ///<           Use a more self-descriptive method to describe this.

    std::shared_ptr<const Texture> texture = ParticleHandler::get().getLightParticleTexture();

    float size = pchr->shadow_size * height_factor;

    {
        BufferScopedLock lock(_vertexBuffer);
        Vertex *vertices = lock.get<Vertex>();
        vertices[0].x = (float)x + size;
        vertices[0].y = (float)y - size;
        vertices[0].z = (float)level;

        vertices[1].x = (float)x + size;
        vertices[1].y = (float)y + size;
        vertices[1].z = (float)level;

        vertices[2].x = (float)x - size;
        vertices[2].y = (float)y + size;
        vertices[2].z = (float)level;

        vertices[3].x = (float)x - size;
        vertices[3].y = (float)y - size;
        vertices[3].z = (float)level;
    }

    // Choose texture and matrix
    Renderer::get().getTextureUnit().setActivated(texture.get());
    {
        BufferScopedLock lock(_vertexBuffer);
        Vertex *vertices = lock.get<Vertex>();
        vertices[0].s = ParticleGraphicsRenderer::CALCULATE_PRT_U0(*texture, 236);
        vertices[0].t = ParticleGraphicsRenderer::CALCULATE_PRT_V0(*texture, 236);

        vertices[1].s = ParticleGraphicsRenderer::CALCULATE_PRT_U1(*texture, 253);
        vertices[1].t = ParticleGraphicsRenderer::CALCULATE_PRT_V0(*texture, 236);

        vertices[2].s = ParticleGraphicsRenderer::CALCULATE_PRT_U1(*texture, 253);
        vertices[2].t = ParticleGraphicsRenderer::CALCULATE_PRT_V1(*texture, 253);

        vertices[3].s = ParticleGraphicsRenderer::CALCULATE_PRT_U0(*texture, 236);
        vertices[3].t = ParticleGraphicsRenderer::CALCULATE_PRT_V1(*texture, 253);
    }

    doShadowSprite(alpha, _vertexBuffer, _vertexDescriptor);
}

void EntityShadowsRenderPass::doHighQualityShadow(const ObjectRef character)
{

    Object *pchr = _currentModule->getObjectHandler().get(character);
    if (pchr->isBeingHeld()) return;

    // if the character is hidden, not drawn at all, so no shadow
    if (pchr->isHidden() || 0 == pchr->shadow_size) return;

    // no shadow if completely transparent
    float alpha = (255 == pchr->inst.light) ? pchr->inst.alpha  * id::fraction<float, 1, 255>() : (pchr->inst.alpha - pchr->inst.light) * id::fraction<float, 1, 255>();

    /// @test ZF@> The previous test didn't work, but this one does
    //if ( alpha * 255 < 1 ) return;
    if (pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE) return;

    // much reduced shadow if on a reflective tile
    auto mesh = _currentModule->getMeshPointer();
    if (0 != mesh->test_fx(pchr->getTile(), MAPFX_REFLECTIVE))
    {
        alpha *= 0.1f;
    }
    if (alpha < id::fraction<float, 1, 255>()) return;

    // Original points
    float level = pchr->getObjectPhysics().getGroundElevation() + SHADOWRAISE;
    float height = pchr->inst.getMatrix()(2, 3) - level;
    if (height < 0) height = 0;

    float size_umbra = 1.5f * (pchr->bump.size - height / 30.0f);
    float size_penumbra = 1.5f * (pchr->bump.size + height / 30.0f);

    alpha *= 0.3f;

    std::shared_ptr<const  Texture> texture = ParticleHandler::get().getLightParticleTexture();

    float   alpha_umbra, alpha_penumbra;
    alpha_umbra = alpha_penumbra = alpha;
    if (height > 0)
    {
        float factor_penumbra = (1.5f) * ((pchr->bump.size) / size_penumbra);
        float factor_umbra = (1.5f) * ((pchr->bump.size) / size_umbra);

        factor_umbra = std::max(1.0f, factor_umbra);
        factor_penumbra = std::max(1.0f, factor_penumbra);

        alpha_umbra *= 1.0f / factor_umbra / factor_umbra / 1.5f;
        alpha_penumbra *= 1.0f / factor_penumbra / factor_penumbra / 1.5f;

        alpha_umbra = Math::constrain(alpha_umbra, 0.0f, 1.0f);
        alpha_penumbra = Math::constrain(alpha_penumbra, 0.0f, 1.0f);
    }

    float x = pchr->inst.getMatrix()(0, 3);
    float y = pchr->inst.getMatrix()(1, 3);

    // Choose texture and matrix
    Renderer::get().getTextureUnit().setActivated(texture.get());

    // GOOD SHADOW
    {
        BufferScopedLock lock(_vertexBuffer);
        Vertex *vertices = lock.get<Vertex>();

        vertices[0].s = ParticleGraphicsRenderer::CALCULATE_PRT_U0(*texture, 238);
        vertices[0].t = ParticleGraphicsRenderer::CALCULATE_PRT_V0(*texture, 238);

        vertices[1].s = ParticleGraphicsRenderer::CALCULATE_PRT_U1(*texture, 255);
        vertices[1].t = ParticleGraphicsRenderer::CALCULATE_PRT_V0(*texture, 238);

        vertices[2].s = ParticleGraphicsRenderer::CALCULATE_PRT_U1(*texture, 255);
        vertices[2].t = ParticleGraphicsRenderer::CALCULATE_PRT_V1(*texture, 255);

        vertices[3].s = ParticleGraphicsRenderer::CALCULATE_PRT_U0(*texture, 238);
        vertices[3].t = ParticleGraphicsRenderer::CALCULATE_PRT_V1(*texture, 255);
    }
    if (size_penumbra > 0)
    {
        {
            BufferScopedLock lock(_vertexBuffer);
            Vertex *vertices = lock.get<Vertex>();

            vertices[0].x = x + size_penumbra;
            vertices[0].y = y - size_penumbra;
            vertices[0].z = level;

            vertices[1].x = x + size_penumbra;
            vertices[1].y = y + size_penumbra;
            vertices[1].z = level;

            vertices[2].x = x - size_penumbra;
            vertices[2].y = y + size_penumbra;
            vertices[2].z = level;

            vertices[3].x = x - size_penumbra;
            vertices[3].y = y - size_penumbra;
            vertices[3].z = level;
        }
        doShadowSprite(alpha_penumbra, _vertexBuffer, _vertexDescriptor);
    };

    if (size_umbra > 0)
    {
        {
            BufferScopedLock lock(_vertexBuffer);
            Vertex *vertices = lock.get<Vertex>();

            vertices[0].x = x + size_umbra;
            vertices[0].y = y - size_umbra;
            vertices[0].z = level + 0.1f;

            vertices[1].x = x + size_umbra;
            vertices[1].y = y + size_umbra;
            vertices[1].z = level + 0.1f;

            vertices[2].x = x - size_umbra;
            vertices[2].y = y + size_umbra;
            vertices[2].z = level + 0.1f;

            vertices[3].x = x - size_umbra;
            vertices[3].y = y - size_umbra;
            vertices[3].z = level + 0.1f;
        }

        doShadowSprite(alpha_umbra, _vertexBuffer, _vertexDescriptor);
    }
}

void EntityShadowsRenderPass::doShadowSprite(float intensity, VertexBuffer& vertexBuffer, VertexDescriptor& vertexDescriptor)
{
    if (intensity*255.0f < 1.0f) return;

    //Limit the intensity to a valid range
    intensity = Math::constrain(intensity, 0.0f, 1.0f);

    auto& renderer = Renderer::get();
    renderer.setColour(Math::Colour4f(intensity, intensity, intensity, 1.0f));

    renderer.render(vertexBuffer, vertexDescriptor, id::primitive_type::triangle_fan, 0, 4);
}

} // namespace Graphics	
} // namespace Ego

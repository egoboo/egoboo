#include "egolib/Math/LERP.hpp"
#include "egolib/vec.h"
#include "egolib/math/Colour3f.hpp"
#include "egolib/math/Colour4f.hpp"

namespace Ego
{
    namespace Math
    {

        template <>
        fvec2_t lerp<fvec2_t>(const fvec2_t& x, const fvec2_t& y, float t)
        {
            if (t < 0)
            {
                throw std::domain_error("t < 0");
            }
            else if (t > 1)
            {
                throw std::domain_error("t > 1");
            }
            return fvec2_t(lerp(x[kX], y[kX], t), lerp(x[kY], y[kY], t));

        }

        template <>
        fvec3_t lerp<fvec3_t>(const fvec3_t& x, const fvec3_t& y, float t)
        {
            if (t < 0)
            {
                throw std::domain_error("t < 0");
            }
            else if (t > 1)
            {
                throw std::domain_error("t > 1");
            }
            return fvec3_t(lerp(x[kX], y[kX], t), lerp(x[kY], y[kY], t), lerp(x[kZ], y[kZ], t));
        }

        template <>
        Colour3f lerp<Colour3f>(const Colour3f& x, const Colour3f& y, float t)
        {
            if (t < 0)
            {
                throw std::domain_error("t < 0");
            }
            else if (t > 1)
            {
                throw std::domain_error("t > 1");
            }
            return Colour3f(lerp(x.getRed(),y.getRed(),t),
                            lerp(x.getGreen(),y.getGreen(),t),
                            lerp(x.getBlue(),y.getBlue(),t));
        }

        template <>
        Colour4f lerp<Colour4f>(const Colour4f& x, const Colour4f& y, float t)
        {
            if (t < 0)
            {
                throw std::domain_error("t < 0");
            }
            else if (t > 1)
            {
                throw std::domain_error("t > 1");
            }
            return Colour4f(lerp(x.getRed(), y.getRed(), t),
                            lerp(x.getGreen(), y.getGreen(), t),
                            lerp(x.getBlue(), y.getBlue(), t),
                            lerp(x.getAlpha(),y.getAlpha(),t));
        }

    }
}
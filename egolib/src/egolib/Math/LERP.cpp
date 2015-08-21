#include "egolib/Math/LERP.hpp"
#include "egolib/Math/Vector.hpp"
#include "egolib/Math/Colour3f.hpp"
#include "egolib/Math/Colour4f.hpp"

namespace Ego
{
    namespace Math
    {

        template <>
		Vector2f lerp<Vector2f>(const Vector2f& x, const Vector2f& y, float t)
        {
            if (t < 0)
            {
                throw std::domain_error("t < 0");
            }
            else if (t > 1)
            {
                throw std::domain_error("t > 1");
            }
            return Vector2f(lerp(x[kX], y[kX], t), lerp(x[kY], y[kY], t));

        }

        template <>
		Vector3f lerp<Vector3f>(const Vector3f& x, const Vector3f& y, float t)
        {
            if (t < 0)
            {
                throw std::domain_error("t < 0");
            }
            else if (t > 1)
            {
                throw std::domain_error("t > 1");
            }
            return Vector3f(lerp(x[kX], y[kX], t), lerp(x[kY], y[kY], t), lerp(x[kZ], y[kZ], t));
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
//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file  egolib/bbox.h
/// @brief A small "library" for dealing with various bounding boxes.

#pragma once

#include "egolib/vec.h"
#include "egolib/platform.h"

//--------------------------------------------------------------------------------------------

/// Level 0 character "bumper"
/// The simplest collision volume, equivalent to the old-style collision data
/// stored in data.txt
    struct bumper_t
    {
        float  size;        ///< Size of bumpers
        float  size_big;    ///< For octagonal bumpers
        float  height;      ///< Distance from head to toe

        bumper_t() :
            size(0.0f),
            size_big(0.0f),
            height(0.0f)
        {
            //ctor
        }
    };

//--------------------------------------------------------------------------------------------

/// The various axes for the octagonal bounding box
    enum e_octagonal_axes
    {
        OCT_X, OCT_Y, OCT_XY, OCT_YX, OCT_Z, OCT_COUNT
    };

/// a "vector" that measures distances based on the axes of an octagonal bounding box
    typedef float * oct_vec_base_t;
    typedef float oct_vec_t[OCT_COUNT];

    struct oct_vec_v2_t
    {

    public:

        float _v[OCT_COUNT];

    public:

        void add(const fvec3_t& other)
        {
            sub(oct_vec_v2_t(other));
        }

        void add(const oct_vec_v2_t& other)
        {
            for (size_t i = 0; i < OCT_COUNT; ++i)
            {
                _v[i] += other[i];
            }
        }

        void sub(const fvec3_t& other)
        {
            sub(oct_vec_v2_t(other));
        }

        void sub(const oct_vec_v2_t& other)
        {
            for (size_t i = 0; i < OCT_COUNT; ++i)
            {
                _v[i] -= other[i];
            }
        }

        void mul(const float scalar)
        {
            for (size_t i = 0; i < OCT_COUNT; ++i)
            {
                _v[i] *= scalar;
            }
        }

        void assign(const oct_vec_v2_t& other)
        {
            for (size_t i = 0; i < OCT_COUNT; ++i)
            {
                _v[i] = other._v[i];
            }
        }

    public:

        oct_vec_v2_t()
        {
            _v[OCT_X] = 0.0f;
            _v[OCT_Y] = 0.0f;
            _v[OCT_Z] = 0.0f;
            _v[OCT_XY] = 0.0f;
            _v[OCT_YX] = 0.0f;
        }

        oct_vec_v2_t(const fvec3_t& point)
        {
            _v[OCT_X] = point[kX];
            _v[OCT_Y] = point[kY];
            _v[OCT_Z] = point[kZ];
            // x + y
            _v[OCT_XY] = point[kX] + point[kY];
            // y - x
            _v[OCT_YX] = point[kY] - point[kX];
        }

        oct_vec_v2_t(float x, float y, float z, float xy, float yx)
        {
            _v[OCT_X] = x;
            _v[OCT_Y] = y;
            _v[OCT_Z] = z;
            _v[OCT_XY] = xy;
            _v[OCT_YX] = yx;
        }

        oct_vec_v2_t(const oct_vec_v2_t& other)
        {
            for (size_t i = 0; i < OCT_COUNT; ++i)
            {
                _v[i] = other._v[i];
            }
        }

    public:

        oct_vec_v2_t operator+(const oct_vec_v2_t& other) const
        {
            return
                oct_vec_v2_t
                (
                _v[OCT_X]  + other._v[OCT_X],
                _v[OCT_Y]  + other._v[OCT_Y],
                _v[OCT_Z]  + other._v[OCT_Z],
                _v[OCT_XY] + other._v[OCT_XY],
                _v[OCT_YX] + other._v[OCT_YX]
                );
        }

        oct_vec_v2_t& operator+=(const oct_vec_v2_t& other)
        {
            add(other);
            return *this;
        }

        oct_vec_v2_t operator-(const oct_vec_v2_t& other) const
        {
            return
                oct_vec_v2_t
                (
                _v[OCT_X]  - other._v[OCT_X],
                _v[OCT_Y]  - other._v[OCT_Y],
                _v[OCT_Z]  - other._v[OCT_Z],
                _v[OCT_XY] - other._v[OCT_XY],
                _v[OCT_YX] - other._v[OCT_YX]
                );
        }

        oct_vec_v2_t& operator-=(const oct_vec_v2_t& other)
        {
            sub(other);
            return *this;
        }

        oct_vec_v2_t& operator*=(const float scalar)
        {
            mul(scalar);
            return *this;
        }

        oct_vec_v2_t operator*(const float scalar) const
        {
            return
                oct_vec_v2_t
                (
                _v[OCT_X]  * scalar,
                _v[OCT_Y]  * scalar,
                _v[OCT_Z]  * scalar,
                // x * scalar + y * scalar = (x + y) * scalar
                _v[OCT_XY] * scalar,
                // y * scalar - x * scalar = (y - x) * scalar
                _v[OCT_YX] * scalar
                );
        }
        oct_vec_v2_t& operator=(const oct_vec_v2_t& other)
        {
            assign(other);
            return *this;
        }
        void setZero()
        {
            for (size_t i = 0; i < OCT_COUNT; ++i)
            {
                _v[i] = 0.0f;
            }
        }
        void ctor(const fvec3_t& point)
        {
            _v[OCT_X]  = point[kX];
            _v[OCT_Y]  = point[kY];
            _v[OCT_Z]  = point[kZ];
            // x + y
            _v[OCT_XY] = point[kX] + point[kY];
            // y - x
            _v[OCT_YX] = point[kY] - point[kX];
        }
        const float& operator[] (const size_t index) const
        {
            if (index >= OCT_COUNT)
            {
                throw std::out_of_range("index out of range");
            }
            return _v[index];
        }
        float& operator[](const size_t index)
        {
            if (index >= OCT_COUNT)
            {
                throw std::out_of_range("index out of range");
            }
            return _v[index];
        }
        
    };

    bool oct_vec_add_fvec3(const oct_vec_v2_t& osrc, const fvec3_t& fvec, oct_vec_v2_t& odst);

//--------------------------------------------------------------------------------------------

/// generic octagonal bounding box
/// to be used for the Level 1 character "bumper"
/// The best possible octagonal bounding volume. A generalization of the old octagonal bounding box
/// values in data.txt. Computed on the fly.
    struct oct_bb_t
    {
    public:
        bool empty;
    public:
        oct_vec_v2_t mins, maxs;

        bool isEmpty() const
        {
            return empty;
        }

        oct_bb_t() :
            mins(),maxs(),empty(true)
        {
        }

        oct_bb_t(const oct_bb_t& other)
            : mins(other.mins), maxs(other.maxs), empty(other.empty)
        {
        }

        void assign(const oct_bb_t& other)
        {
            mins = other.mins;
            maxs = other.maxs;
            empty = other.empty;
        }

        oct_bb_t& operator=(const oct_bb_t& other)
        {
            assign(other);
            return *this;
        }

        const oct_vec_v2_t& getMin() const
        {
            if (empty)
            {
                throw std::invalid_argument("an empty obb does not have a min-point");
            }
            return mins;
        }

        const oct_vec_v2_t& getMax() const
        {
            if (empty)
            {
                throw std::invalid_argument("an empty obb does not have a max-point");
            }
            return maxs;
        }

        oct_vec_v2_t getMid() const
        {
            if (empty)
            {
                throw std::invalid_argument("an empty obb does not have a mid-point");
            }
            return (mins + maxs) * 0.5f;
        }

        void assign(const bumper_t& other)
        {
            mins[OCT_X] = -other.size;
            maxs[OCT_X] = +other.size;

            mins[OCT_Y] = -other.size;
            maxs[OCT_Y] = +other.size;

            mins[OCT_XY] = -other.size_big;
            maxs[OCT_XY] = +other.size_big;

            mins[OCT_YX] = -other.size_big;
            maxs[OCT_YX] = +other.size_big;

            mins[OCT_Z] = -other.height;
            maxs[OCT_Z] = +other.height;

            oct_bb_t::validate(this);
        }

        /**
         * @brief
         *	Translate this bounding box.
         * @param t
         *	the translation vector
         */
        void translate(const fvec3_t& t)
        {
            translate(oct_vec_v2_t(t));
        }
        void translate(const oct_vec_v2_t& t)
        {
            mins.add(t);
            maxs.add(t);
        }

        /**
         * @brief
         *  Get if this bounding volume contains a point.
         * @param self
         *  this bounding volume
         * @param other
         *  the point
         * @return
         *  @a true if this bounding volume contains the point, @a false otherwise
         */
        static bool contains(const oct_bb_t *self, const oct_vec_v2_t& other);
        
        /**
         * @brief
         *  Get if this bounding volume contains another bounding volume.
         * @param self
         *  this bounding volume
         * @param other
         *  the other bounding volume
         * @return
         *  @a true if this bounding volume contains the other bounding volume, @a false otherwise
         */
        static bool contains(const oct_bb_t *self, const oct_bb_t *other);
        
		static oct_bb_t *ctor(oct_bb_t *self);
		static void dtor(oct_bb_t *self);
        static egolib_rv validate(oct_bb_t *self);
        static bool empty_raw(const oct_bb_t *self);
    };

    /**
     * @brief
     *  Assign this octagonal bounding box the join of itself with a octagonal vector.
     * @param self
     *  this octagonal bounding box
     * @param other
     *  the octagonal vector
     * @post
     *  This octagonal bounding box was assigned the join of itself with the octagonal vector.
     */
    egolib_rv oct_bb_self_join(oct_bb_t& self, const oct_vec_v2_t& other);
    egolib_rv oct_bb_self_join(oct_bb_t& self, const oct_bb_t& other);
    egolib_rv oct_bb_self_cut(oct_bb_t& self, const oct_bb_t& other);
    
    egolib_rv oct_bb_self_grow(oct_bb_t *self, const oct_vec_v2_t& v);
    /**
     * @brief
     *  Assign this octagonal bounding box the restricted join of itself with another octagonal bounding box.
     * @param self
     *  this octagonal bounding box
     * @param other
     *  the other octagonal bounding box
     * @param index
     *  the axis the join is restricted to
     * @post
     *  This octagonal bounding box was asisigned the restricted intersection of itself with the other octagonal bounding box.
     */
    egolib_rv oct_bb_self_union_index(oct_bb_t *self, const oct_bb_t *other, int index);
    /**
     * @brief
     *  Assign this octagonal bounding box the restricted intersection of itself with another octagonal bounding box.
     * @param self
     *  this octagonal bounding box
     * @param other
     *  the other octagonal bounding box
     * @param index
     *  the axis the join is restricted to
     * @post
     *  This octagonal bounding box was assigned the restricted intersection of itself with the other octagonal bounding box.
     */
    egolib_rv oct_bb_self_intersection_index(oct_bb_t *self, const oct_bb_t *other, int index);



    /**
     * @brief
     *	Translate this bounding box.
     * @param src
     *	the source bounding box
     * @param t
     *	the translation vector
     * @param dst
     *	the target bounding box
     */
    egolib_rv oct_bb_translate(const oct_bb_t *src, const fvec3_t& t, oct_bb_t *dst);
    egolib_rv oct_bb_translate(const oct_bb_t *src, const oct_vec_v2_t& t, oct_bb_t *dst);

    egolib_rv  oct_bb_interpolate( const oct_bb_t * psrc1, const oct_bb_t * psrc2, oct_bb_t * pdst, float flip );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @details A convex poly representation of an object volume
    struct OVolume_t
    {
        OVolume_t() :
            lod(0),
            needs_shape(false),
            needs_position(false),
            oct()
        {
            //ctor
        }

        int        lod;             ///< the level of detail (LOD) of this volume
        bool   needs_shape;     ///< is the shape data valid?
        bool   needs_position;  ///< Is the position data valid?

        oct_bb_t   oct;
    };

    OVolume_t * OVolume__ctor( OVolume_t * );
    OVolume_t OVolume_merge( const OVolume_t * pv1, const OVolume_t * pv2 );
    OVolume_t OVolume_intersect( const OVolume_t * pv1, const OVolume_t * pv2 );
//bool    OVolume_draw( OVolume_t * cv, bool draw_square, bool draw_diamond );
//bool    OVolume_shift( OVolume_t * cv_src, fvec3_t * pos_src, OVolume_t *cv_dst );
//bool    OVolume_unshift( OVolume_t * cv_src, fvec3_t * pos_src, OVolume_t *cv_dst );

    bool    OVolume_refine( OVolume_t * pov, fvec3_t * pcenter, float * pvolume );

//--------------------------------------------------------------------------------------------

    struct OVolume_Tree_t { OVolume_t leaf[8]; };

//--------------------------------------------------------------------------------------------

/// @details A convex polygon representation of the collision of two objects
    struct CVolume_t
    {
        CVolume_t() :
            volume(0.0f),
            center(0, 0, 0),
            ov(),
            tree(nullptr)
        {
            //ctor
        }

        float            volume;
        fvec3_t          center;
        OVolume_t        ov;
        OVolume_Tree_t  *tree;
    };

    CVolume_t * CVolume__blank( CVolume_t * );
    bool CVolume_ctor( CVolume_t * , const OVolume_t * pva, const OVolume_t * pvb );
    bool CVolume_refine( CVolume_t * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// type conversion routines

//bool bumper_to_oct_bb_1( const bumper_t src, const fvec3_t vel, oct_bb_t * pdst );

    egolib_rv oct_bb_downgrade( const oct_bb_t * psrc_bb, const bumper_t bump_stt, const bumper_t bump_base, bumper_t * pdst_bump, oct_bb_t * pdst_bb );

    int    oct_bb_to_points( const oct_bb_t * pbmp, fvec4_t pos[], size_t pos_count );
    void   points_to_oct_bb( oct_bb_t * pbmp, const fvec4_t pos[], const size_t pos_count );

//--------------------------------------------------------------------------------------------
//inline
//--------------------------------------------------------------------------------------------






egolib_rv oct_bb_copy(oct_bb_t *dst, const oct_bb_t *src);
egolib_rv oct_bb_copy_index(oct_bb_t *dst, const oct_bb_t *src, int index);


egolib_rv oct_bb_validate_index(oct_bb_t *self, int index);

bool oct_bb_empty(const oct_bb_t *self);
void oct_bb_set_ovec(oct_bb_t *self, const oct_vec_v2_t& v);
oct_bb_t *oct_bb_ctor_index(oct_bb_t *self, int index);


bool oct_bb_empty_index_raw(const oct_bb_t *self, int index);
bool oct_bb_empty_index(const oct_bb_t *self, int index);
egolib_rv oct_bb_union_index(const oct_bb_t *src1, const oct_bb_t *src2, oct_bb_t * pdst, int index );
egolib_rv oct_bb_intersection_index( const oct_bb_t *src1, const oct_bb_t *src2, oct_bb_t * pdst, int index );
egolib_rv oct_bb_union(const oct_bb_t *src1, const oct_bb_t  *src2, oct_bb_t *dst);
egolib_rv oct_bb_intersection(const oct_bb_t *src1, const oct_bb_t *src2, oct_bb_t *dst);


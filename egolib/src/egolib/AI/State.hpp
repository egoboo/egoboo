/// @file egolib/AI/State.hpp
/// @brief Abstract A.I. state for a single-object A.I.
/// @author Michael Heilmann
#pragma once

namespace AI {

/**
 * @brief Generic A.I. state for a single-object A.I.
 * @details A single-object A.I. is supposed to take control of one object.
 * The controlled object is referred to as "self". The full list of state constiuents is:
 *
 * - "self":       the object this state is the A.I. of i.e. the object controlled by the A.I.
 * - "target":     the target "self" is focusing on in the current cycle
 * - "oldTarget":  the target "self" was focusing on in the previous cycle
 * - "bumpedBy":   the last object "self" bumped into; wether "self" or the other initiate this process, is irrelevant in that case
  *
 * - "lastAttacker":   "self" was attacked by the "last attackeR" last
 * - "lastAttacked":   the "last attackeD" was attacked by "self" last
 *
 * @todo "lastAttacked" is not yet available.
 *
 * @tparam ObjectType the object type. The object type shall be
 *                    - equality-comparable (http://en.cppreference.com/w/cpp/concept/EqualityComparable)
 *                    - default-constructibled (http://en.cppreference.com/w/cpp/concept/DefaultConstructible)
 *                    - copy-constructible (http://en.cppreference.com/w/cpp/concept/CopyConstructible)
 *                    - copy-assignable (http://en.cppreference.com/w/cpp/concept/CopyAssignable)
 *                    - hashable (std::hash)
 *                    Furthermore, the default construct shall yield an
 *                    object which is invalid.
 */
template
<
    typename ObjectType, typename Equal = std::equal_to<ObjectType>, typename Hash = std::hash<ObjectType>,
    typename Enabled = typename std::enable_if
                             <
                               std::is_default_constructible<ObjectType>::value
                            && std::is_copy_constructible<ObjectType>::value
                            && std::is_copy_assignable<ObjectType>::value
                            >::type
>
struct State {

private:
    /** @brief "self". */
    ObjectType _self;
    /** @brief "target". */
    ObjectType _target;
    /** @brief "old target". */
    ObjectType _oldTarget;
    /** @brief "bumped". */
    ObjectType  _bumped;
    /** @brief "last attacker". */
    ObjectType _lastAttacker;

public:
    /// @brief Construct this state.
    /// @post
    /// - "self", "target", "old target", "bumped", "lastAttacker" are invalid.
    State()
        : _self(), _target(), _oldTarget(), _bumped(), _lastAttacker()
    { }

public:
    /// @brief Set the "self".
    /// @param the "self"
    void setSelf(const ObjectType& self) {
        _self = self;
    }
    /// @brief Get the "self".
    /// @return the "self"
    const ObjectType& getSelf() const {
        return _self;
    }

public:
    /// @brief Set the "target".
    /// @param the "target"
    void setTarget(const ObjectType& target) {
        _target = target;
    }
    /// @brief Get the "target".
    /// @return the "target"
    const ObjectType& getTarget() const {
        return _target;
    }

public:
    /// @brief Set the "old target".
    /// @param oldTarget the "old target"
    void setOldTarget(const ObjectType& oldTarget) {
        _oldTarget = oldTarget;
    }
    /// @brief Get the "old target".
    /// @return the "old target"
    const ObjectType& getOldTarget() const {
        return _oldTarget;
    }

public:
    /// @brief Set the "bumped".
    /// @param oldTarget the "bumped"
    void setBumped(const ObjectType& bumped) {
        _bumped = bumped;
    }
    /// @brief Get the "bumped".
    /// @return the "bumped"
    const ObjectType& getBumped() const {
        return _bumped;
    }

public:
    /// @brief Set the "last attacker"
    /// @param lastAttacker the "last attacker"
    void setLastAttacker(const ObjectType& lastAttacker) {
        _lastAttacker = lastAttacker;
    }
    /// @brief Get the "last attacker"
    /// @return the "last attacker"
    ObjectType getLastAttacker() const {
        return _lastAttacker;
    }

}; // struct state

} // namespace AI

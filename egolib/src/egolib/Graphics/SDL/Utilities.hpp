#pragma once

#include "egolib/platform.h"

class Requirement
{
public:
    /// @brief Construct this requirement.
    Requirement();
    /// @brief Destruct this requirement.
    virtual ~Requirement();
    /// @brief Reset the requirement to its initial value(s).
    virtual void reset() = 0;
    /// @brief Relax the requirement value(s).
    /// @return @a true if we were able to relax the requirement value(s),
    /// @a false if we were not able to relax the requirement value(s)
    virtual bool relax() = 0;
};

class AliasingRequirement : public Requirement
{
private:
    int m_oldValue;
public:
    AliasingRequirement();
    void reset() override;
    bool relax() override;
};

class FullscreenRequirement : public Requirement
{
private:
    bool m_oldValue;
public:
    FullscreenRequirement();
    void reset() override;
    bool relax() override;
};

class Requirements
{
public:
    std::deque<std::shared_ptr<Requirement>> requirements;
    void reset();
    bool relax();
};


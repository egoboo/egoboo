#pragma once

#include "idlib/utility.hpp"
#include "idlib/language.hpp"
#include "ScriptMigrator/tree_v1_forward.hpp"
#include "ScriptMigrator/tree_v2_forward.hpp"

// Rules are applied to nodes in any order until no more rules can be applied.
// Termination is not assured by this (wish you where here ... CTL library for C++).
class rule
{
    /// The type of a condition function.
    using condition_type = std::function<bool(const std::shared_ptr<node_v1>&)>;
    /// The type of an action function.
    using action_type = std::function<void(const std::shared_ptr<node_v1>&)>;
    /// @brief The condition.
    condition_type m_condition;
    /// @brief The action.
    action_type m_action;

public:
    rule(condition_type condition, action_type action)  :
        m_condition(condition), m_action(action)
    {}
    const condition_type& get_condition() const
    { return m_condition; }

    const action_type& get_action() const
    { return m_action; }
};

class rewriter_v1_v2
{
public:
    /// @brief Construct this rewriter.
    /// @param diagnostics a pointer to the diagnostics used by this rewriter
    rewriter_v1_v2(id::diagnostics *diagnostics);

    /// @brief Destruct this rewriter.
    virtual ~rewriter_v1_v2();

protected:
    /// @brief A pointer to the diagnostics used by this rewriter.
    id::diagnostics *m_diagnostics;

    std::shared_ptr<::word_v2> word(const std::shared_ptr<word_v1>& word);
    std::shared_ptr<::empty_line_v2> empty_line(const std::shared_ptr<::empty_line_v1>& empty_line_v1);
    std::shared_ptr<::line_v2> line(const std::shared_ptr<::line_v1>& line_v1);
    std::shared_ptr<::block_v2> block(const std::shared_ptr<::block_v1>& block_v1);
    std::shared_ptr<::statement_v2> statement(const std::shared_ptr<::statement_v1>& statement_v1);
    std::shared_ptr<::statement_list_v2> statement_list(const std::shared_ptr<::statement_list_v1>& statement_list_v1);
    std::shared_ptr<::program_v2> program(const std::shared_ptr<::program_v1>& program_v1);
public:
    std::shared_ptr<::program_v2> run(const std::shared_ptr<program_v1>& program_v1);
}; // class rewriter_v1_v2

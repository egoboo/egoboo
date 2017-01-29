#pragma once

namespace Ego {

// Different types of feedback.
enum class FeedbackType
{
    // Show no feedback.
    None = 0,
    // Show a descriptive text.
    Text,
    // Show damage as numbers.
    Number,
};

} // namespace Ego

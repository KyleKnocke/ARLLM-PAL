#pragma once
#include <string>

namespace pal {

// Count number of PAL/Ø glyphs (tokens) in a string.
// Each Unicode glyph (e.g., λ, ₁, ⁇, +) counts as one token.
int count_pal_tokens(const std::string& stream);

} // namespace pal
#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <regex>
#include <span>
#include <vector>

#include <anitomy/detail/delimiter.hpp>
#include <anitomy/detail/token.hpp>
#include <anitomy/element.hpp>

namespace anitomy::detail {

inline std::vector<Element> parse_video_resolution(std::span<Token> tokens) noexcept {
  using namespace std::views;

  // A video resolution can be in `1080p` or `1920x1080` format
  static constexpr auto is_video_resolution = [](const Token& token) {
    static const std::regex pattern{R"(\d{3,4}(?:[ipP]|(?:x|X|×)\d{3,4}[ipP]?))"};
    return std::regex_match(token.value, pattern);
  };

  std::vector<Element> elements;

  // Find all free tokens matching the pattern
  for (auto& token : tokens | filter(is_free_token) | filter(is_video_resolution)) {
    token.element_kind = ElementKind::VideoResolution;
    elements.emplace_back(ElementKind::VideoResolution, token.value, token.position);
  }

  // If not found, look for special cases
  if (elements.empty()) {
    static constexpr auto is_space_token = [](const Token& token) {
      return is_delimiter_token(token) && is_space(token.value.front());
    };

    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
      if (!is_free_token(*it) || !is_numeric_token(*it) || it->value != "1080") continue;

      // A number after a dash is an episode, e.g. `One Piece - 1080`
      const auto prev = std::find_if_not(std::make_reverse_iterator(it), tokens.rend(),
                                         is_space_token);
      if (prev != tokens.rend() && is_dash_token(*prev)) continue;

      it->element_kind = ElementKind::VideoResolution;
      elements.emplace_back(ElementKind::VideoResolution, it->value, it->position);
      break;
    }
  }

  return elements;
}

}  // namespace anitomy::detail

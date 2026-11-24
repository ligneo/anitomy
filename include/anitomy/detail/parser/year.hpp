#pragma once

#include <algorithm>
#include <optional>
#include <ranges>
#include <regex>
#include <span>
#include <tuple>

#include <anitomy/detail/token.hpp>
#include <anitomy/detail/util.hpp>
#include <anitomy/element.hpp>

namespace anitomy::detail {

inline std::optional<Element> parse_year(std::span<Token> tokens) noexcept {
  using namespace std::views;
  using window_t = std::tuple<Token&, Token&, Token&>;

  static constexpr auto is_isolated = [](window_t tokens) {
    return std::get<0>(tokens).kind == TokenKind::OpenBracket &&
           std::get<2>(tokens).kind == TokenKind::CloseBracket;
  };

  static constexpr auto is_free_number = [](window_t tokens) {
    auto& token = std::get<1>(tokens);
    return is_free_token(token) && is_numeric_token(token);
  };

  static constexpr auto is_year = [](window_t tokens) {
    const int number = to_int(std::get<1>(tokens).value);
    return 1950 < number && number < 2050;
  };

  // Find the first free isolated number within the interval
  auto view = tokens | adjacent<3> | filter(is_isolated) | filter(is_free_number) |
              filter(is_year) | take(1);

  if (!view.empty()) {
    auto& token = std::get<1>(view.front());
    token.element_kind = ElementKind::Year;
    return element_from_token(ElementKind::Year, token);
  }

  // Fall back to a number right before the season (e.g. `Title.2022.S01`), which would
  // otherwise end up in the title or be taken for an episode
  static constexpr auto is_season = [](const Token& token) {
    static const std::regex pattern{R"(S\d{1,2}(?:E\d{1,4})?)"};
    return std::regex_match(token.value, pattern);
  };

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    if (!is_free_token(*it) || !is_numeric_token(*it) || it->is_enclosed) continue;
    const int number = to_int(it->value);
    if (number <= 1950 || number >= 2050) continue;
    auto next = std::find_if(std::next(it), tokens.end(), is_not_delimiter_token);
    if (next == tokens.end() || !is_free_token(*next) || !is_season(*next)) continue;
    it->element_kind = ElementKind::Year;
    return element_from_token(ElementKind::Year, *it);
  }

  return {};
}

}  // namespace anitomy::detail

#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace doll {
constexpr std::uint32_t kReset=12100980, kUndo=12100981, kFail=12100982;
constexpr std::int32_t kOption=14000332, kFailText=14000334, kAccept=14000335, kUndoText=14000336;
bool rewrite(std::vector<std::uint8_t>& b,std::string* why);
}

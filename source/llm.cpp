#include "llm.hpp"

#include <string>
#include <format>

std::string dummy_message(const char* prompt)
{
    return std::format("Dummy response to prompt: {}", prompt);
}
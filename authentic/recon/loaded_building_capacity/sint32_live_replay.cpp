// Offline bounded replay of ORIGINAL GetValSint32 @ 0x10004f5c8.
// All expected conversion behavior comes from the approved production adapter.
#include "sint32_live_cases.h"
#include "opensc4/property_scalar_sint32.h"
#include <bit>
#include <charconv>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>

namespace {
std::uint64_t number(const std::string& text, int base = 10) {
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        throw std::runtime_error("invalid unsigned number");
    return value;
}
std::int32_t signed_number(const std::string& text) {
    std::int32_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        throw std::runtime_error("invalid signed number");
    return value;
}
void require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}
} // namespace

int main(int argc, char** argv) {
    bool synthetic = false;
    const char* path = nullptr;
    if (argc == 2) path = argv[1];
    else if (argc == 3 && std::string(argv[1]) == "--synthetic-smoke") {
        synthetic = true;
        path = argv[2];
    } else {
        std::cerr << "usage: sint32_live_replay [--synthetic-smoke] capture.txt\n";
        return 2;
    }
    std::size_t lineNumber = 0;
    try {
        std::ifstream file(path);
        require(file.is_open(), "cannot open capture");
        // Full lines only. The sole optional prefix is oracle logf_'s [HH:MM:SS].
        const std::regex stamp(R"(^\[(?:[01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]\] )");
        const std::regex begin(R"(SINT32_BEGIN origin=(ORIGINAL|SYNTHETIC) expected=([0-9]+) mxcsr=00001f80)");
        const std::regex item(R"(SINT32_CASE index=([0-9]+) tag=([0-9a-f]{4}) bits=([0-9a-f]{8}) ok=([01]) output=([0-9a-f]{8}) signed=(-?(?:0|[1-9][0-9]*)) unchanged=([01]))");
        const std::regex end(R"(SINT32_END complete=1 actual=([0-9]+) mxcsr_saved=([0-9a-f]{8}) mxcsr_restored=([0-9a-f]{8}) restored=1)");
        bool begun = false, ended = false;
        std::array<bool, sint32_live::kCount> seen{};
        std::size_t count = 0;
        std::string line;
        while (std::getline(file, line)) {
            ++lineNumber;
            require(!file.eof(), "unterminated capture line");
            require(line.size() <= 256, "oversized capture line");
            std::smatch match;
            if (std::regex_search(line, match, stamp)) line.erase(0, match.length());
            require(!ended, "data after END");
            if (std::regex_match(line, match, begin)) {
                require(!begun && count == 0, "duplicate/misplaced BEGIN");
                require(match[1].str() == (synthetic ? "SYNTHETIC" : "ORIGINAL"),
                        "capture origin does not match mode");
                require(number(match[2]) == sint32_live::kCount, "BEGIN count mismatch");
                begun = true;
            } else if (std::regex_match(line, match, item)) {
                require(begun, "CASE before BEGIN");
                const auto index = number(match[1]);
                require(index < sint32_live::kCount, "case index out of range");
                require(!seen[index], "duplicate case index");
                require(index == count, "case order mismatch");
                const auto input = sint32_live::CaseAt(index);
                require(number(match[2], 16) == input.tag && number(match[3], 16) == input.bits,
                        "case input differs from shared corpus");
                require(match[4] == "1", "original returned false");
                require(match[7] == "1", "original source changed");
                const auto originalBits = number(match[5], 16);
                const auto originalSigned = signed_number(match[6]);
                require(std::bit_cast<std::uint32_t>(originalSigned) == originalBits,
                        "inconsistent captured signed value/output bits");
                const opensc4::property_scalar::Scalar32 source{input.tag, input.bits};
                const auto before = source;
                std::int32_t output = 0x13579bdf;
                const auto status = opensc4::property_scalar::ReadSint32(source, output);
                require(source.runtimeTag == before.runtimeTag && source.bits == before.bits,
                        "production source changed");
                require(status == opensc4::property_scalar::ReadStatus::Value,
                        "production did not return Value");
                require(std::bit_cast<std::uint32_t>(output) == originalBits && output == originalSigned,
                        "production/original output mismatch");
                seen[index] = true;
                ++count;
            } else if (std::regex_match(line, match, end)) {
                require(begun && count == sint32_live::kCount, "incomplete case inventory");
                require(number(match[1]) == count, "END count mismatch");
                require(match[2].str() == match[3].str(), "MXCSR not restored");
                for (bool present : seen) require(present, "missing case index");
                ended = true;
            } else {
                throw std::runtime_error("malformed/unexpected line (including FAIL)");
            }
        }
        require(!file.bad(), "capture read error");
        require(begun && ended, "missing BEGIN/END (truncated capture)");
        std::cout << (synthetic ? "SYNTHETIC plumbing only" : "ORIGINAL capture replay")
                  << ": " << count << "/" << sint32_live::kCount
                  << " matched production ReadSint32; no full-state parity claim\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "REJECT line " << lineNumber << ": " << error.what() << '\n';
        return 1;
    }
}

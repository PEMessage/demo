#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

struct TaintFlag {
    int offset;
    std::string log;
    int mask;
    std::string reason;
};

// See: https://www.kernel.org/doc/html/v6.15/admin-guide/tainted-kernels.html
// /proc/sys/kernel/tainted
std::vector<TaintFlag> taint_flags = {
    {0, "G/P", 0x1, "proprietary module was loaded"},
    {1, " /F", 0x1, "module was force loaded"},
    {2, " /S", 0x1, "kernel running on an out of specification system"},
    {3, " /R", 0x1, "module was force unloaded"},
    {4, " /M", 0x1, "processor reported a Machine Check Exception (MCE)"},
    {5, " /B", 0x1, "bad page referenced or some unexpected page flags"},
    {6, " /U", 0x1, "taint requested by userspace application"},
    {7, " /D", 0x1, "kernel died recently, i.e. there was an OOPS or BUG"},
    {8, " /A", 0x1, "ACPI table overridden by user"},
    {9, " /W", 0x1, "kernel issued warning"},
    {10, " /C", 0x1, "staging driver was loaded"},
    {11, " /I", 0x1, "workaround for bug in platform firmware applied"},
    {12, " /O", 0x1, "externally-built (\"out-of-tree\") module was loaded"},
    {13, " /E", 0x1, "unsigned module was loaded"},
    {14, " /L", 0x1, "soft lockup occurred"},
    {15, " /K", 0x1, "kernel has been live patched"},
    {16, " /X", 0x1, "auxiliary taint, defined for and used by distros"},
    {17, " /T", 0x1, "kernel was built with the struct randomization plugin"},
    {18, " /N", 0x1, "an in-kernel test has been run"},
    {19, " /J", 0x1, "userspace used a mutating debug operation in fwctl"}
};

void parse_taint_flags(int taint_value) {
    std::cout << "Kernel taint value: 0x" << std::hex << taint_value << std::dec << " (" << taint_value << ")" << std::endl;
    std::cout << "Taint flags set:" << std::endl;
    
    bool any_flags = false;
    std::string log_string;
    
    for (const auto& flag : taint_flags) {
        if ((taint_value >> flag.offset) & flag.mask) {
            std::cout << "  Offset " << flag.offset << " (" << flag.log << "): " << flag.reason << std::endl;
            log_string += flag.log;
            any_flags = true;
        }
    }
    
    if (!any_flags) {
        std::cout << "  No taint flags are set (clean kernel)" << std::endl;
    } else {
        std::cout << "Concatenated log string: " << log_string << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <taint_value>" << std::endl;
        std::cerr << "Example: " << argv[0] << " `cat /proc/sys/kernel/tainted`" << std::endl;
        return 1;
    }
    
    try {
        int taint_value = std::stoi(argv[1], nullptr, 0); // Accepts hex (0x) and octal (0) prefixes
        parse_taint_flags(taint_value);
    } catch (const std::invalid_argument&) {
        std::cerr << "Error: Invalid taint value (must be an integer)" << std::endl;
        return 1;
    } catch (const std::out_of_range&) {
        std::cerr << "Error: Taint value out of range" << std::endl;
        return 1;
    }
    
    return 0;
}

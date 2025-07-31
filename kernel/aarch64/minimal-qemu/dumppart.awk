#!/usr/bin/awk -f

# Usage: awk -f this_script.awk -v begin_pattern="pattern" -v end_pattern="pattern" < input.log
# Or: ./this_script.awk -v begin_pattern="pattern" -v end_pattern="pattern" < input.log

BEGIN {
    # Initialize variables
    output_active = 0
}

{
    # Output line if active
    if (output_active) {
        print $0
    }

    # Check if line matches begin pattern
    if (begin_pattern != "" && $0 ~ begin_pattern) {
        output_active = 1
    }


    # Check if line matches end pattern
    if (end_pattern != "" && $0 ~ end_pattern) {
        output_active = 0
    }
}

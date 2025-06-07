#include <stdio.h>

// See:
//  C++ 中如何优雅进行 enum 到 string 的转换 ？
//  https://zhuanlan.zhihu.com/p/680412313
// X-macro definition
#define COLOR_ENUMS \
    COLOR_ENUM(RED) \
    COLOR_ENUM(GREEN) \
    COLOR_ENUM(BLUE)

// Enum definition
typedef enum {
#define COLOR_ENUM(x) x,
    COLOR_ENUMS
#undef COLOR_ENUM
    COLOR_COUNT  // Will be equal to number of colors
} Color;

// String conversion
const char* color_to_string(Color value) {
    switch(value) {
#define COLOR_ENUM(x) case x: return #x;
        COLOR_ENUMS
#undef COLOR_ENUM
        default: return "UNKNOWN";
    }
    return "UNKNOWN";
}

int main() {
    printf("Color Demonstration:\n");
    printf("--------------------\n");
    
    // Demo all defined colors
    for (int c = 0; c < COLOR_COUNT; c++) {
        printf("Enum value: %d -> Name: %s\n", c, color_to_string((Color)c));
    }
    
    // Test with an invalid value
    Color invalid = (Color)42;
    printf("Invalid value: %d -> Name: %s\n\n", invalid, color_to_string(invalid));
    
    // Example usage
    printf("Example usage:\n");
    Color favorite = GREEN;
    printf("My favorite color is %s (%d)\n", color_to_string(favorite), favorite);
    
    return 0;
}

// Clean up
#undef COLOR_ENUMS

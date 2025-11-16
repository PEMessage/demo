#include <stdio.h>

// q-gcc: -fms-extensions --

// 复杂的嵌套匿名结构体
struct GraphicsObject {
    int type;

    // 匿名结构体包含匿名联合体
    struct {
        char name[32];

        union {
            // 对于圆形
            struct {
                float radius;
                float center_x, center_y;
            };

            // 对于矩形
            struct {
                float width, height;
                float x, y;
            };
        };
    };
};

void print_circle(struct GraphicsObject *obj) {
    printf("Circle: %s\n", obj->name);
    printf("Radius: %.2f, Center: (%.2f, %.2f)\n",
           obj->radius, obj->center_x, obj->center_y);
}

int main() {
    struct GraphicsObject circle = {
        .type = 1,
        .name = "My Circle",
        .radius = 5.0f,
        .center_x = 10.0f,
        .center_y = 20.0f
    };

    print_circle(&circle);

    return 0;
}

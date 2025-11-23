// q-gcc: `pkg-config --cflags --libs gtk+-3.0 gtk-layer-shell-0` --
#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // 初始化 layer shell 前检查支持
    if (!gtk_layer_is_supported()) {
        g_printerr("Layer shell protocol not supported by your compositor!\n");
        return 1;
    } else {
        return 0;
    }
}

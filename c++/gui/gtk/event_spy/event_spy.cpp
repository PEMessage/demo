#include <gtkmm.h>
#include <iostream>
#include <magic_enum.hpp>

// q-gcc: `pkg-config --cflags --libs gtkmm-3.0` -I. --
int main()
{

    // Print GTK version
    std::cout << "GTK+ version: " << GTK_MAJOR_VERSION << "."
        << GTK_MINOR_VERSION << "." << GTK_MICRO_VERSION << std::endl;

    // Print gtkmm version
    std::cout << "gtkmm version: " << GTKMM_MAJOR_VERSION << "."
        << GTKMM_MINOR_VERSION << "." << GTKMM_MICRO_VERSION << std::endl;

    auto app = Gtk::Application::create();
    Gtk::Window window;
    window.set_default_size(1024, 768);
    app->signal_startup().connect([&]
    {
        app->add_window(window);
    });

    window.show();

    window.signal_event().connect([&](GdkEvent* event)->bool
    {
        std::cout<<"EVENT: "<< magic_enum::enum_name(event->type) << std::endl;
        return GDK_EVENT_PROPAGATE;
    });

    app->run();
    return 0;
}


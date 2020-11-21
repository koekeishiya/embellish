int g_connection;

int main(int argc, char **argv)
{
    g_connection = SLSMainConnectionID();

    struct component *spotify = component_create_shell(0.0125f, 0.01f, 0.20f, 0.03f, 0xffa3576e, 0xff262944, 0xff3e304a, "echo \"🎶 $(shpotify status artist) - $(shpotify status track)\"");
    struct component *focus = component_create_shell(0.40f, 0.01f, 0.20f, 0.03f, 0xff262626, 0xffa35763, 0xff3e304a, "yabai -m query --windows --window | jq -r '\"\\(.app) - \\(.title)\"'");
    struct component *power = component_create_power(0.88f, 0.01f, 0.05f, 0.03f, 0xffcd950c, 0xff191b27, 0xff353c54);
    struct component *time = component_create_time(0.94f, 0.01f, 0.0465f, 0.03f, 0xff647494, 0xff191b27, 0xff353c54);

    for (;;) {
        component_update(spotify);
        component_render(spotify);

        component_update(focus);
        component_render(focus);

        component_update(power);
        component_render(power);

        component_update(time);
        component_render(time);

        sleep(1);
    }
}

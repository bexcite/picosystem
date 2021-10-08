#include "hardware/spi.h"
#include "hardware/sync.h"
#include "pico/binary_info.h"

#include "libraries/picosystem.hpp"

using namespace picosystem;

extern "C" {
#include "picosystem.h"
#include "math.h"

//#define NOT_INITIALISED_MSG     "Cannot call this function, as picodisplay is not initialised. Call picodisplay.init(<bytearray>) first."

//mp_obj_t picodisplay_buf_obj;

// mp_obj_t picodisplay_init(mp_obj_t buf_obj) {
//     mp_buffer_info_t bufinfo;
//     mp_get_buffer_raise(buf_obj, &bufinfo, MP_BUFFER_RW);
//     picodisplay_buf_obj = buf_obj;
//     if(display == nullptr)
//         display = new PicoDisplay((uint16_t *)bufinfo.buf);
//     display->init();
//     return mp_const_none;
// }

uint32_t update_rate_ms = 10;
uint32_t pending_update_ms = 0;
uint32_t last_ms = time();

uint32_t tick = 0;


mp_obj_t picosystem_init() {

  _init_hardware();

  // setup lut for fast sin/cos functions
  for(uint32_t i = 0; i < 256; i++) {
    _fsin_lut[i] = sin((_PI * 2.0f) * (float(i) / 256.0f));
  }

  #ifndef NO_STARTUP_LOGO
    // fade in logo by ramping up backlight
    pen(0, 0, 0); clear();
    pen(15, 15, 15); _logo();
    for(int i = 0; i < 75; i++) {
      backlight(i);
      _wait_vsync();
      _flip();
    }

    sleep(300); // ...and breathe out...

    // fade out logo in 16 colour steps
    for(int i = 15; i >= 0; i--) {
      pen(0, 0, 0); clear();
      pen(i, i, i); _logo();
      _wait_vsync();
      _flip();

      sleep(20);
    }
  #else
    backlight(75);
  #endif

  sleep(300);

  pen(0, 0, 0); clear();

  // call users init() function so they can perform any needed
  // setup for world state etc
  //init();

  update_rate_ms = 10;
  pending_update_ms = 0;
  last_ms = time();

  tick = 0;

  _io = _gpio_get();

  return mp_const_none;
}

mp_obj_t picosystem_tick() {
  //while(true) {
    uint32_t ms = time();

    // work out how many milliseconds of updates we're waiting
    // to process and then call the users update() function as
    // many times as needed to catch up
    pending_update_ms += (ms - last_ms);
    while(pending_update_ms >= update_rate_ms) {
      _lio = _io;
      _io = _gpio_get();

      //update(tick++);
      pending_update_ms -= update_rate_ms;
    }

    // if current flipping the framebuffer in the background
    // then wait until that is complete before allow the user
    // to render
    while(_is_flipping()) {}

    // call user render function to draw world
    //draw();

    // wait for the screen to vsync before triggering flip
    // to ensure no tearing
    _wait_vsync();

    // flip the framebuffer to the screen
    _flip();

    last_ms = ms;
  //}


  return mp_const_none;
}

// state
mp_obj_t picosystem_pen(mp_uint_t n_args, const mp_obj_t *args) {
    switch(n_args) {
        case 1: {
            int p = mp_obj_get_int(args[0]);

            if(p < 0 || p > 0xffff) {
                mp_raise_ValueError("p is not a valid pen.");
            }
            else {
                pen(p);
            }
        } break;

        case 3:
        case 4: {
            int r = mp_obj_get_int(args[0]);
            int g = mp_obj_get_int(args[1]);
            int b = mp_obj_get_int(args[2]);

            if(r < 0 || r > 15) {
                mp_raise_ValueError("r out of range. Expected 0 to 15");
            }
            else if(g < 0 || g > 15) {
                mp_raise_ValueError("g out of range. Expected 0 to 15");
            }
            else if(b < 0 || b > 15) {
                mp_raise_ValueError("b out of range. Expected 0 to 15");
            }
            else {
                if(n_args == 4) {
                    int a = mp_obj_get_int(args[3]);
                    if(a < 0 || a > 15) {
                        mp_raise_ValueError("a out of range. Expected 0 to 15");
                    }
                    else {
                        pen(r, g, b, a);
                    }
                }
                else {
                    pen(r, g, b);
                }
            }
        } break;

        default: {
            char *buffer;
            buffer = (char*)malloc(100);
            sprintf(buffer, "function takes 1 (color), 3 (r,g,b), or 4 (r,g,b,a) positional arguments but %d were given", n_args);
            mp_raise_TypeError(buffer);
            free(buffer);
        } break;
    }

    return mp_const_none;
}

mp_obj_t picosystem_clip(mp_uint_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);
    clip(x, y, w, h);
    return mp_const_none;
}

mp_obj_t picosystem_blend(mp_obj_t bf_obj) {
    //TODO
    return mp_const_none;
}

mp_obj_t picosystem_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    //TODO
    return mp_const_none;
}

mp_obj_t picosystem_camera(mp_obj_t camx_obj, mp_obj_t camy_obj) {
    int camx = mp_obj_get_int(camx_obj);
    int camy = mp_obj_get_int(camy_obj);
    camera(camx, camy);
    return mp_const_none;
}

mp_obj_t picosystem_spritesheet(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    return mp_const_none;
}

// primitives
mp_obj_t picosystem_clear() {
    clear();
    return mp_const_none;
}

mp_obj_t picosystem_pixel(mp_obj_t x_obj, mp_obj_t y_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    pixel(x, y);
    return mp_const_none;
}

mp_obj_t picosystem_hline(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t c_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    int c = mp_obj_get_int(c_obj);
    hline(x, y, c);
    return mp_const_none;
}

mp_obj_t picosystem_vline(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t c_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    int c = mp_obj_get_int(c_obj);
    vline(x, y, c);
    return mp_const_none;
}

mp_obj_t picosystem_rect(mp_uint_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);
    rect(x, y, w, h);
    return mp_const_none;
}

mp_obj_t picosystem_circle(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t r_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    int r = mp_obj_get_int(r_obj);
    circle(x, y, r);
    return mp_const_none;
}

mp_obj_t picosystem_poly(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    //TODO
    return mp_const_none;
}

mp_obj_t picosystem_frect(mp_uint_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);
    frect(x, y, w, h);
    return mp_const_none;
}

mp_obj_t picosystem_fcircle(mp_obj_t x_obj, mp_obj_t y_obj, mp_obj_t r_obj) {
    int x = mp_obj_get_int(x_obj);
    int y = mp_obj_get_int(y_obj);
    int r = mp_obj_get_int(r_obj);
    fcircle(x, y, r);
    return mp_const_none;
}

mp_obj_t picosystem_fpoly(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    //TODO
    return mp_const_none;
}

mp_obj_t picosystem_line(mp_uint_t n_args, const mp_obj_t *args) {
    int x1 = mp_obj_get_int(args[0]);
    int y1 = mp_obj_get_int(args[1]);
    int x2 = mp_obj_get_int(args[2]);
    int y2 = mp_obj_get_int(args[3]);
    line(x1, y1, x2, y2);
    return mp_const_none;
}

mp_obj_t picosystem_blit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    //TODO
    return mp_const_none;
}

mp_obj_t picosystem_sprite(mp_uint_t n_args, const mp_obj_t *args) {
    int i = mp_obj_get_int(args[0]);
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);

    if(n_args == 4) {
        int flags = mp_obj_get_int(args[3]);
        sprite(i, x, y, flags);
    }
    else {
        sprite(i, x, y);
    }
    return mp_const_none;
}

mp_obj_t picosystem_text(mp_uint_t n_args, const mp_obj_t *args) {
    if(mp_obj_is_str_or_bytes(args[0])) {
        GET_STR_DATA_LEN(args[0], str, str_len);

        std::string t((const char*)str);

        if(n_args == 3) {
            int x = mp_obj_get_int(args[1]);
            int y = mp_obj_get_int(args[2]);
            text(t, x, y);
        }
        else {
            text(t);
        }
    }
    else if(mp_obj_is_float(args[0])) {
        mp_raise_TypeError("can't convert 'float' object to str implicitly");
    }
    else if(mp_obj_is_int(args[0])) {
        mp_raise_TypeError("can't convert 'int' object to str implicitly");
    }
    else if(mp_obj_is_bool(args[0])) {
        mp_raise_TypeError("can't convert 'bool' object to str implicitly");
    }
    else {
        mp_raise_TypeError("can't convert object to str implicitly");
    }

    return mp_const_none;
}

// utility
mp_obj_t picosystem_rgb(mp_uint_t n_args, const mp_obj_t *args) {
    int r = mp_obj_get_int(args[0]);
    int g = mp_obj_get_int(args[1]);
    int b = mp_obj_get_int(args[2]);
    if(r < 0 || r > 15) {
        mp_raise_ValueError("r out of range. Expected 0 to 15");
    }
    else if(g < 0 || g > 15) {
        mp_raise_ValueError("g out of range. Expected 0 to 15");
    }
    else if(b < 0 || b > 15) {
        mp_raise_ValueError("b out of range. Expected 0 to 15");
    }
    else {
        if(n_args == 4) {
            int a = mp_obj_get_int(args[3]);
            if(a < 0 || a > 15) {
                mp_raise_ValueError("a out of range. Expected 0 to 15");
            }
            else {
                return mp_obj_new_int(rgb(r, g, b, a));
            }
        }
        else {
            return mp_obj_new_int(rgb(r, g, b));
        }
    }
    return mp_obj_new_int(0);
}

mp_obj_t picosystem_hsv(mp_uint_t n_args, const mp_obj_t *args) {
    float h = mp_obj_get_float(args[0]);
    float s = mp_obj_get_float(args[1]);
    float v = mp_obj_get_float(args[2]);
    if(h < 0.0f || h > 1.0f) {
        mp_raise_ValueError("h out of range. Expected 0.0 to 1.0");
    }
    else if(s < 0.0f || s > 1.0f) {
        mp_raise_ValueError("s out of range. Expected 0.0 to 1.0");
    }
    else if(v < 0.0f || v > 1.0f) {
        mp_raise_ValueError("v out of range. Expected 0.0 to 1.0");
    }
    else {
        if(n_args == 4) {
            float a = mp_obj_get_float(args[3]);
            if(a < 0.0f || a > 1.0f) {
                mp_raise_ValueError("v out of range. Expected 0.0 to 1.0");
            }
            return mp_obj_new_int(hsv(h, s, v, a));
        }
        else {
            return mp_obj_new_int(hsv(h, s, v));
        }
    }
    return mp_obj_new_int(0);
}

mp_obj_t picosystem_intersects(mp_uint_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int w = mp_obj_get_int(args[2]);
    int h = mp_obj_get_int(args[3]);
    int cx = mp_obj_get_int(args[4]);
    int cy = mp_obj_get_int(args[5]);
    int cw = mp_obj_get_int(args[6]);
    int ch = mp_obj_get_int(args[7]);
    return mp_obj_new_bool(intersects(x, y, w, h, cx, cy, cw, ch));
}

mp_obj_t picosystem_intersection(mp_uint_t n_args, const mp_obj_t *args) {
    int32_t x = mp_obj_get_int(args[0]);
    int32_t y = mp_obj_get_int(args[1]);
    int32_t w = mp_obj_get_int(args[2]);
    int32_t h = mp_obj_get_int(args[3]);
    int cx = mp_obj_get_int(args[4]);
    int cy = mp_obj_get_int(args[5]);
    int cw = mp_obj_get_int(args[6]);
    int ch = mp_obj_get_int(args[7]);

    intersection(x, y, w, h, cx, cy, cw, ch);

    mp_obj_t tuple[4];
    tuple[0] = mp_obj_new_int(x);
    tuple[1] = mp_obj_new_int(y);
    tuple[2] = mp_obj_new_int(w);
    tuple[3] = mp_obj_new_int(h);
    return mp_obj_new_tuple(4, tuple);
}

mp_obj_t picosystem_contains(mp_uint_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    int cx = mp_obj_get_int(args[2]);
    int cy = mp_obj_get_int(args[3]);
    int cw = mp_obj_get_int(args[4]);
    int ch = mp_obj_get_int(args[5]);
    return mp_obj_new_bool(contains(x, y, cx, cy, cw, ch));
}

// hardware
mp_obj_t picosystem_pressed(mp_obj_t b_obj) {
    bool button_pressed = false;

    int b = mp_obj_get_int(b_obj);
    switch(b) {
        case button::UP:
        case button::DOWN:
        case button::LEFT:
        case button::RIGHT:
        case button::A:
        case button::B:
        case button::X:
        case button::Y:
            button_pressed = pressed(b);
            break;

        default:
            mp_raise_ValueError("button not valid.");
            break;
    }

    return button_pressed ? mp_const_true : mp_const_false;
}

mp_obj_t picosystem_button(mp_obj_t b_obj) {
    bool button_down = false;

    int b = mp_obj_get_int(b_obj);
    switch(b) {
        case button::UP:
        case button::DOWN:
        case button::LEFT:
        case button::RIGHT:
        case button::A:
        case button::B:
        case button::X:
        case button::Y:
            button_down = button(b);
            break;

        default:
            mp_raise_ValueError("button not valid.");
            break;
    }

    return button_down ? mp_const_true : mp_const_false;
}

mp_obj_t picosystem_battery() {
    return mp_obj_new_int(battery());
}

mp_obj_t picosystem_led(mp_obj_t r_obj, mp_obj_t g_obj, mp_obj_t b_obj) {
    int r = mp_obj_get_int(r_obj);
    int g = mp_obj_get_int(g_obj);
    int b = mp_obj_get_int(b_obj);

    if(r < 0 || r > 100) {
        mp_raise_ValueError("r out of range. Expected 0 to 100");
    }
    else if(g < 0 || g > 100) {
        mp_raise_ValueError("g out of range. Expected 0 to 100");
    }
    else if(b < 0 || b > 100) {
        mp_raise_ValueError("b out of range. Expected 0 to 100");
    }
    else {
        led(r, g, b);
    }

    return mp_const_none;
}

mp_obj_t picosystem_backlight(mp_obj_t b_obj) {
    int b = mp_obj_get_int(b_obj);

    if(b < 0 || b > 100) {
        mp_raise_ValueError("b out of range. Expected 0 to 100");
    }
    else {
        backlight(b);
    }

    return mp_const_none;
}
}
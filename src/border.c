extern int g_connection;
extern struct window_manager g_window_manager;

static void border_order_in(struct window *window)
{
    SLSOrderWindow(g_connection, window->border.id, -1, window->id);
    scripting_addition_add_to_window_ordering_group(window->id, window->border.id);
}

static void border_order_out(struct window *window)
{
    scripting_addition_remove_from_window_ordering_group(window->id, window->border.id);
    SLSOrderWindow(g_connection, window->border.id, 0, window->id);
}

void border_redraw(struct window *window)
{
    SLSDisableUpdate(g_connection);
    CGContextClearRect(window->border.context, window->border.frame);
    CGContextAddPath(window->border.context, window->border.path);
    CGContextDrawPath(window->border.context, kCGPathFillStroke);
    // CGContextStrokePath(window->border.context);
    CGContextFlush(window->border.context);
    SLSReenableUpdate(g_connection);
}

void border_activate(struct window *window)
{
    if (!window->border.id) return;

    CGContextSetRGBStrokeColor(window->border.context,
                               g_window_manager.active_border_color.r,
                               g_window_manager.active_border_color.g,
                               g_window_manager.active_border_color.b,
                               g_window_manager.active_border_color.a);
    CGContextSetRGBFillColor(window->border.context, 1, 1, 1, 0x15 / 255.);
    border_redraw(window);
}

void border_deactivate(struct window *window)
{
    if (!window->border.id) return;

    CGContextSetRGBStrokeColor(window->border.context,
                               g_window_manager.normal_border_color.r,
                               g_window_manager.normal_border_color.g,
                               g_window_manager.normal_border_color.b,
                               g_window_manager.normal_border_color.a);
    CGContextSetRGBFillColor(window->border.context, 1, 1, 1, 0x15 / 255.);
    border_redraw(window);
}

void border_enter_fullscreen(struct window *window)
{
    if (!window->border.id) return;

    border_order_out(window);
}

void border_exit_fullscreen(struct window *window)
{
    if (!window->border.id) return;

    border_order_in(window);
}

extern CGError SLSWindowSetShadowProperties(uint32_t wid, CFDictionaryRef options);
static inline void border_disable_shadow(struct window* window)
{
    int t = 0;
    CFNumberRef density = CFNumberCreate(NULL, kCFNumberSInt32Type, &t);
    const void *keys[1] = { CFSTR("com.apple.WindowShadowDensity") };
    const void *values[1] = { density };
    CFDictionaryRef options = CFDictionaryCreate(NULL, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    SLSWindowSetShadowProperties(window->border.id, options);
    CFRelease(density);
    CFRelease(options);
}

extern CGError SLSSetWindowBackgroundBlurRadius(int cid, uint32_t wid, uint32_t radius);
void border_create(struct window *window)
{
    if (window->border.id) return;

    if ((!window->rule_manage) && (!window_is_standard(window)) && (!window_is_dialog(window))) return;

    CGRect frame = CGRectInset(window->frame, -4, -4);
    CGSNewRegionWithRect(&frame, &window->border.region);
    window->border.frame.size = frame.size;
    window->border.frame.origin = CGPointZero;

    window->border.path = CGPathCreateMutable();
    frame.size = window->frame.size;
    frame.origin = (CGPoint){4, 4};
    CGPathAddRoundedRect(window->border.path, NULL, frame, 9, 9);

    uint64_t tags = kCGSIgnoreForEventsTagBit | kCGSDisableShadowTagBit;
    SLSNewWindow(g_connection, 2, 0, 0, window->border.region, &window->border.id);
    SLSSetWindowTags(g_connection, window->border.id, &tags, 64);
    SLSSetWindowResolution(g_connection, window->border.id, 2.0f);
    SLSSetWindowOpacity(g_connection, window->border.id, 0);
    SLSSetWindowLevel(g_connection, window->border.id, window_level(window));
    SLSSetWindowBackgroundBlurRadius(g_connection, window->border.id, 20);
    border_disable_shadow(window);
    window->border.context = SLWindowContextCreate(g_connection, window->border.id, 0);
    CGContextSetLineWidth(window->border.context, g_window_manager.border_width);
    CGContextSetRGBStrokeColor(window->border.context,
                               g_window_manager.normal_border_color.r,
                               g_window_manager.normal_border_color.g,
                               g_window_manager.normal_border_color.b,
                               g_window_manager.normal_border_color.a);
    CGContextSetRGBFillColor(window->border.context,
                               g_window_manager.normal_border_color.r,
                               g_window_manager.normal_border_color.g,
                               g_window_manager.normal_border_color.b,
                               g_window_manager.normal_border_color.a);
    window->border.in_movement_group = false;

    border_redraw(window);

    if ((!window->application->is_hidden) &&
        (!window->is_minimized) &&
        (!window->is_fullscreen)) {
        border_order_in(window);
    }
}

void border_resize(struct window *window)
{
    if (!window->border.id) return;

    if (window->border.region) CFRelease(window->border.region);
    CGRect frame = CGRectInset(window->frame, -4, -4);
    CGSNewRegionWithRect(&frame, &window->border.region);
    window->border.frame.size = frame.size;
    window->border.frame.origin = CGPointZero;

    if (window->border.path) CGPathRelease(window->border.path);
    window->border.path = CGPathCreateMutable();

    frame.size = window->frame.size;
    frame.origin = (CGPoint){4, 4};
    CGPathAddRoundedRect(window->border.path, NULL, frame, 9, 9);

    SLSDisableUpdate(g_connection);
    SLSSetWindowShape(g_connection, window->border.id, 0.0f, 0.0f, window->border.region);
    CGContextClearRect(window->border.context, CGRectInset(window->border.frame, -1, -1));
    CGContextAddPath(window->border.context, window->border.path);
    CGContextDrawPath(window->border.context, kCGPathFillStroke);
    // CGContextStrokePath(window->border.context);
    CGContextFlush(window->border.context);
    SLSReenableUpdate(g_connection);
}

void border_destroy(struct window *window)
{
    if (!window->border.id) return;

    if (window->border.region) CFRelease(window->border.region);
    if (window->border.path) CGPathRelease(window->border.path);
    CGContextRelease(window->border.context);
    SLSReleaseWindow(g_connection, window->border.id);
    memset(&window->border, 0, sizeof(struct border));
}

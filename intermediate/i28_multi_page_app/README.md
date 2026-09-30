# i26 — Multi-Page Navigation App

Production-quality page navigation example demonstrating the same patterns
used in the TESAIoT production firmware, without depending on `page_manager.h`.

## Architecture

- **Page definitions**: static array of `{title, icon, create_fn}` structs
- **Navigation stack**: array-based push/pop for back navigation (max depth 8)
- **Tab bar**: bottom tab bar with 3 tabs, active tab highlighted
- **Page lifecycle**: `lv_obj_clean()` destroys old page, `create_fn()` builds new one
- **Timer management**: sensor timer created/destroyed with page transitions

## Pages

| Page     | Content                                             |
|----------|-----------------------------------------------------|
| Home     | Welcome card, navigation buttons, stack depth info  |
| Sensors  | BMI270 accel + gyro with 200ms live update timer    |
| Settings | Brightness slider, dark theme toggle, about info    |

## Key Techniques

- `navigate_to(page_id, push_stack)` — clears content, creates new page, updates tabs
- `navigate_back()` — pops stack and navigates to previous page
- Timer lifecycle tied to page: sensor timer only runs on Sensors page
- Tab bar event callbacks use `intptr_t` user data for page ID
- `lv_obj_clean()` for efficient page teardown (LVGL handles child cleanup)

## Navigation Flow

```
Home ─[tap "Sensors"]─> Sensors ─[tap "Back"]─> Home
Home ─[tap "Settings"]─> Settings ─[tap tab]─> any page
```

Tab bar is always visible; back buttons provide stack-based navigation.

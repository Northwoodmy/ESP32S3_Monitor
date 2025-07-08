// Minimal internationalization header for SquareLine Studio UI
// This file provides basic i18n support for the ESP32S3 Monitor project

#ifndef LV_I18N_H
#define LV_I18N_H

#ifdef __cplusplus
extern "C" {
#endif

// Basic internationalization support
// For this project, we use Chinese as the primary language
// No complex translation system is needed

// Placeholder function for text localization
// In this implementation, it simply returns the input string
static inline const char* _(const char* text) {
    return text;
}

// Language codes (for future expansion)
typedef enum {
    LV_I18N_LANG_ZH_CN = 0,  // Simplified Chinese (default)
    LV_I18N_LANG_EN_US,      // English (optional)
} lv_i18n_lang_t;

// Current language (default to Chinese)
extern lv_i18n_lang_t lv_i18n_current_lang;

// Function declarations (minimal implementation)
void lv_i18n_init(void);
void lv_i18n_set_lang(lv_i18n_lang_t lang);
const char* lv_i18n_get_text(const char* key);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // LV_I18N_H 
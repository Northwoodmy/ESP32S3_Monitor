// Minimal internationalization implementation for SquareLine Studio UI
// This file provides basic i18n support for the ESP32S3 Monitor project

#include "lv_i18n.h"
#include <string.h>

// Current language setting (default to Simplified Chinese)
lv_i18n_lang_t lv_i18n_current_lang = LV_I18N_LANG_ZH_CN;

/**
 * @brief Initialize internationalization system
 */
void lv_i18n_init(void) {
    // Set default language to Simplified Chinese
    lv_i18n_current_lang = LV_I18N_LANG_ZH_CN;
}

/**
 * @brief Set current language
 * @param lang Language to set
 */
void lv_i18n_set_lang(lv_i18n_lang_t lang) {
    lv_i18n_current_lang = lang;
}

/**
 * @brief Get localized text
 * @param key Text key to translate
 * @return Localized text (in this simple implementation, returns the key itself)
 */
const char* lv_i18n_get_text(const char* key) {
    if (key == NULL) {
        return "";
    }
    
    // For this project, we simply return the original text
    // In a full i18n implementation, this would look up translations
    return key;
} 
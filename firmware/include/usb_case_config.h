#ifndef USB_CASE_CONFIG_H
#define USB_CASE_CONFIG_H

#if defined(__has_include)
#if __has_include("usb_case_config.generated.h")
#include "usb_case_config.generated.h"
#endif
#endif

#ifndef USB_CASE_ID
#define USB_CASE_ID 0u
#endif

#ifndef USB_CASE_NAME
#define USB_CASE_NAME "baseline_cdc"
#endif

#ifndef USB_CASE_GROUP
#define USB_CASE_GROUP "baseline"
#endif

#ifndef USB_CASE_BASE_PERSONA
#define USB_CASE_BASE_PERSONA "cdc_acm"
#endif

#ifndef USB_CASE_MUTATION_SUMMARY
#define USB_CASE_MUTATION_SUMMARY "none"
#endif

/* Planned descriptor personas for usb_case_demo. */
#define USB_CASE_PERSONA_CDC_ACM 0u
#define USB_CASE_PERSONA_HID_BASELINE 1u

/*
 * Selection by case id for the first descriptor-layer iteration:
 * - 000 -> baseline CDC/ACM persona
 * - 001 -> baseline HID persona (planned; descriptor callbacks not wired yet)
 * Other IDs fall back to CDC/ACM for safe behavior.
 */
#if (USB_CASE_ID == 1u)
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_HID_BASELINE
#else
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_CDC_ACM
#endif

#endif /* USB_CASE_CONFIG_H */

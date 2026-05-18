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

/* Descriptor personas for usb_case_demo. */
#define USB_CASE_PERSONA_CDC_ACM 0u
#define USB_CASE_PERSONA_HID_BASELINE 1u
#define USB_CASE_PERSONA_COMPOSITE_CDC_HID 2u
#define USB_CASE_PERSONA_MSC_BASELINE 3u
#define USB_CASE_PERSONA_COMPOSITE_CDC_MSC 4u
#define USB_CASE_PERSONA_COMPOSITE_HID_MSC 5u

/*
 * Selection by case id:
 * - 000 -> baseline CDC/ACM persona
 * - 001 -> baseline HID persona (inert, no keyboard/mouse input behavior)
 * - 002 -> composite CDC+HID persona (inert HID, no keyboard/mouse input behavior)
 * - 050/051/052/053/054/055/056/057/058/059 -> MSC read-only RAM-backed persona
 * - 060/062/063/064/065 -> composite CDC+MSC persona (CDC diagnostic + read-only RAM-backed MSC)
 * - 061 -> composite HID+MSC persona (inert HID + read-only RAM-backed MSC)
 * Other IDs fall back to CDC/ACM for safe behavior.
 */
#if (USB_CASE_ID == 1u)
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_HID_BASELINE
#elif (USB_CASE_ID == 2u)
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_COMPOSITE_CDC_HID
#elif (USB_CASE_ID == 60u) || (USB_CASE_ID == 62u) || (USB_CASE_ID == 63u) || (USB_CASE_ID == 64u) || (USB_CASE_ID == 65u)
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_COMPOSITE_CDC_MSC
#elif (USB_CASE_ID == 61u)
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_COMPOSITE_HID_MSC
#elif (USB_CASE_ID == 50u) || (USB_CASE_ID == 51u) || (USB_CASE_ID == 52u) || (USB_CASE_ID == 53u) || (USB_CASE_ID == 54u) || (USB_CASE_ID == 55u) || (USB_CASE_ID == 56u) || (USB_CASE_ID == 57u) || (USB_CASE_ID == 58u) || (USB_CASE_ID == 59u)
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_MSC_BASELINE
#else
#define USB_CASE_PERSONA_ID USB_CASE_PERSONA_CDC_ACM
#endif

#endif /* USB_CASE_CONFIG_H */

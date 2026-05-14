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

#endif /* USB_CASE_CONFIG_H */

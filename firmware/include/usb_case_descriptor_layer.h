#ifndef USB_CASE_DESCRIPTOR_LAYER_H
#define USB_CASE_DESCRIPTOR_LAYER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct usb_case_descriptor_profile {
    uint32_t persona_id;
    const char *persona_name;
    bool descriptors_switched;
    const char *active_transport;
} usb_case_descriptor_profile_t;

usb_case_descriptor_profile_t usb_case_get_descriptor_profile(void);
bool usb_case_persona_has_cdc(void);

#endif /* USB_CASE_DESCRIPTOR_LAYER_H */

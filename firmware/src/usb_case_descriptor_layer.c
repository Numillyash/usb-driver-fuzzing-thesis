#include "usb_case_descriptor_layer.h"

#include "usb_case_config.h"

usb_case_descriptor_profile_t usb_case_get_descriptor_profile(void)
{
    usb_case_descriptor_profile_t profile = {
        .persona_id = USB_CASE_PERSONA_CDC_ACM,
        .persona_name = "cdc_acm",
        .descriptors_switched = false,
        .active_transport = "pico_stdio_usb_cdc",
    };

    if (USB_CASE_PERSONA_ID == USB_CASE_PERSONA_HID_BASELINE) {
        /*
         * Planned persona: HID baseline.
         * Current iteration keeps safe CDC stdio transport and does not
         * yet install TinyUSB descriptor callbacks for HID descriptors.
         */
        profile.persona_id = USB_CASE_PERSONA_HID_BASELINE;
        profile.persona_name = "hid_baseline_planned";
    }

    return profile;
}

#include "usb_case_descriptor_layer.h"

#include "usb_case_config.h"

static bool usb_case_is_hid_persona(void)
{
    return USB_CASE_PERSONA_ID == USB_CASE_PERSONA_HID_BASELINE;
}

bool usb_case_persona_has_cdc(void)
{
    return !usb_case_is_hid_persona();
}

usb_case_descriptor_profile_t usb_case_get_descriptor_profile(void)
{
    usb_case_descriptor_profile_t profile = {
        .persona_id = USB_CASE_PERSONA_CDC_ACM,
        .persona_name = "cdc_acm",
        .descriptors_switched = true,
        .active_transport = "pico_stdio_usb_cdc",
    };

    if (usb_case_is_hid_persona()) {
        profile.persona_id = USB_CASE_PERSONA_HID_BASELINE;
        profile.persona_name = "hid_generic_no_input";
        profile.active_transport = "tinyusb_hid_inert_no_cdc";
    }

    return profile;
}

#if defined(USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS) && (USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS == 1)
#include <string.h>

#include "pico/unique_id.h"
#include "tusb.h"

enum {
    USB_CASE_ITF_NUM_CDC_COMM = 0,
    USB_CASE_ITF_NUM_CDC_DATA = 1,
    USB_CASE_ITF_NUM_HID = 0,
};

enum {
    USB_CASE_ENDPOINT_CDC_NOTIF = 0x81,
    USB_CASE_ENDPOINT_CDC_OUT = 0x02,
    USB_CASE_ENDPOINT_CDC_IN = 0x82,
    USB_CASE_ENDPOINT_HID_IN = 0x81,
};

enum {
    USB_CASE_STRIDX_LANGID = 0,
    USB_CASE_STRIDX_MANUFACTURER = 1,
    USB_CASE_STRIDX_PRODUCT = 2,
    USB_CASE_STRIDX_SERIAL = 3,
    USB_CASE_STRIDX_CDC = 4,
    USB_CASE_STRIDX_HID = 5,
};

enum {
    USB_CASE_CDC_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN,
    USB_CASE_HID_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
};

static const tusb_desc_device_t usb_case_device_desc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x2E8A,
    .idProduct = 0x4005,
    .bcdDevice = 0x0100,
    .iManufacturer = USB_CASE_STRIDX_MANUFACTURER,
    .iProduct = USB_CASE_STRIDX_PRODUCT,
    .iSerialNumber = USB_CASE_STRIDX_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t usb_case_cfg_desc_cdc[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, USB_CASE_CDC_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(
        USB_CASE_ITF_NUM_CDC_COMM,
        USB_CASE_STRIDX_CDC,
        USB_CASE_ENDPOINT_CDC_NOTIF,
        8,
        USB_CASE_ENDPOINT_CDC_OUT,
        USB_CASE_ENDPOINT_CDC_IN,
        64),
};

static const uint8_t usb_case_hid_report_desc[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT(8),
};

static const uint8_t usb_case_cfg_desc_hid[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, USB_CASE_HID_TOTAL_LEN, 0x00, 100),
    TUD_HID_DESCRIPTOR(
        USB_CASE_ITF_NUM_HID,
        USB_CASE_STRIDX_HID,
        HID_ITF_PROTOCOL_NONE,
        sizeof(usb_case_hid_report_desc),
        USB_CASE_ENDPOINT_HID_IN,
        CFG_TUD_HID_EP_BUFSIZE,
        10),
};

static const char *const usb_case_cdc_strings[] = {
    "",
    "RP2040 USB Research",
    "usb_case_custom_demo CDC baseline",
    "",
    "USB Case CDC",
};

static const char *const usb_case_hid_strings[] = {
    "",
    "RP2040 USB Research",
    "usb_case_custom_demo HID baseline",
    "",
    "USB Case HID (inert)",
};

static const char *usb_case_get_serial_string(void)
{
    static bool initialized = false;
    static char serial[(2u * PICO_UNIQUE_BOARD_ID_SIZE_BYTES) + 1u];

    if (!initialized) {
        pico_get_unique_board_id_string(serial, sizeof(serial));
        initialized = true;
    }

    return serial;
}

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&usb_case_device_desc;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return usb_case_persona_has_cdc() ? usb_case_cfg_desc_cdc : usb_case_cfg_desc_hid;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return usb_case_hid_report_desc;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t desc_str[32];
    const char *str = NULL;
    const char *const *table = usb_case_persona_has_cdc() ? usb_case_cdc_strings : usb_case_hid_strings;
    const uint8_t table_len = usb_case_persona_has_cdc() ? (uint8_t)(sizeof(usb_case_cdc_strings) / sizeof(usb_case_cdc_strings[0]))
                                                         : (uint8_t)(sizeof(usb_case_hid_strings) / sizeof(usb_case_hid_strings[0]));
    size_t char_count = 0u;

    (void)langid;

    if (index == USB_CASE_STRIDX_LANGID) {
        desc_str[1] = 0x0409;
        char_count = 1u;
    } else {
        if (index >= table_len) {
            return NULL;
        }
        str = table[index];
        if (index == USB_CASE_STRIDX_SERIAL) {
            str = usb_case_get_serial_string();
        }
        if (str == NULL) {
            str = "";
        }
        char_count = strlen(str);
        if (char_count > 31u) {
            char_count = 31u;
        }
        for (size_t i = 0u; i < char_count; i++) {
            desc_str[1u + i] = (uint8_t)str[i];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * (char_count + 1u)));
    return desc_str;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
#endif

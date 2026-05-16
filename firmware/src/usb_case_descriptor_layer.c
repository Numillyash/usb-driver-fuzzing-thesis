#include "usb_case_descriptor_layer.h"

#include "usb_case_config.h"

static bool usb_case_is_hid_persona(void)
{
    return USB_CASE_PERSONA_ID == USB_CASE_PERSONA_HID_BASELINE;
}

static bool usb_case_is_composite_persona(void)
{
    return USB_CASE_PERSONA_ID == USB_CASE_PERSONA_COMPOSITE_CDC_HID;
}

static bool usb_case_is_msc_persona(void)
{
    return USB_CASE_PERSONA_ID == USB_CASE_PERSONA_MSC_BASELINE;
}

bool usb_case_persona_has_cdc(void)
{
    return !usb_case_is_hid_persona() && !usb_case_is_msc_persona();
}

usb_case_descriptor_profile_t usb_case_get_descriptor_profile(void)
{
    usb_case_descriptor_profile_t profile = {
        .persona_id = USB_CASE_PERSONA_CDC_ACM,
        .persona_name = "cdc_acm",
#if defined(USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS) && (USB_CASE_ENABLE_TINYUSB_CUSTOM_DESCRIPTORS == 1)
        .descriptors_switched = true,
#else
        .descriptors_switched = false,
#endif
        .active_transport = "pico_stdio_usb_cdc",
    };

    if (usb_case_is_hid_persona()) {
        profile.persona_id = USB_CASE_PERSONA_HID_BASELINE;
        profile.persona_name = "hid_generic_no_input";
        profile.active_transport = "tinyusb_hid_inert_no_cdc";
    } else if (usb_case_is_composite_persona()) {
        profile.persona_id = USB_CASE_PERSONA_COMPOSITE_CDC_HID;
        profile.persona_name = "composite_cdc_hid_inert";
        profile.active_transport = "tinyusb_cdc_plus_hid_inert";
    } else if (usb_case_is_msc_persona()) {
        profile.persona_id = USB_CASE_PERSONA_MSC_BASELINE;
        profile.persona_name = "msc_inert_readonly";
        profile.active_transport = "tinyusb_msc_readonly_ram";
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
    USB_CASE_ITF_NUM_HID = 2,
    USB_CASE_ITF_NUM_MSC = 0,
};

enum {
    USB_CASE_ENDPOINT_CDC_NOTIF = 0x81,
    USB_CASE_ENDPOINT_CDC_OUT = 0x02,
    USB_CASE_ENDPOINT_CDC_IN = 0x82,
    USB_CASE_ENDPOINT_HID_IN = 0x83,
    USB_CASE_ENDPOINT_MSC_OUT = 0x01,
    USB_CASE_ENDPOINT_MSC_IN = 0x81,
};

enum {
    USB_CASE_STRIDX_LANGID = 0,
    USB_CASE_STRIDX_MANUFACTURER = 1,
    USB_CASE_STRIDX_PRODUCT = 2,
    USB_CASE_STRIDX_SERIAL = 3,
    USB_CASE_STRIDX_CDC = 4,
    USB_CASE_STRIDX_HID = 5,
    USB_CASE_STRIDX_MSC = 4,
};

enum {
#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1)
    USB_CASE_CDC_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN,
#endif
#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
    USB_CASE_HID_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
#endif
#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1) && defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
    USB_CASE_COMPOSITE_CDC_HID_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN,
#endif
#if defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
    USB_CASE_MSC_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN,
#endif
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

static const tusb_desc_device_t usb_case_device_desc_msc = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00u,
    .bDeviceSubClass = 0x00u,
    .bDeviceProtocol = 0x00u,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x2E8A,
    .idProduct = 0x4006,
    .bcdDevice = 0x0100,
    .iManufacturer = USB_CASE_STRIDX_MANUFACTURER,
    .iProduct = USB_CASE_STRIDX_PRODUCT,
    .iSerialNumber = USB_CASE_STRIDX_SERIAL,
    .bNumConfigurations = 1,
};

static const tusb_desc_device_t usb_case_device_desc_blength_too_short = {
    .bLength = 8u,
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

static const tusb_desc_device_t usb_case_device_desc_blength_too_long = {
    .bLength = 64u,
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

static const tusb_desc_device_t usb_case_device_desc_unknown_class = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0xAAu,
    .bDeviceSubClass = 0x00u,
    .bDeviceProtocol = 0x00u,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x2E8A,
    .idProduct = 0x4005,
    .bcdDevice = 0x0100,
    .iManufacturer = USB_CASE_STRIDX_MANUFACTURER,
    .iProduct = USB_CASE_STRIDX_PRODUCT,
    .iSerialNumber = USB_CASE_STRIDX_SERIAL,
    .bNumConfigurations = 1,
};

static const tusb_desc_device_t usb_case_device_desc_zero_vid_pid = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x0000u,
    .idProduct = 0x0000u,
    .bcdDevice = 0x0100,
    .iManufacturer = USB_CASE_STRIDX_MANUFACTURER,
    .iProduct = USB_CASE_STRIDX_PRODUCT,
    .iSerialNumber = USB_CASE_STRIDX_SERIAL,
    .bNumConfigurations = 1,
};

static uint8_t const *usb_case_select_device_descriptor(void)
{
#if defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
    if (usb_case_is_msc_persona()) {
        return (uint8_t const *)&usb_case_device_desc_msc;
    }
#endif
#if (USB_CASE_ID == 10u)
    return (uint8_t const *)&usb_case_device_desc_blength_too_short;
#elif (USB_CASE_ID == 11u)
    return (uint8_t const *)&usb_case_device_desc_blength_too_long;
#elif (USB_CASE_ID == 12u)
    return (uint8_t const *)&usb_case_device_desc_unknown_class;
#elif (USB_CASE_ID == 13u)
    return (uint8_t const *)&usb_case_device_desc_zero_vid_pid;
#else
    return (uint8_t const *)&usb_case_device_desc;
#endif
}

#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1)
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
#endif

#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
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
#endif

#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1) && defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
static const uint8_t usb_case_cfg_desc_composite_cdc_hid[] = {
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, USB_CASE_COMPOSITE_CDC_HID_TOTAL_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(
        USB_CASE_ITF_NUM_CDC_COMM,
        USB_CASE_STRIDX_CDC,
        USB_CASE_ENDPOINT_CDC_NOTIF,
        8,
        USB_CASE_ENDPOINT_CDC_OUT,
        USB_CASE_ENDPOINT_CDC_IN,
        64),
    TUD_HID_DESCRIPTOR(
        USB_CASE_ITF_NUM_HID,
        USB_CASE_STRIDX_HID,
        HID_ITF_PROTOCOL_NONE,
        sizeof(usb_case_hid_report_desc),
        USB_CASE_ENDPOINT_HID_IN,
        CFG_TUD_HID_EP_BUFSIZE,
        10),
};
#endif

#if defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
static const uint8_t usb_case_cfg_desc_msc[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, USB_CASE_MSC_TOTAL_LEN, 0x80, 100),
    TUD_MSC_DESCRIPTOR(
        USB_CASE_ITF_NUM_MSC,
        USB_CASE_STRIDX_MSC,
        USB_CASE_ENDPOINT_MSC_OUT,
        USB_CASE_ENDPOINT_MSC_IN,
        CFG_TUD_MSC_EP_BUFSIZE),
};
#endif

#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1)
static const char *const usb_case_cdc_strings[] = {
    "",
    "RP2040 USB Research",
    "usb_case_custom_demo CDC baseline",
    "",
    "USB Case CDC",
};
#endif

#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
static const char *const usb_case_hid_strings[] = {
    "",
    "RP2040 USB Research",
    "usb_case_custom_demo HID baseline",
    "",
    "USB Case HID (inert)",
};
#endif

#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1) && defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
static const char *const usb_case_composite_cdc_hid_strings[] = {
    "",
    "RP2040 USB Research",
    "usb_case_custom_demo Composite CDC+HID baseline",
    "",
    "USB Case CDC",
    "USB Case HID (inert)",
};
#endif

static const char *const usb_case_msc_strings[] = {
    "",
    "RP2040 USB Research",
    "usb_case_msc_demo MSC baseline",
    "",
    "USB Case MSC read-only",
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
    return usb_case_select_device_descriptor();
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
    if (usb_case_is_hid_persona()) {
        return usb_case_cfg_desc_hid;
    }
#endif
#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1) && defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
    if (usb_case_is_composite_persona()) {
        return usb_case_cfg_desc_composite_cdc_hid;
    }
#endif
#if defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
    if (usb_case_is_msc_persona()) {
        return usb_case_cfg_desc_msc;
    }
#endif
#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1)
    return usb_case_cfg_desc_cdc;
#elif defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
    return usb_case_cfg_desc_msc;
#else
    return NULL;
#endif
}

#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return usb_case_hid_report_desc;
}
#endif

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static uint16_t desc_str[32];
    const char *str = NULL;
    const char *const *table = NULL;
    uint8_t table_len = 0u;
    size_t char_count = 0u;

    (void)langid;

#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1)
    table = usb_case_cdc_strings;
    table_len = (uint8_t)(sizeof(usb_case_cdc_strings) / sizeof(usb_case_cdc_strings[0]));
#endif

#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
    if (usb_case_is_hid_persona()) {
        table = usb_case_hid_strings;
        table_len = (uint8_t)(sizeof(usb_case_hid_strings) / sizeof(usb_case_hid_strings[0]));
    } else
#endif
#if defined(CFG_TUD_CDC) && (CFG_TUD_CDC == 1) && defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
    if (usb_case_is_composite_persona()) {
        table = usb_case_composite_cdc_hid_strings;
        table_len = (uint8_t)(sizeof(usb_case_composite_cdc_hid_strings) / sizeof(usb_case_composite_cdc_hid_strings[0]));
    } else
#endif
#if defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
    if (usb_case_is_msc_persona()) {
        table = usb_case_msc_strings;
        table_len = (uint8_t)(sizeof(usb_case_msc_strings) / sizeof(usb_case_msc_strings[0]));
    }
#endif

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

#if defined(CFG_TUD_HID) && (CFG_TUD_HID == 1)
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

#if defined(CFG_TUD_MSC) && (CFG_TUD_MSC == 1)
enum {
    USB_CASE_MSC_BLOCK_SIZE = 512u,
    USB_CASE_MSC_BLOCK_COUNT = 16u,
    USB_CASE_MSC_MEDIA_SIZE = USB_CASE_MSC_BLOCK_SIZE * USB_CASE_MSC_BLOCK_COUNT,
};

static uint8_t usb_case_msc_media[USB_CASE_MSC_MEDIA_SIZE];
static bool usb_case_msc_media_initialized = false;

static void usb_case_msc_init_media(void)
{
    if (usb_case_msc_media_initialized) {
        return;
    }

    for (uint32_t i = 0u; i < USB_CASE_MSC_MEDIA_SIZE; i++) {
        usb_case_msc_media[i] = (uint8_t)(i & 0xffu);
    }
    usb_case_msc_media_initialized = true;
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    (void)lun;
    memcpy(vendor_id, "RP2040  ", 8u);
    memcpy(product_id, "MSC READONLY RAM", 16u);
    memcpy(product_rev, "0001", 4u);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    (void)lun;
    usb_case_msc_init_media();
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
    (void)lun;
#if (USB_CASE_ID == 52u)
    *block_count = 0u;
#else
    *block_count = USB_CASE_MSC_BLOCK_COUNT;
#endif
#if (USB_CASE_ID == 51u)
    *block_size = 0u;
#else
    *block_size = USB_CASE_MSC_BLOCK_SIZE;
#endif
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void)lun;
    (void)power_condition;
    (void)start;
    (void)load_eject;
    return true;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    uint32_t media_offset = 0u;

    (void)lun;
    usb_case_msc_init_media();

    media_offset = (lba * USB_CASE_MSC_BLOCK_SIZE) + offset;
    if (media_offset >= USB_CASE_MSC_MEDIA_SIZE) {
        return -1;
    }

    if (bufsize > (USB_CASE_MSC_MEDIA_SIZE - media_offset)) {
        bufsize = USB_CASE_MSC_MEDIA_SIZE - media_offset;
    }

    memcpy(buffer, &usb_case_msc_media[media_offset], bufsize);
    return (int32_t)bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    (void)lun;
    (void)lba;
    (void)offset;
    (void)buffer;
    (void)bufsize;

    tud_msc_set_sense(lun, SCSI_SENSE_DATA_PROTECT, 0x27, 0x00);
    return -1;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;

    switch (scsi_cmd[0]) {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
        return 0;
    case SCSI_CMD_MODE_SENSE_6:
        /* Report read-only device. */
        if (bufsize < 4u) {
            return -1;
        }
        ((uint8_t *)buffer)[0] = 0x03u;
        ((uint8_t *)buffer)[1] = 0x00u;
        ((uint8_t *)buffer)[2] = 0x80u;
        ((uint8_t *)buffer)[3] = 0x00u;
        return 4;
    default:
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
        return -1;
    }
}
#endif
#endif

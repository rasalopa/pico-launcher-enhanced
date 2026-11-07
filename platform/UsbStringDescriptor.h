#pragma once
#include <cstddef>
#include "tusb.h"

template<std::size_t N>
struct UsbStringDescriptor
{
    u8 length = 2 * N;
    u8 descriptorType = TUSB_DESC_STRING;
    char16_t string[N - 1];

    explicit constexpr UsbStringDescriptor(const char16_t(&str)[N])
    {
        for (size_t i = 0; i < N - 1; i++)
        {
            string[i] = str[i];
        }
    }

    explicit constexpr UsbStringDescriptor(u16 languageId)
    {
        string[0] = languageId;
    }
};

#define USB_STRING_DESCRIPTOR(str)  ({ static const UsbStringDescriptor descriptor(str); (const u16*)&descriptor; })

#include "common.h"
#include "GbFileType.h"

// The short name picks the banner, icon and cover folders (/_pico/icons/gb and so on), so
// the two extensions get one each, as gba has.
const GbFileType GbFileType::sGbInstance("gb");
const GbFileType GbFileType::sGbcInstance("gbc");

#include "common.h"
#include "RomPublisherCodes.h"

// Nintendo kept a two character company code in the rom header: the GBA maker code, the
// DS maker code, the Game Boy's new licensee code and the SNES expanded maker code all use
// the same two character format. Assignments were revised over the years, so a code can name
// a different company on an older cartridge; this table follows the GBA-era names. The old one
// byte codes are a separate table below.
//
// Sources: the new licensee and old licensee lists in Pan Docs (The Cartridge Header) and
// the publisher code list behind the Dolphin and Cemu game IDs. Only codes whose company is
// known are listed: a guessed name would be shown as fact on screen, while an unknown code
// simply leaves the line out.
typedef struct
{
    char code[2];
    const char* name;
} rom_publisher_t;

static const rom_publisher_t sPublishers[] =
{
    { { '0', '1' }, "Nintendo" },
    { { '0', '2' }, "Nintendo" },
    { { '0', '6' }, "Falcom" },
    { { '0', '8' }, "Capcom" },
    { { '0', 'A' }, "Jaleco" },
    { { '0', 'Q' }, "IE Institute" },
    { { '1', '3' }, "Electronic Arts" },
    { { '1', '8' }, "Hudson Soft" },
    { { '1', '9' }, "B-AI" },
    { { '1', 'K' }, "Titus Software" },
    { { '2', '0' }, "DSI Games" },
    { { '2', '2' }, "Planning Office WADA" },
    { { '2', '4' }, "PCM Complete" },
    { { '2', '5' }, "San-X" },
    { { '2', '8' }, "Kemco" },
    { { '2', '9' }, "SETA" },
    { { '2', 'K' }, "NEC Interchannel" },
    { { '2', 'L' }, "Agatsuma Entertainment" },
    { { '2', 'M' }, "Jorudan" },
    { { '2', 'N' }, "Rocket Company" },
    { { '2', 'Q' }, "MediaKite" },
    { { '3', '0' }, "Viacom" },
    { { '3', '1' }, "Nintendo" },
    { { '3', '2' }, "Bandai" },
    { { '3', '3' }, "Ocean Software" },
    { { '3', '4' }, "Konami" },
    { { '3', '5' }, "HectorSoft" },
    { { '3', '6' }, "Codemasters" },
    { { '3', '7' }, "Taito" },
    { { '3', '8' }, "Hudson Soft" },
    { { '3', '9' }, "Banpresto" },
    { { '4', '1' }, "Ubisoft" },
    { { '4', '2' }, "Atlus" },
    { { '4', '4' }, "Malibu Interactive" },
    { { '4', '6' }, "Angel" },
    { { '4', '7' }, "Bullet-Proof Software" },
    { { '4', '9' }, "Irem" },
    { { '4', 'F' }, "Eidos Interactive" },
    { { '4', 'Q' }, "Disney Interactive Studios" },
    { { '4', 'Z' }, "Crave Entertainment" },
    { { '5', '0' }, "Absolute" },
    { { '5', '1' }, "Acclaim Entertainment" },
    { { '5', '2' }, "Activision" },
    { { '5', '3' }, "Sammy" },
    { { '5', '4' }, "2K Games" },
    { { '5', '5' }, "Hi Tech Expressions" },
    { { '5', '6' }, "LJN" },
    { { '5', '7' }, "Matchbox" },
    { { '5', '8' }, "Mattel" },
    { { '5', '9' }, "Milton Bradley" },
    { { '5', 'D' }, "Midway Games" },
    { { '5', 'G' }, "Majesco Entertainment" },
    { { '5', 'H' }, "3DO" },
    { { '5', 'L' }, "NewKidCo" },
    { { '5', 'S' }, "Xicat Interactive" },
    { { '5', 'Z' }, "Conspiracy Entertainment" },
    { { '6', '0' }, "Titus Software" },
    { { '6', '1' }, "Virgin Interactive" },
    { { '6', '4' }, "LucasArts" },
    { { '6', '7' }, "Ocean Software" },
    { { '6', '8' }, "ZeniMax Media" },
    { { '6', '9' }, "Electronic Arts" },
    { { '6', 'K' }, "UFO Interactive" },
    { { '6', 'L' }, "BAM! Entertainment" },
    { { '6', 'M' }, "Studio 3" },
    { { '6', 'N' }, "Midas Interactive" },
    { { '6', 'S' }, "TDK Mediactive" },
    { { '6', 'U' }, "The Adventure Company" },
    { { '6', 'V' }, "JoWooD Entertainment" },
    { { '6', 'W' }, "Sega" },
    { { '6', 'X' }, "Wanadoo Edition" },
    { { '6', 'Z' }, "ITE Media" },
    { { '7', '0' }, "Atari" },
    { { '7', '1' }, "Interplay Entertainment" },
    { { '7', '2' }, "Broderbund" },
    { { '7', '3' }, "Sculptured Software" },
    { { '7', '5' }, "SCi" },
    { { '7', '8' }, "THQ" },
    { { '7', '9' }, "Accolade" },
    { { '7', 'D' }, "Sierra Entertainment" },
    { { '7', 'F' }, "Kemco" },
    { { '7', 'G' }, "Rage Software" },
    { { '7', 'H' }, "Encore Software" },
    { { '7', 'J' }, "Zushi Games" },
    { { '7', 'K' }, "Kiddinx Entertainment" },
    { { '7', 'L' }, "Simon & Schuster" },
    { { '7', 'N' }, "Empire Interactive" },
    { { '7', 'S' }, "Rockstar Games" },
    { { '7', 'T' }, "Scholastic" },
    { { '7', 'U' }, "Ignition Entertainment" },
    { { '8', '0' }, "Misawa Entertainment" },
    { { '8', '2' }, "Namco Bandai Games" },
    { { '8', '3' }, "LOZC G." },
    { { '8', '6' }, "Tokuma Shoten" },
    { { '8', '7' }, "Tsukuda Original" },
    { { '8', 'B' }, "Bullet-Proof Software" },
    { { '8', 'C' }, "Vic Tokai" },
    { { '8', 'E' }, "Character Soft" },
    { { '8', 'M' }, "CyberFront" },
    { { '8', 'N' }, "Success" },
    { { '8', 'P' }, "Sega" },
    { { '9', '1' }, "Chunsoft" },
    { { '9', '2' }, "Video System" },
    { { '9', '3' }, "Ocean Software" },
    { { '9', '5' }, "Varie" },
    { { '9', '6' }, "Yonezawa" },
    { { '9', '7' }, "Kaneko" },
    { { '9', '9' }, "Marvelous Entertainment" },
    { { '9', 'B' }, "Tecmo" },
    { { '9', 'G' }, "Take-Two Interactive" },
    { { '9', 'H' }, "Bottom Up" },
    { { 'A', '4' }, "Konami" },
    { { 'A', '7' }, "Takara" },
    { { 'A', 'F' }, "Namco Bandai Games" },
    { { 'B', '0' }, "Acclaim Entertainment" },
    { { 'B', '1' }, "ASCII Corporation" },
    { { 'B', '2' }, "Bandai Games" },
    { { 'B', '4' }, "Enix" },
    { { 'B', '6' }, "HAL Laboratory" },
    { { 'B', '7' }, "SNK" },
    { { 'B', '9' }, "Pony Canyon" },
    { { 'B', 'A' }, "Culture Brain" },
    { { 'B', 'B' }, "Sunsoft" },
    { { 'B', 'D' }, "Sony Imagesoft" },
    { { 'B', 'F' }, "Sammy" },
    { { 'B', 'L' }, "MTO" },
    { { 'B', 'N' }, "Sunrise Interactive" },
    { { 'B', 'P' }, "Global A Entertainment" },
    { { 'C', '0' }, "Taito" },
    { { 'C', '8' }, "Koei" },
    { { 'C', 'M' }, "Konami" },
    { { 'C', 'Q' }, "From Software" },
    { { 'D', '9' }, "Banpresto" },
    { { 'D', 'A' }, "Takara Tomy" },
    { { 'D', 'K' }, "Kodansha" },
    { { 'D', 'Q' }, "Compile Heart" },
    { { 'E', '5' }, "Epoch" },
    { { 'E', '7' }, "Athena" },
    { { 'E', '8' }, "Asmik Ace Entertainment" },
    { { 'E', '9' }, "Natsume" },
    { { 'E', 'B' }, "Atlus" },
    { { 'E', 'L' }, "Spike" },
    { { 'E', 'M' }, "Konami" },
    { { 'E', 'P' }, "Sting Entertainment" },
    { { 'E', 'S' }, "Starfish SD" },
    { { 'E', 'Y' }, "Vblank Entertainment" },
    { { 'F', 'H' }, "Easy Interactive" },
    { { 'F', 'J' }, "Virtual Toys" },
    { { 'F', 'K' }, "The Game Factory" },
    { { 'F', 'P' }, "Mastiff" },
    { { 'F', 'R' }, "DTP Entertainment" },
    { { 'F', 'S' }, "XS Games" },
    { { 'G', '0' }, "Alpha Unit" },
    { { 'G', '2' }, "Yuke's" },
    { { 'G', '9' }, "D3 Publisher" },
    { { 'G', 'D' }, "Square Enix" },
    { { 'G', 'E' }, "Kids Station" },
    { { 'G', 'G' }, "O3 Entertainment" },
    { { 'G', 'J' }, "Detn8" },
    { { 'G', 'L' }, "Gameloft" },
    { { 'G', 'M' }, "Gamecock Media Group" },
    { { 'G', 'N' }, "Oxygen Games" },
    { { 'G', 'R' }, "GSP" },
    { { 'G', 'T' }, "505 Games" },
    { { 'G', 'X' }, "Commodore" },
    { { 'G', 'Y' }, "The Game Factory" },
    { { 'H', '3' }, "Zen United" },
    { { 'H', '4' }, "SNK Playmore" },
    { { 'H', 'A' }, "Dorasu" },
    { { 'H', 'F' }, "Level-5" },
    { { 'H', 'G' }, "Graffiti Entertainment" },
    { { 'H', 'H' }, "Focus Home Interactive" },
    { { 'H', 'J' }, "Genius Products" },
    { { 'H', 'L' }, "Frontier Developments" },
    { { 'H', 'N' }, "High Voltage Software" },
    { { 'H', 'S' }, "Tru Blu" },
    { { 'H', 'U' }, "Ghostfire Games" },
    { { 'H', 'W' }, "Incredible Technologies" },
    { { 'H', 'Y' }, "Reef Entertainment" },
    { { 'H', 'Z' }, "Nordcurrent" },
    { { 'J', '8' }, "D4 Enterprise" },
    { { 'J', '9' }, "AQ Interactive" },
    { { 'J', 'F' }, "Arc System Works" },
    { { 'J', 'H' }, "City Interactive" },
    { { 'J', 'J' }, "Deep Silver" },
    { { 'J', 'R' }, "Engine Software" },
    { { 'J', 'S' }, "Digital Leisure" },
    { { 'J', 'T' }, "Empty Clip Studios" },
    { { 'J', 'W' }, "BigBen Interactive" },
    { { 'J', 'X' }, "Shin'en Multimedia" },
    { { 'J', 'Z' }, "505 Games" },
    { { 'K', '6' }, "Nihon System" },
    { { 'K', 'B' }, "NIS America" },
    { { 'K', 'J' }, "Studio Zan" },
    { { 'K', 'L' }, "Abylight" },
    { { 'K', 'M' }, "Deep Silver" },
    { { 'K', 'P' }, "Purple Hills" },
    { { 'K', 'Q' }, "Over the Top Games" },
    { { 'K', 'W' }, "Semnat Studios" },
    { { 'L', 'G' }, "Black Bean Games" },
    { { 'L', 'J' }, "Legendo Entertainment" },
    { { 'L', 'T' }, "Legacy Interactive" },
    { { 'L', 'Y' }, "Icon Games" },
    { { 'M', 'D' }, "Ateam" },
    { { 'M', 'J' }, "Mumbo Jumbo" },
    { { 'M', 'L' }, "DTP Young Entertainment" },
    { { 'M', 'R' }, "Mindscape" },
    { { 'M', 'S' }, "Milestone" },
    { { 'M', 'T' }, "Blast! Entertainment" },
    { { 'M', 'V' }, "Marvelous Entertainment" },
    { { 'M', 'Z' }, "Mad Catz" },
    { { 'N', '9' }, "Tera Box" },
    { { 'N', 'G' }, "Nordic Games" },
    { { 'N', 'H' }, "Gevo Entertainment" },
    { { 'N', 'J' }, "Enjoy Gaming" },
    { { 'N', 'K' }, "Neko Entertainment" },
    { { 'N', 'P' }, "Nobilis" },
    { { 'N', 'Q' }, "Namco Bandai Games" },
    { { 'N', 'R' }, "Bold Games" },
    { { 'N', 'S' }, "NIS America" },
    { { 'N', 'V' }, "Nicalis" },
    { { 'N', 'W' }, "Deep Fried Entertainment" },
    { { 'P', 'H' }, "Playful Entertainment" },
    { { 'P', 'K' }, "Knowledge Adventure" },
    { { 'P', 'L' }, "Playlogic Entertainment" },
    { { 'P', 'Q' }, "PopCap Games" },
    { { 'P', 'Z' }, "GameMill Publishing" },
    { { 'Q', 'C' }, "Kadokawa Shoten" },
    { { 'Q', 'H' }, "Virtual Play Games" },
    { { 'Q', 'X' }, "Press Play" },
    { { 'R', 'G' }, "Ronimo Games" },
    { { 'R', 'M' }, "Rondomedia" },
    { { 'R', 'N' }, "Mastiff" },
    { { 'R', 'S' }, "Brash Entertainment" },
    { { 'R', 'T' }, "RTL Enterprises" },
    { { 'R', 'V' }, "bitComposer Games" },
    { { 'R', 'W' }, "RealArcade" },
    { { 'R', 'Z' }, "Akaoni Studio" },
    { { 'S', '5' }, "SouthPeak Games" },
    { { 'S', 'J' }, "Cosmonaut Games" },
    { { 'S', 'P' }, "Blade Interactive Studios" },
    { { 'S', 'V' }, "SevenOne Intermedia" },
    { { 'S', 'Z' }, "Storm City Games" },
    { { 'T', 'L' }, "Telltale Games" },
    { { 'T', 'R' }, "Tetris Online" },
    { { 'T', 'V' }, "Tivola" },
    { { 'T', 'W' }, "Two Tribes" },
    { { 'U', 'G' }, "Data Design Interactive" },
    { { 'U', 'J' }, "Ghostlight" },
    { { 'U', 'P' }, "EnjoyUp Games" },
    { { 'U', 'U' }, "Onteca" },
    { { 'U', 'W' }, "Coresoft" },
    { { 'V', 'N' }, "Valcon Games" },
    { { 'V', 'P' }, "Virgin Play" },
    { { 'V', 'S' }, "Korner Entertainment" },
    { { 'V', 'T' }, "Microforum Games" },
    { { 'V', 'Z' }, "Little Orbit" },
    { { 'W', 'G' }, "2D Boy" },
    { { 'W', 'J' }, "Studio Walljump" },
    { { 'W', 'R' }, "Warner Bros. Interactive Entertainment" },
    { { 'W', 'Y' }, "WayForward Technologies" },
    { { 'X', '3' }, "CK Games" },
    { { 'X', 'G' }, "XGen Studios" },
    { { 'X', 'J' }, "XSEED Games" },
    { { 'X', 'K' }, "Exkee" },
    { { 'X', 'M' }, "DreamBox Games" },
    { { 'X', 'S' }, "Aksys Games" },
    { { 'X', 'T' }, "Funbox Media" },
    { { 'X', 'V' }, "Keystone Game Studio" },
    { { 'Y', '1' }, "Tubby Games" },
    { { 'Y', 'G' }, "Maximum Family Games" },
    { { 'Y', 'T' }, "Valcon Games" },
    { { 'Y', 'Y' }, "FDG Entertainment" },
    { { 'Z', '4' }, "Ntreev Soft" },
    { { 'Z', 'A' }, "WBA Interactive" },
    { { 'Z', 'G' }, "Zallag" },
    { { 'Z', 'S' }, "Zinkia Entertainment" },
    { { 'Z', 'W' }, "Judo Baby" },
    { { 'Z', 'X' }, "TopWare Interactive" }
};

// The old one byte code used by the Game Boy before the two character one, by the optional
// Nintendo header on the NES and by the SNES developer ID. 0x33 is not a company: on the Game
// Boy it says the two character code at 0x144 is the one to read.
typedef struct
{
    u8 code;
    const char* name;
} gameboy_licensee_t;

static const gameboy_licensee_t sGameBoyLicensees[] =
{
    { 0x01, "Nintendo" },
    { 0x08, "Capcom" },
    { 0x09, "HOT-B" },
    { 0x0A, "Jaleco" },
    { 0x0B, "Coconuts Japan" },
    { 0x0C, "Elite Systems" },
    { 0x13, "Electronic Arts" },
    { 0x18, "Hudson Soft" },
    { 0x19, "ITC Entertainment" },
    { 0x1A, "Yanoman" },
    { 0x1D, "Japan Clary" },
    { 0x1F, "Virgin Games" },
    { 0x24, "PCM Complete" },
    { 0x25, "San-X" },
    { 0x28, "Kemco" },
    { 0x29, "SETA" },
    { 0x30, "Infogrames" },
    { 0x31, "Nintendo" },
    { 0x32, "Bandai" },
    { 0x34, "Konami" },
    { 0x35, "HectorSoft" },
    { 0x38, "Capcom" },
    { 0x39, "Banpresto" },
    { 0x3C, "Entertainment Interactive" },
    { 0x3E, "Gremlin" },
    { 0x41, "Ubisoft" },
    { 0x42, "Atlus" },
    { 0x44, "Malibu Interactive" },
    { 0x46, "Angel" },
    { 0x47, "Spectrum HoloByte" },
    { 0x49, "Irem" },
    { 0x4A, "Virgin Games" },
    { 0x4D, "Malibu Interactive" },
    { 0x4F, "U.S. Gold" },
    { 0x50, "Absolute" },
    { 0x51, "Acclaim Entertainment" },
    { 0x52, "Activision" },
    { 0x53, "Sammy" },
    { 0x54, "GameTek" },
    { 0x55, "Park Place" },
    { 0x56, "LJN" },
    { 0x57, "Matchbox" },
    { 0x59, "Milton Bradley" },
    { 0x5A, "Mindscape" },
    { 0x5B, "Romstar" },
    { 0x5C, "Naxat Soft" },
    { 0x5D, "Tradewest" },
    { 0x60, "Titus Interactive" },
    { 0x61, "Virgin Games" },
    { 0x67, "Ocean Software" },
    { 0x69, "Electronic Arts" },
    { 0x6E, "Elite Systems" },
    { 0x6F, "Electro Brain" },
    { 0x70, "Infogrames" },
    { 0x71, "Interplay Entertainment" },
    { 0x72, "Broderbund" },
    { 0x73, "Sculptured Software" },
    { 0x75, "The Sales Curve" },
    { 0x78, "THQ" },
    { 0x79, "Accolade" },
    { 0x7A, "Triffix Entertainment" },
    { 0x7C, "MicroProse" },
    { 0x7F, "Kemco" },
    { 0x80, "Misawa Entertainment" },
    { 0x83, "LOZC G." },
    { 0x86, "Tokuma Shoten" },
    { 0x8B, "Bullet-Proof Software" },
    { 0x8C, "Vic Tokai" },
    { 0x8E, "Ape Inc." },
    { 0x8F, "I'Max" },
    { 0x91, "Chunsoft" },
    { 0x92, "Video System" },
    { 0x93, "Tsubaraya Productions" },
    { 0x95, "Varie" },
    { 0x96, "Yonezawa" },
    { 0x97, "Kemco" },
    { 0x99, "Arc" },
    { 0x9A, "Nihon Bussan" },
    { 0x9B, "Tecmo" },
    { 0x9C, "Imagineer" },
    { 0x9D, "Banpresto" },
    { 0x9F, "Nova" },
    { 0xA1, "Hori Electric" },
    { 0xA2, "Bandai" },
    { 0xA4, "Konami" },
    { 0xA6, "Kawada" },
    { 0xA7, "Takara" },
    { 0xA9, "Technos Japan" },
    { 0xAA, "Broderbund" },
    { 0xAC, "Toei Animation" },
    { 0xAD, "Toho" },
    { 0xAF, "Namco" },
    { 0xB0, "Acclaim Entertainment" },
    { 0xB1, "ASCII Corporation" },
    { 0xB2, "Bandai" },
    { 0xB4, "Square Enix" },
    { 0xB6, "HAL Laboratory" },
    { 0xB7, "SNK" },
    { 0xB9, "Pony Canyon" },
    { 0xBA, "Culture Brain" },
    { 0xBB, "Sunsoft" },
    { 0xBD, "Sony Imagesoft" },
    { 0xBF, "Sammy" },
    { 0xC0, "Taito" },
    { 0xC2, "Kemco" },
    { 0xC3, "Square" },
    { 0xC4, "Tokuma Shoten" },
    { 0xC5, "Data East" },
    { 0xC6, "Tonkin House" },
    { 0xC8, "Koei" },
    { 0xC9, "UFL" },
    { 0xCA, "Ultra Games" },
    { 0xCB, "VAP" },
    { 0xCC, "Use Corporation" },
    { 0xCD, "Meldac" },
    { 0xCE, "Pony Canyon" },
    { 0xCF, "Angel" },
    { 0xD0, "Taito" },
    { 0xD1, "SOFEL" },
    { 0xD2, "Quest" },
    { 0xD3, "Sigma Enterprises" },
    { 0xD4, "ASK Kodansha" },
    { 0xD6, "Naxat Soft" },
    { 0xD7, "Copya System" },
    { 0xD9, "Banpresto" },
    { 0xDA, "Tomy" },
    { 0xDB, "LJN" },
    { 0xDD, "Nippon Computer Systems" },
    { 0xDE, "Human Entertainment" },
    { 0xDF, "Altron" },
    { 0xE0, "Jaleco" },
    { 0xE1, "Towa Chiki" },
    { 0xE2, "Yutaka" },
    { 0xE3, "Varie" },
    { 0xE5, "Epoch" },
    { 0xE7, "Athena" },
    { 0xE8, "Asmik Ace Entertainment" },
    { 0xE9, "Natsume" },
    { 0xEA, "King Records" },
    { 0xEB, "Atlus" },
    { 0xEC, "Epic/Sony Records" },
    { 0xEE, "IGS" },
    { 0xF0, "A Wave" },
    { 0xF3, "Extreme Entertainment" },
    { 0xFF, "LJN" }
};

static char toUpper(char c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

const char* GetRomPublisherName(const char* code)
{
    if (code[0] == 0 || code[1] == 0)
    {
        return nullptr;
    }

    const char first = toUpper(code[0]);
    const char second = toUpper(code[1]);

    for (const auto& publisher : sPublishers)
    {
        if (publisher.code[0] == first && publisher.code[1] == second)
        {
            return publisher.name;
        }
    }

    return nullptr;
}

static bool IsPublisherCode(const char* code, char first, char second)
{
    return toUpper(code[0]) == first && toUpper(code[1]) == second;
}

const char* GetGameBoyPublisherName(const char* code, bool colorGameBoy)
{
    // These codes were used for other companies before the GBA-era names in the shared
    // table. 54 is the one that changed between monochrome and color cartridges.
    if (IsPublisherCode(code, '2', '0')) return "KSS";
    if (IsPublisherCode(code, '3', '7')) return "Taito";
    if (IsPublisherCode(code, '3', '8')) return "Hudson Soft";
    if (IsPublisherCode(code, '4', '2')) return "Sunsoft";
    if (IsPublisherCode(code, '4', '7')) return "Spectrum HoloByte";
    if (IsPublisherCode(code, '5', '4')) return colorGameBoy ? "2K Games" : "GameTek";
    if (IsPublisherCode(code, '7', '0')) return "Infogrames";
    if (IsPublisherCode(code, '9', '9')) return "Pack-In-Video";

    return GetRomPublisherName(code);
}

const char* GetSnesPublisherName(const char* code)
{
    // The shared table follows the GBA-era names; these codes belonged to earlier
    // companies when SNES cartridges used them.
    if (IsPublisherCode(code, '2', '0')) return "KSS";
    if (IsPublisherCode(code, '4', '2')) return "Sunsoft";
    if (IsPublisherCode(code, '4', '7')) return "Spectrum HoloByte";
    if (IsPublisherCode(code, '5', '4')) return "GameTek";
    if (IsPublisherCode(code, '7', '0')) return "Infogrames";
    if (IsPublisherCode(code, '9', '9')) return "Pack-In-Video";

    return GetRomPublisherName(code);
}

const char* GetLegacyPublisherName(u8 code)
{
    for (const auto& licensee : sGameBoyLicensees)
    {
        if (licensee.code == code)
        {
            return licensee.name;
        }
    }

    return nullptr;
}

void CopyPublisherName(char16_t* dest, u32 destSize, const char* name)
{
    if (!dest || destSize == 0)
    {
        return;
    }

    u32 i = 0;
    if (name)
    {
        for (; *name != 0 && i + 1 < destSize; name++)
        {
            dest[i++] = (u8)*name;
        }
    }
    dest[i] = 0;
}

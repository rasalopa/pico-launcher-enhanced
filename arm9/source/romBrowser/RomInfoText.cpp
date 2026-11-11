#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "RomInfoText.h"

namespace
{
    // The regions a name tag can name. Names, as no-intro writes them, and the
    // single letters GoodTools uses. A tag such as "(USA, Europe)" is a comma
    // separated list of these.
    const char* const sRegionNames[] =
    {
        "USA", "Europe", "Japan", "World", "Australia", "Germany", "France",
        "Spain", "Italy", "Netherlands", "Sweden", "Norway", "Denmark", "Finland",
        "Portugal", "Russia", "Korea", "China", "Taiwan", "Brazil", "Canada",
        "Hong Kong", "Asia", "Scandinavia", "UK", "Ireland", "Poland", "Greece",
        "Turkey", "New Zealand", "Belgium", "Austria", "Switzerland", "Mexico",
        "Argentina", "Latin America", "South Africa"
    };

    const char* const sRegionCodes[] =
    {
        "U", "E", "J", "W", "UE", "JU", "UJ", "JE", "A"
    };

    // The tags a romhack or homebrew name carries where a retail one names its
    // region, so a name with one of them can be cleaned without a region.
    const char* const sCleanupTags[] =
    {
        "Hack", "Aftermarket", "Unlicensed", "Homebrew", "Translation",
        "Bootleg", "Pirate"
    };

    bool IsSpace(char c)
    {
        return c == ' ' || c == '\t';
    }

    char ToUpper(char c)
    {
        return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
    }

    bool EqualsIgnoreCase(const char* text, u32 length, const char* other)
    {
        u32 otherLength = strlen(other);
        if (length != otherLength)
            return false;
        for (u32 i = 0; i < length; i++)
        {
            if (ToUpper(text[i]) != ToUpper(other[i]))
                return false;
        }
        return true;
    }

    bool IsRegionName(const char* text, u32 length)
    {
        for (const char* name : sRegionNames)
        {
            if (EqualsIgnoreCase(text, length, name))
                return true;
        }
        for (const char* code : sRegionCodes)
        {
            if (EqualsIgnoreCase(text, length, code))
                return true;
        }
        return false;
    }

    /// @brief Whether the content of a group (the text between its brackets)
    ///        names a region, or a comma separated list of them.
    bool IsRegionGroup(const char* text, u32 length)
    {
        if (length == 0)
            return false;
        u32 position = 0;
        while (position < length)
        {
            u32 begin = position;
            while (position < length && text[position] != ',')
                position++;
            u32 end = position;
            while (begin < end && IsSpace(text[begin]))
                begin++;
            while (end > begin && IsSpace(text[end - 1]))
                end--;
            if (begin == end || !IsRegionName(text + begin, end - begin))
                return false;
            position++; // past the comma, when there is one
        }
        return true;
    }

    /// @brief Whether a group marks a romhack or homebrew name, or a version:
    ///        the tags such a name ends in instead of a region.
    bool IsCleanupGroup(const char* text, u32 length)
    {
        if (length == 0)
            return false;
        // a version is "v" followed by a digit, as in "(v2.0.1a)"
        if ((text[0] == 'v' || text[0] == 'V') && length > 1 &&
            text[1] >= '0' && text[1] <= '9')
        {
            return true;
        }
        for (const char* tag : sCleanupTags)
        {
            if (EqualsIgnoreCase(text, length, tag))
                return true;
        }
        return false;
    }

    /// @brief The index just past the "(...)" or "[...]" group that starts at
    ///        \p start, or 0 when no complete group starts there.
    u32 MatchGroup(const char* text, u32 start)
    {
        char open = text[start];
        if (open != '(' && open != '[')
            return 0;
        char close = open == '(' ? ')' : ']';
        for (u32 i = start + 1; text[i]; i++)
        {
            if (text[i] == close)
                return i + 1;
        }
        return 0;
    }

    /// @brief The length of a file name without its extension, so that
    ///        "Game (USA).gba" is read as "Game (USA)".
    u32 NameLength(const char* fileName)
    {
        u32 length = strlen(fileName);
        u32 i = length;
        while (i > 0 && fileName[i - 1] != '.')
            i--;
        // a leading dot is part of the name, not an extension
        return i > 1 ? i - 1 : length;
    }

    /// @brief Puts an article that no-intro appends to the end back in front, so
    ///        "Legend of Zelda, The - Link's Awakening" reads as "The Legend of
    ///        Zelda - Link's Awakening".
    void MoveArticleToFront(char* title)
    {
        const u32 length = strlen(title);
        for (u32 comma = length; comma > 0; comma--)
        {
            if (title[comma - 1] != ',')
                continue;

            u32 articleStart = comma;
            while (articleStart < length && title[articleStart] == ' ')
                articleStart++;
            u32 articleEnd = articleStart;
            while (articleEnd < length && title[articleEnd] != ' ')
                articleEnd++;

            u32 articleLength = articleEnd - articleStart;
            if (!EqualsIgnoreCase(title + articleStart, articleLength, "The") &&
                !EqualsIgnoreCase(title + articleStart, articleLength, "A") &&
                !EqualsIgnoreCase(title + articleStart, articleLength, "An"))
            {
                continue;
            }

            // The article only moves when it ends the title or leads a " - "
            // subtitle; a comma in the middle of a title is left alone.
            if (articleEnd < length &&
                (articleEnd + 2 >= length || title[articleEnd + 1] != '-' ||
                    title[articleEnd + 2] != ' '))
            {
                continue;
            }

            char result[128];
            u32 resultLength = 0;
            for (u32 i = 0; i < articleLength && resultLength < sizeof(result) - 1; i++)
                result[resultLength++] = title[articleStart + i];
            if (resultLength < sizeof(result) - 1)
                result[resultLength++] = ' ';

            u32 mainEnd = comma;
            while (mainEnd > 0 && title[mainEnd - 1] == ' ')
                mainEnd--;
            for (u32 i = 0; i < mainEnd && resultLength < sizeof(result) - 1; i++)
                result[resultLength++] = title[i];

            for (u32 i = articleEnd; i < length && resultLength < sizeof(result) - 1; i++)
                result[resultLength++] = title[i];

            result[resultLength] = 0;
            memcpy(title, result, resultLength + 1);
            return;
        }
    }

    /// @brief The title part of a no-intro, GoodTools or romhack name, with the
    ///        groups after it taken off.
    /// @return \c true when a suffix was taken off, \c false otherwise (the
    ///         buffer is left untouched then).
    bool ParseCleanName(const char* fileName, char* title, u32 titleLength)
    {
        if (!fileName || !title || titleLength == 0)
            return false;

        u32 length = NameLength(fileName);

        for (u32 i = 0; i < length; i++)
        {
            u32 groupEnd = MatchGroup(fileName, i);
            if (groupEnd == 0 || groupEnd > length)
                continue;
            // groupEnd is a character of the extension or part of a word then
            if (groupEnd < length && !IsSpace(fileName[groupEnd]))
                continue;

            bool isRegion = IsRegionGroup(fileName + i + 1, groupEnd - i - 2);
            bool isCleanup = IsCleanupGroup(fileName + i + 1, groupEnd - i - 2);

            // From this group to the end there must be nothing but more groups.
            bool onlyGroups = true;
            for (u32 j = groupEnd; j < length; )
            {
                if (IsSpace(fileName[j]))
                {
                    j++;
                    continue;
                }
                u32 nextEnd = MatchGroup(fileName, j);
                if (nextEnd == 0 || nextEnd > length ||
                    (nextEnd < length && !IsSpace(fileName[nextEnd])))
                {
                    onlyGroups = false;
                    break;
                }
                if (!isCleanup)
                    isCleanup = IsCleanupGroup(fileName + j + 1, nextEnd - j - 2);
                j = nextEnd;
            }
            if (!onlyGroups || (!isRegion && !isCleanup))
                continue;

            u32 titleEnd = i;
            while (titleEnd > 0 && IsSpace(fileName[titleEnd - 1]))
                titleEnd--;
            if (titleEnd == 0)
                continue;

            u32 copyLength = titleEnd < titleLength - 1 ? titleEnd : titleLength - 1;
            memcpy(title, fileName, copyLength);
            title[copyLength] = 0;
            return true;
        }

        return false;
    }
}

void BuildRomTitle(char16_t* title, u32 titleLength, const char* fileName)
{
    if (!title || titleLength == 0)
        return;

    title[0] = 0;
    if (!fileName)
        return;

    char name[128];
    if (!ParseCleanName(fileName, name, sizeof(name)))
    {
        u32 length = NameLength(fileName);
        u32 copyLength = length < sizeof(name) - 1 ? length : sizeof(name) - 1;
        memcpy(name, fileName, copyLength);
        name[copyLength] = 0;
    }
    MoveArticleToFront(name);
    StringUtil::Copy(title, name, titleLength);
}

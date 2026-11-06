#pragma once

/// @brief What the delete confirmation sheet shows and does, so the same sheet,
///        and its X-only confirm, serves every kind of delete.
class IDeleteConfirmViewModel
{
public:
    virtual ~IDeleteConfirmViewModel() = default;

    virtual const char16_t* GetTitle() const = 0;

    /// @brief The name of what goes, when it is UTF-16 text; nullptr to use
    ///        GetNameLine instead.
    virtual const char16_t* GetNameLine16() const { return nullptr; }

    /// @brief The name of what goes, as UTF-8.
    virtual const char* GetNameLine() const = 0;

    /// @brief A second line under the name; empty for none.
    virtual const char* GetDetailLine() const = 0;

    /// @brief The end of the detail when it needs a line of its own, so a long
    ///        name can't push it out of sight; nullptr or empty for none. Only
    ///        shown under a detail line: with no detail line it is ignored.
    virtual const char* GetDetailEndLine() const { return nullptr; }

    /// @brief Replaces the key hint while not nullptr, e.g. to say how a delete
    ///        went. Read every frame.
    virtual const char* GetStatusLine() const { return nullptr; }

    virtual void Confirm() = 0;
    virtual void Cancel() = 0;
};

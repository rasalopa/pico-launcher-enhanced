#pragma once
#include "core/String.h"
#include "core/mini-printf.h"
#include "../IRomBrowserController.h"
#include "IDeleteConfirmViewModel.h"

/// @brief View model for the delete confirmation sheet. Copies the names at
///        construction time so nothing dangles while the sheet animates.
class DeleteConfirmViewModel : public IDeleteConfirmViewModel
{
public:
    explicit DeleteConfirmViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
        , _fileName(romBrowserController->GetDeleteRomFileName())
    {
        const char* saveFileName = romBrowserController->GetDeleteSaveFileName();
        // a save kept with an emulator's rom never has slots, see RequestDeleteSelected
        int extraSaves = (int)romBrowserController->GetDeleteExtraSaveCount();
        bool hasSave = saveFileName && saveFileName[0] != 0;
        _detailEndLine[0] = 0;
        // the save's name gets the line to itself and what happens to it goes on the
        // next one: a long name is cut short, never the part that says what is deleted
        if (hasSave)
            mini_snprintf(_detailLine, sizeof(_detailLine), "The save %s", saveFileName);
        if (hasSave && romBrowserController->DeleteKeepsSave())
            mini_snprintf(_detailEndLine, sizeof(_detailEndLine), "is kept");
        else if (hasSave && extraSaves == 1)
            mini_snprintf(_detailEndLine, sizeof(_detailEndLine), "and 1 slot save are also deleted");
        else if (hasSave && extraSaves > 1)
            mini_snprintf(_detailEndLine, sizeof(_detailEndLine), "and %d slot saves are also deleted", extraSaves);
        else if (hasSave)
            mini_snprintf(_detailEndLine, sizeof(_detailEndLine), "is also deleted");
        else if (extraSaves == 1)
            mini_snprintf(_detailLine, sizeof(_detailLine), "1 slot save is also deleted");
        else if (extraSaves > 1)
            mini_snprintf(_detailLine, sizeof(_detailLine), "%d slot saves are also deleted", extraSaves);
        else
            _detailLine[0] = 0;
    }

    const char16_t* GetTitle() const override { return u"Delete game?"; }
    const char* GetNameLine() const override { return _fileName.GetString(); }
    const char* GetDetailLine() const override { return _detailLine; }
    const char* GetDetailEndLine() const override { return _detailEndLine; }

    void Confirm() override
    {
        _romBrowserController->ConfirmDelete();
    }

    void Cancel() override
    {
        _romBrowserController->CancelDelete();
    }

private:
    IRomBrowserController* _romBrowserController;
    String<char, 256> _fileName;
    char _detailLine[280];
    char _detailEndLine[40];
};

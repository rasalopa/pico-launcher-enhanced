#include "common.h"
#include "gui/FocusManager.h"
#include "gui/input/InputProvider.h"
#include "gui/views/View.h"
#include "romBrowser/viewModels/IRomBrowserItemViewModel.h"
#include "RomBrowserItemInputHandler.h"

#define LONG_PRESS_FRAMES   30

bool RomBrowserItemInputHandler::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::A))
    {
        _viewModel->Activate();
        return true;
    }
    else if (inputProvider.Triggered(InputKey::Y))
    {
        _viewModel->ShowGameInfo();
        return true;
    }
    else if (inputProvider.Triggered(InputKey::X))
    {
        // the action is decided later: a short press toggles favorite on
        // release, holding to LONG_PRESS_FRAMES toggles completed instead
        _xHeldFrames = 1;
        _xArmedItemIndex = _viewModel->GetIndex();
        return true;
    }
    else if (_xHeldFrames > 0 && _viewModel->GetIndex() != _xArmedItemIndex)
    {
        // this pooled view was rebound to another item since the press was
        // armed: a stale hold must never act on a game the user did not
        // press X over
        _xHeldFrames = 0;
        return false;
    }
    else if (_xHeldFrames > 0 && inputProvider.Current(InputKey::X))
    {
        if (++_xHeldFrames >= LONG_PRESS_FRAMES)
        {
            _viewModel->ToggleCompleted();
            _xHeldFrames = 0; // consumed; the release must not toggle favorite
            return true;
        }
        // still deciding: the frame must keep bubbling so B/START stay
        // responsive under an X that may end up being a short press
        return false;
    }
    else if (_xHeldFrames > 0 && inputProvider.Released(InputKey::X))
    {
        _viewModel->ToggleFavorite();
        _xHeldFrames = 0;
        return true;
    }
    else
    {
        // a pending hold is only valid while continuously observed: X is
        // neither held nor released this frame, so the press it belonged to
        // ended while this view was out of focus
        _xHeldFrames = 0;
        return false;
    }
}

void RomBrowserItemInputHandler::HandlePenDown(const Point& touchPoint, FocusManager& focusManager)
{
    if (_view->GetBounds().Contains(touchPoint))
    {
        _penDown = true;
        _penDownFrames = 0;
    }
}

void RomBrowserItemInputHandler::HandlePenMove(const Point& touchPoint, FocusManager& focusManager)
{
    if (_penDown && _view->GetBounds().Contains(touchPoint))
    {
        if (++_penDownFrames == LONG_PRESS_FRAMES)
        {
            // Long press
            if (focusManager.GetCurrentFocus().GetPointer() != _view)
            {
                focusManager.Focus(_view->SharedFromThis());
            }

            _viewModel->ShowGameInfo();

            _penDown = false; // pen action is complete
        }
    }
    else
    {
        _penDown = false;
    }
}

void RomBrowserItemInputHandler::HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager)
{
    if (_penDown && _view->GetBounds().Contains(lastTouchPoint))
    {
        // Short tap
        if (focusManager.GetCurrentFocus().GetPointer() == _view)
        {
            _viewModel->Activate();
        }
        else
        {
            focusManager.Focus(_view->SharedFromThis());
        }
    }

    _penDown = false;
}

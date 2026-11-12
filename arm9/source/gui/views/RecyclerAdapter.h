#pragma once
#include "core/SharedPtr.h"

class View;

/// @brief Adapter class providing the content to show in a \see RecyclerView.
///        All items in the adapter use an identical view that can be reused to
///        show another item of this adapter when it goes off screen.
class RecyclerAdapter
{
public:
    virtual ~RecyclerAdapter() { }

    /// @brief Returns the total number of items in this adapter.
    /// @return The total number of items in this adapter.
    virtual u32 GetItemCount() const = 0;

    /// @brief Returns the size of each view of this adapter.
    /// @param width Returns the with of the view.
    /// @param height Returns the height of the view.
    virtual void GetViewSize(int& width, int& height) const = 0;

    /// @brief Creates and returns a view for this adapter.
    /// @return The created view.
    virtual SharedPtr<View> CreateView() const = 0;

    /// @brief Binds the given \p view to the item at the given \p index.
    /// @param view The view to bind.
    /// @param index The item index to bind to.
    virtual void BindView(SharedPtr<View> view, int index) const = 0;

    /// @brief Releases a \p view that was previously bound with BindView, such that it can be reused.
    /// @param view The view to release.
    /// @param index The item index that was bound to the view.
    virtual void ReleaseView(SharedPtr<View> view, int index) const = 0;

    /// @brief Called by the recycler view's destructor after it released its views and before it destroys them.
    virtual void ViewsReleased() const { }

    /// @brief Returns the item to jump to when the user asks for a big step
    ///        through the list, or -1 when this adapter has nothing smarter to
    ///        offer than paging.
    /// @param fromIdx The item the selection is on.
    /// @param direction 1 to move towards the end of the list, -1 towards the start.
    /// @return The item to select, or -1 to fall back to paging.
    virtual int GetBigStepTarget(int fromIdx, int direction) const { return -1; }

    /// @brief Called by the view right after it performs a GetBigStepTarget jump,
    ///        so an adapter can react to it. Const like GetBigStepTarget: it does
    ///        not change the adapter, only forwards elsewhere. Default does nothing.
    virtual void OnBigStepJump() const { }
};

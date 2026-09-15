#include "hooking.hpp"
#include "Helpers/delegates.hpp"
#include "HMUI/ModalView.hpp"

MAKE_AUTO_HOOK_MATCH(ModalView_HandleParentViewControllerDidDeactivate, &HMUI::ModalView::HandleParentViewControllerDidDeactivate, void,
    HMUI::ModalView* self, bool removedFromHierarchy, bool screenSystemDisabling) {
    // Preserve the game's parent-canvas handling, but match PC by hiding immediately.
    // An animated hide can be interrupted when the parent view is disabled.
    bool previousAnimateParentCanvas = self->_animateParentCanvas;
    self->_animateParentCanvas = screenSystemDisabling;
    self->Hide(false, BSML::MakeSystemAction([self, previousAnimateParentCanvas] {
        self->_animateParentCanvas = previousAnimateParentCanvas;
    }));
}
